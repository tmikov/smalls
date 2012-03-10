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


#ifndef UTF_8_HPP
#define	UTF_8_HPP

#include <iostream>

#include "IErrorReporter.hpp"
#include "BufferedInput.hpp"

class UTF8StreamDecoder : public BufferedInput<int32_t,int32_t,-1>
{
  typedef BufferedInput<int32_t,int32_t,-1> Super;
  static const unsigned MAX_UTF8_LEN = 4;
  
  FastCharInput & m_in;
  IErrorReporter & m_errors;
  SourceCoords & m_coords;
  
  static const unsigned BUFSIZE = 256;
  int32_t m_thebuf[BUFSIZE];
  
public:
  UTF8StreamDecoder ( FastCharInput & in, IErrorReporter & errors, SourceCoords & coords );
  
private:
  const unsigned char * _readCodePoint ( const unsigned char * from, int32_t * result );
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

#define UNICODE_MAX_VALUE    0x10FFFF 
#define UNICODE_SURROGATE_LO   0xD800
#define UNICODE_SURROGATE_HI   0xDFFF

#define UNICODE_REPLACEMENT_CHARACTER 0xFFFD

inline bool isValidCodePoint ( uint32_t cp )
{
  if (cp >= UNICODE_SURROGATE_LO && cp <= UNICODE_SURROGATE_HI || cp > UNICODE_MAX_VALUE)
    return false;
  return true;
}


#endif	/* UTF_8_HPP */

