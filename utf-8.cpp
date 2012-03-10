/*
   Copyright 2012 Tzvetan Mikov <tmikov@gmail.com>
   All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "utf-8.hpp"
#include <algorithm> // for std::min


UTF8StreamDecoder::UTF8StreamDecoder ( FastCharInput & in, IErrorReporter & errors, SourceCoords & coords )
  : Super( m_thebuf, BUFSIZE ),
    m_in( in ), m_errors( errors ), m_coords( coords )
{
}

void UTF8StreamDecoder::doRead ( size_t toRead )
{
  int32_t * const end = m_tail + toRead;
  for(;;)
  {
    const unsigned char * from, * to;
    unsigned char buf[MAX_UTF8_LEN];
    size_t avail;
    
    // If there are less than MAX_UTF8_LEN chars available, we have to refill the buffer
    if (likely((avail = m_in.available()) < MAX_UTF8_LEN) &&
        unlikely((avail = m_in.fillBuffer()) < MAX_UTF8_LEN))
    {
      // The buffer is still mostly empty. Have to proceed using the slow path
      if (avail != 0)
      {
        // Fill the small buffer and pad it with 0xFF
        std::memset( buf, 0xFF, MAX_UTF8_LEN );
        std::memcpy( buf, m_in.head(), avail );
        from = buf;
        to = buf + avail;
      }
      else
        return;
    }
    else
    {
      from = m_in.head();
      to = m_in.tail() - MAX_UTF8_LEN + 1; // we check earlier we have more than MAX_UTF8_LEN bytes; 
    }

    const unsigned char * saveFrom = from; 
    from = decodeBuffer( from, to, end );
    m_in.advance( std::min( avail,(size_t)(from - saveFrom) ) );
    if (m_tail == end)
      return;
  }
}

const unsigned char * UTF8StreamDecoder::decodeBuffer ( 
  const unsigned char * from, const unsigned char * to, int32_t * outLimit 
)
{
  int32_t * tail = m_tail;
  while (from < to)
  {
    unsigned ch = from[0];
    uint32_t result;

    if (likely((ch & 0x80) == 0)) // Ordinary ASCII?
    {
      result = ch;
      ++from;
    }
    else if (likely((ch & 0xE0) == 0xC0))
    {
      unsigned ch1 = from[1];
      if (unlikely((ch1 & 0xC0) != 0x80))
      {
        m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 continuation byte" );
        result = UNICODE_REPLACEMENT_CHARACTER;
      }
      else
      {
        result = ((ch & 0x1F) << 6) | (ch1 & 0x3F);
        if (unlikely(result <= 0x7F))
        {
          m_errors.errorFormat( m_coords, NULL, "Non-canonical UTF-8 encoding" );
          result = UNICODE_REPLACEMENT_CHARACTER;
        }
      }
      from += 2;
    }
    else if (likely((ch & 0xF0) == 0xE0))
    {
      int32_t ch1 = from[1];
      int32_t ch2 = from[2];
      if (unlikely(((ch1 | ch2) & 0xC0) != 0x80))
      {
        m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 continuation byte" );
        result = UNICODE_REPLACEMENT_CHARACTER;
      }
      else
      {
        result = ((ch & 0x0F) << 12) | ((ch1 & 0x3F) << 6) | (ch2 & 0x3F);
        if (unlikely(result <= 0x7FF))
        {
          m_errors.errorFormat( m_coords, NULL, "Non-canonical UTF-8 encoding");
          result = UNICODE_REPLACEMENT_CHARACTER;
        }
        if (unlikely(result >= UNICODE_SURROGATE_LO && result <= UNICODE_SURROGATE_HI))
        {
          m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 code point 0x%04x", result );
          result = UNICODE_REPLACEMENT_CHARACTER;
        }
      }
      from += 3;
    }
    else if ((ch & 0xF8) == 0xF0)
    {
      int32_t ch1 = from[1];
      int32_t ch2 = from[2];
      int32_t ch3 = from[3];
      if (unlikely(((ch1 | ch2 | ch3) & 0xC0) != 0x80)) \
      {
        m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 continuation byte" );
        result = UNICODE_REPLACEMENT_CHARACTER;
      }
      else
      {
        result = ((ch & 0x07) << 18) | ((ch1 & 0x3F) << 12) | ((ch2 & 0x3F) << 6) | (ch3 & 0x3F);
        if (unlikely(result <= 0xFFFF))
        {
          m_errors.errorFormat( m_coords, NULL, "Non-canonical UTF-8 encoding");
          result = UNICODE_REPLACEMENT_CHARACTER;
        }
        if (unlikely(result > UNICODE_MAX_VALUE))
        {
          m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 code point 0x%06x", result );
          result = UNICODE_REPLACEMENT_CHARACTER;
        }
      }
      from += 4;
    }
    else
    {
      m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 lead byte 0x%02x", ch );
      result = UNICODE_REPLACEMENT_CHARACTER;
      from += 1;
    }
    
    *tail = result;
    if (++tail == outLimit)
      break;
  }
  m_tail = tail;
  return from;
}

size_t encodeUTF8 ( char * dst, uint32_t cp )
{
  if (cp <= 0x7F)
  {
    *dst = (char)cp;
    return 1;
  }
  else if (cp <= 0x7FF)
  {
    dst[1] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[0] = (cp & 0x1F) | 0xC0;
    return 2;
  }
  else if (cp <= 0xFFFF)
  {
    dst[2] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[1] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[0] = (cp & 0x0F) | 0xE0;
    return 3;
  }
  else if (cp <= 0x1FFFFF)
  {
    dst[3] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[2] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[1] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[0] = (cp & 0x07) | 0xF0;
    return 4;
  }
  else if (cp <= 0x3FFFFFF)
  {
    dst[4] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[3] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[2] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[1] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[0] = (cp & 0x03) | 0xF8;
    return 5;
  }
  else
  {
    dst[5] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[4] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[3] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[2] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[1] = (cp & 0x3F) | 0x80;
    cp >>= 6;
    dst[0] = (cp & 0x01) | 0xFC;
    return 6;
  }
}
