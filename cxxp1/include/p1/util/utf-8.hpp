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
#ifndef P1_UTIL_UTF_8_HPP
#define P1_UTIL_UTF_8_HPP

#include "FastInput.hpp"
#include "gc-support.hpp"
#include <stdint.h>

namespace p1 {

class IStreamDecoderErrorReporter
{
public:
  virtual void error ( off_t inputOffset, off_t outputOffset, const gc_char * message ) = 0;
};

class UTF8StreamDecoder : public BufferedInput<int32_t,int32_t,-1>
{
  typedef BufferedInput<int32_t,int32_t,-1> Super;
  static const unsigned MAX_UTF8_LEN = 4;

  FastCharInput & m_in;
  IStreamDecoderErrorReporter & m_errors;

  static const unsigned BUFSIZE = 256;

  const unsigned char * m_saveFrom;
  off_t m_fromOffset;

  /**
   * Returns the output stream offset of the next character to be written
   */
  off_t outOffset () const
  {
    return m_bufOffset + (m_tail - m_buf);
  }

public:
  UTF8StreamDecoder ( FastCharInput & in, IStreamDecoderErrorReporter & errors );

private:
  /**
   * Decode UTF-8 bytes between 'from' and 'to' (exclusive). There are at least (MAX_UTF8_LEN-1)
   * valid bytes beyond 'to', so it is always safe to decode a valid UTF-8 character at the end
   * of the range.
   *
   * <p>32-bit output characters are stored in '*m_tail' which must be updated to point one beyond
   * the  last converted character. 'outLimit' holds the end of the result buffer. The function
   * terminates as soon as it is reached.
   *
   * @return the next value of 'from'
   */
  const unsigned char * decodeBuffer ( const unsigned char * from, const unsigned char * to, int32_t * outLimit );
protected:
  virtual void doRead ( size_t len );
};

/**
 *
 * @param dst  buffer big enough to hold at least 6 bytes
 * @param codePoint
 * @return the number of characters stored
 */
size_t encodeUTF8 ( char * dst, uint32_t codePoint );

const uint32_t UNICODE_MAX_VALUE             = 0x10FFFF;
const uint32_t UNICODE_SURROGATE_LO          =   0xD800;
const uint32_t UNICODE_SURROGATE_HI          =   0xDFFF;
const uint32_t UNICODE_REPLACEMENT_CHARACTER =   0xFFFD;

inline bool isValidCodePoint ( uint32_t cp )
{
  if ((cp >= UNICODE_SURROGATE_LO && cp <= UNICODE_SURROGATE_HI) || cp > UNICODE_MAX_VALUE)
    return false;
  return true;
}

} // namespaces

#endif	/* P1_UTIL_UTF_8_HPP */
