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

template<typename ELEM, typename RES, RES _EOF>
class BufferedInput
{
public:
  static const RES EOFVAL = _EOF;
  
  BufferedInput ()
  {
    m_head = m_tail = 0;
    m_eof = m_error = false;
  }
  virtual ~BufferedInput () {};
  
  __forceinline RES get ()
  {
    if (likely(m_head != m_tail))
      return *m_head++;
    else
      return slowGet();
  }
  
  __forceinline size_t read ( ELEM * dest, size_t count )
  {
     // NOTE: must check for 0 first, as the rest of the code depends on the count being non=0
    if (unlikely(count == 0))
      return 0;
    if (likely(m_tail - m_head >= count))
    {
      size_t t = count;
      do
        *dest++ = *m_head++;
      while (--t);
      return count;
    }
    else
      return slowRead( dest, count );
  }
  
  size_t available () const { return m_tail - m_head; }
  const ELEM * head () const { return m_head; }
  const ELEM * tail () const { return m_tail; }
  
  void advance ( size_t len )
  {
    assert( len <= m_tail - m_head );
    m_head += len;
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
  
  virtual size_t fillBuffer () = 0;
  
protected:  
  bool m_eof, m_error;
  ELEM * m_head, * m_tail;
  
private:
  RES    slowGet ();
  size_t slowRead ( ELEM * dest, size_t count );
};

template<typename ELEM, typename RES, RES _EOF>
__neverinline RES BufferedInput<ELEM,RES,_EOF>::slowGet ()
{
  return fillBuffer() != 0 ? *m_head++ : EOFVAL;
}


template<typename ELEM, typename RES, RES _EOF>
__neverinline size_t BufferedInput<ELEM,RES,_EOF>::slowRead ( ELEM * dest, size_t count )
{
  size_t res = 0;
  do
  {
    *dest++ = get();
    if (!isGood())
      break;
    ++res;
  }
  while (--count);
  return res;
}

class BufferedCharInput : public BufferedInput<unsigned char,int,-1>
{
};

class FileInput : public BufferedCharInput
{
  FILE * m_f;
  static const unsigned BUFSIZE = 4096;
  unsigned char m_buf[BUFSIZE];
public:
  FileInput ( FILE * f );
  virtual size_t fillBuffer ();
};

class CharBufInput : public BufferedCharInput
{
public:
  CharBufInput ( const char * str, size_t len );
  CharBufInput ( const char * str );
  CharBufInput ( const std::string & str );  
  virtual size_t fillBuffer ();
};


#endif	/* BUFFEREDINPUT_HPP */

