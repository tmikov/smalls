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


#ifndef BUFFEREDINPUT_HPP
#define	BUFFEREDINPUT_HPP

#include "base.hpp"

template<typename ELEM, typename RES>
class BufferedInput
{
public:
  BufferedInput ()
  {
    m_head = m_tail = 0;
    m_eof = m_error = false;
  }
  virtual ~BufferedInput () {};
  
  RES get ()
  {
    if (likely(m_head != m_tail))
      return *m_head++;
    else
      return fillBuffer();
  }
  
  bool isGood () const
  {
    return !m_eof && !m_error;
  }
  
  bool isEof () const
  {
    return m_eof;
  }
  
  bool isError () const
  {
    return m_error;
  }
  
protected:  
  bool m_eof, m_error;
  const ELEM * m_head, * m_tail;
  
  virtual RES fillBuffer () = 0;
};

class BufferedCharInput : public BufferedInput<unsigned char,int>
{
};

class FileInput : public BufferedCharInput
{
  FILE * m_f;
  static const unsigned BUFSIZE = 4096;
  unsigned char m_buf[BUFSIZE];
public:
  FileInput ( FILE * f );
protected:  
  virtual int fillBuffer ();
};

class CharBufInput : public BufferedCharInput
{
public:
  CharBufInput ( const char * str, size_t len );
  CharBufInput ( const char * str );
  CharBufInput ( const std::string & str );
protected:  
  virtual int fillBuffer ();
};


#endif	/* BUFFEREDINPUT_HPP */

