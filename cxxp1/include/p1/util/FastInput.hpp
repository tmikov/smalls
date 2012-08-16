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


#ifndef P1_UTIL_FASTINPUT_HPP
#define P1_UTIL_FASTINPUT_HPP

#include "compiler.h"

#include <cstring>
#include <string>
#include <stdexcept>
#include <boost/scoped_array.hpp>

namespace p1 {

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

  __forceinline RES peek ()
  {
    if (likely(m_head != m_tail))
      return *m_head;
    else
      return slowPeek();
  }

  size_t available () const { return m_tail - m_head; }
  const ELEM * head () const { return m_head; }
  const ELEM * tail () const { return m_tail; }

  void advance ( size_t len )
  {
    assert( (ssize_t)len <= m_tail - m_head );
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
  RES    slowPeek ();
  size_t slowRead ( ELEM * dest, size_t count );
};

template<typename ELEM, typename RES, RES _EOF>
__neverinline RES FastInput<ELEM,RES,_EOF>::slowGet ()
{
  return fillBuffer() != 0 ? *m_head++ : EOFVAL;
}

template<typename ELEM, typename RES, RES _EOF>
__neverinline RES FastInput<ELEM,RES,_EOF>::slowPeek ()
{
  return fillBuffer() != 0 ? *m_head : EOFVAL;
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

  /**
   * Puts back one character. Must only be used for actual characters that have been read and must
   * always out back the character that was read.
   * @param x The character to put back
   */
  void unget ( int x )
  {
    assert( m_head > m_buf && (int)*(m_head-1) == x );
    --m_head;
  }
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
  {
    assert( bufSize > UNGET_LIMIT );
    Super::m_head = Super::m_tail = Super::m_buf + UNGET_LIMIT;
  };

public:
  static const unsigned UNGET_LIMIT = 8;

  /**
   * Puts back one character. No more than {@code UNGET_LIMIT} characters can
   * be put back
   * @param x The character to put back
   */
  __forceinline void unget ( RES x )
  {
    assert( Super::m_head > Super::m_buf );
    *--Super::m_head = (ELEM)x;
  }

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
  if (Super::m_head < Super::m_tail) // Do we have unread data?
  {
    if (Super::m_head > Super::m_buf + UNGET_LIMIT) // Do we need to shift?
    {
      Super::m_bufOffset += Super::m_head - Super::m_buf - UNGET_LIMIT;
      size_t avail;
      memmove( Super::m_buf + UNGET_LIMIT, Super::m_head, (avail = Super::m_tail - Super::m_head)*sizeof(ELEM) );
      Super::m_head = Super::m_buf + UNGET_LIMIT;
      Super::m_tail = Super::m_head + avail;
    }
  }
  else
  {
    Super::m_bufOffset += Super::m_head - Super::m_buf - UNGET_LIMIT;
    Super::m_head = Super::m_tail = Super::m_buf + UNGET_LIMIT;
  }

  doRead( Super::m_buf + m_bufSize - Super::m_tail );

  return Super::available();
}

typedef BufferedInput<unsigned char,int,-1> BufferedCharInput;

} // namespaces

#endif /* P1_UTIL_FASTINPUT_HPP */
