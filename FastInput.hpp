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


#ifndef FASTINPUT_HPP
#define	FASTINPUT_HPP

#include "base.hpp"

#include "boost/scoped_array.hpp"

class io_error : public std::runtime_error
{
public:
  io_error ( const std::string & what ) : runtime_error( what ) {};
};

template<typename ELEM, typename RES, RES _EOF>
class FastInput
{
protected:
  ELEM * m_buf,  * m_head, * m_tail;
  off_t m_bufOffset;
  
  FastInput ( ELEM * buf )
  {
    m_buf = m_head = m_tail = buf;
    m_bufOffset = 0;
  }
  
public:
  static const RES EOFVAL = _EOF;
  
  virtual ~FastInput () {};
  
  __forceinline RES get ()
  {
    if (likely(m_head != m_tail))
      return *m_head++;
    else
      return slowGet();
  }
  
  __forceinline size_t read ( ELEM * dest, size_t count )
  {
    if (likely(m_tail - m_head >= count))
    {
      std::memcpy( dest, m_head, count*sizeof(ELEM) );
      m_head += count;
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
  
  /**
   * Return the offset of the next character (head)
   */
  off_t offset () const
  {
    return m_bufOffset + (m_head - m_buf);
  }
  
  virtual size_t fillBuffer () = 0;
  
private:
  RES    slowGet ();
  size_t slowRead ( ELEM * dest, size_t count );
};

template<typename ELEM, typename RES, RES _EOF>
__neverinline RES FastInput<ELEM,RES,_EOF>::slowGet ()
{
  return fillBuffer() != 0 ? *m_head++ : EOFVAL;
}


template<typename ELEM, typename RES, RES _EOF>
__neverinline size_t FastInput<ELEM,RES,_EOF>::slowRead ( ELEM * dest, size_t count )
{
  assert( count != 0 );
  size_t res = 0;
  for(;;)
  {
    size_t len = std::min( available(), count );
    std::memcpy( dest, m_head, len * sizeof(ELEM) );
    dest += len;
    m_head += len;
    res += len;
    count -= len;
    if (!count || !fillBuffer())
      break;
  }
  return res;
}

typedef FastInput<unsigned char,int,-1> FastCharInput;

class CharBufInput : public FastCharInput
{
public:
  CharBufInput ( const char * str, size_t len );
  CharBufInput ( const char * str );
  CharBufInput ( const std::string & str );  
  virtual size_t fillBuffer ();
};

template<typename ELEM, typename RES, RES _EOF>
class BufferedInput : public FastInput<ELEM,RES,_EOF>
{
  typedef FastInput<ELEM,RES,_EOF> Super;
  
  boost::scoped_array<ELEM> m_bufCleaner;
  
protected:
  static const unsigned DEFAULT_BUFSIZE = 4096;
  size_t m_bufSize;
  
  BufferedInput ( size_t bufSize = DEFAULT_BUFSIZE )
    : Super( new ELEM[bufSize] ), m_bufCleaner( Super::m_buf ), m_bufSize(bufSize)  
  {};
  
public:

  virtual size_t fillBuffer ();
protected:
  /**
   * Read up to len bytes into m_tail and advance m_tail. Throw io_error on error.
   * @param len
   */
  virtual void doRead ( size_t len ) = 0;
};

template<typename ELEM, typename RES, RES _EOF>
size_t BufferedInput<ELEM,RES,_EOF>::fillBuffer ()
{
  Super::m_bufOffset += Super::m_head - Super::m_buf;
  if (Super::m_head < Super::m_tail) // Do we have unread data?
  {
    if (Super::m_head > Super::m_buf) // Do we need to shift?
    {
      size_t avail;
      memmove( Super::m_buf, Super::m_head, (avail = Super::m_tail - Super::m_head)*sizeof(ELEM) );
      Super::m_head = Super::m_buf;
      Super::m_tail = Super::m_head + avail;
    }
  }
  else
    Super::m_head = Super::m_tail = Super::m_buf;
  
  doRead( Super::m_buf + m_bufSize - Super::m_tail );
 
  return Super::available();
}

typedef BufferedInput<unsigned char,int,-1> BufferedCharInput;

#endif	/* FASTINPUT_HPP */

