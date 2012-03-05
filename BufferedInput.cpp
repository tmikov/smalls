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

#include "BufferedInput.hpp"

FileInput::FileInput ( FILE * f )
{}

int FileInput::fillBuffer ()
{
  if (isGood())
  {
    m_head = m_tail = m_buf;

    size_t len = std::fread( m_buf, 1, BUFSIZE, m_f );
    m_tail += len;
    if (len < BUFSIZE)
    {
      if (feof(m_f))
        m_eof = true;
      else if (ferror(m_f))
        m_error = true;
    }
  }
  
  return m_head < m_tail ? *m_head++ : -1;
}

CharBufInput::CharBufInput ( const char * str, size_t len )
{
  m_head = (unsigned char *)str;
  m_tail = m_head + len;
}

CharBufInput::CharBufInput ( const char * str )
{
  m_head = (unsigned char *)str;
  m_tail = (unsigned char *)std::strchr( str, 0 );
}

CharBufInput::CharBufInput ( const std::string & str )
{
  m_head = (unsigned char *)str.c_str();
  m_tail = m_head + str.length();
}

int CharBufInput::fillBuffer ()
{
  m_eof = true; 
  return -1;
}
