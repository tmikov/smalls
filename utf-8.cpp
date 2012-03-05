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


UTF8StreamDecoder::UTF8StreamDecoder ( BufferedCharInput & in, IErrorReporter & errors, SourceCoords & coords )
  : m_in( in ), m_errors( errors ), m_coords( coords )
{
}

int32_t UTF8StreamDecoder::fillBuffer ()
{
  int32_t * dst;
  m_head = m_tail = dst = m_buf;
  int32_t ch;
  do
    *dst++ = ch = _readCodePoint();
  while (ch != -1 && dst < m_buf + BUFSIZE);
  m_tail = dst;
  return *m_head++;
}

int32_t UTF8StreamDecoder::_readCodePoint ()
{
  int ch = m_in.get();
  
retry: 
  if (ch < 0)
    return -1;
  ch &= 0xFF; // Redundant, but makes me feel more secure
  
  // Ordinary ASCII?
  if ((ch & 0x80) == 0)
    return ch;
  
  // We expect a lead byte
  if ((ch & 0x40) == 0)
  {
    m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 lead byte 0x%02x", ch );
    goto resync;
  }
 
  {
    // Determine the number of continuation bytes
    unsigned len = 1;
    uint_fast32_t result = ch & 0x1F;
    if (ch & 0x20)
    {
      len = 2; // (hopefully?) avoid a RAW dependency
      result = ch & 0x0F; // (hopefully?) avoid a RAW dependency
      if (ch & 0x10)
      {
        len = 3;
        result = ch & 0x07;
        if (ch & 0x08)
        {
          len = 4;
          result = ch & 0x03;
          if (ch & 0x04)
          {
            len = 5;
            result = ch & 0x01;
            if (ch & 0x02)
            {
              m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 lead byte 0x%02x", ch);
              goto resync;
            }
          }
        }
      }
    }

    do
    {
      ch = m_in.get();
      if (ch < 0)
      {
        m_errors.errorFormat( m_coords, NULL, "EOF in the middle of UTF-8 character" );
        return -1;
      }
      ch &= 0xFF; // Redundant
      if ((ch & 0xC0) != 0x80)
      {
        m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 continuation byte 0x%02x", ch);
        goto resync;
      }

      result <<= 6;
      result |= ch & 0x3F;
      if (result == 0)
      {
        m_errors.errorFormat( m_coords, NULL, "Non-canonical UTF-8 encoding");
        goto resync;
      }
    }
    while (--len);

    if (result < 0x80)
    {
      m_errors.errorFormat( m_coords, NULL, "Non-canonical UTF-8 encoding");
      result = ' ';
    }
    else if (!isValidCodePoint(result))
    {
      m_errors.errorFormat( m_coords, NULL, "Invalid UTF-8 code point 0x%04x", result );
      result = ' ';
    }

    return result;
  }
  
resync:  
  // Resynchronize. Skip all non-leading bytes
  do
    ch = m_in.get();
  while (ch >= 0 && (ch & 0x80)!=0 && (ch & 0x40)==0);
  goto retry;
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
