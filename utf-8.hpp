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

class UTF8StreamDecoder : public BufferedInput<int32_t,int32_t>
{
  BufferedCharInput & m_in;
  IErrorReporter & m_errors;
  SourceCoords & m_coords;
  
  static const unsigned BUFSIZE = 256;
  int32_t m_buf[BUFSIZE];
  
public:
  UTF8StreamDecoder ( BufferedCharInput & in, IErrorReporter & errors, SourceCoords & coords );
  
private:
  int32_t _readCodePoint ();
  virtual int32_t fillBuffer ();
};

/**
 * 
 * @param dst  buffer big enough to hold at least 6 bytes
 * @param codePoint
 * @return the number of characters stored
 */
size_t encodeUTF8 ( char * dst, uint32_t codePoint );

inline bool isValidCodePoint ( uint32_t cp )
{
  if (cp >= 0xD800 && cp <= 0xDFFF || cp > 0x10ffff)
    return false;
  return true;
}


#endif	/* UTF_8_HPP */

