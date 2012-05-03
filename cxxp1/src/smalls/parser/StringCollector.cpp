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
#include "detail/StringCollector.hpp"
#include "p1/util/utf-8.hpp"

using namespace p1;
using namespace p1::smalls::detail;

void StringCollector::append ( const char * src, unsigned len )
{
  ensureFreeSpace( len );
  std::memcpy( m_buf + m_len, src, len );
  m_len += len;
}

const gc_char * StringCollector::createGCString ()
{
  gc_char * res;

  // If the buffer is already heap allocated, and it is at least 75% full, just return it
  // otherwise, allocate the result in the heap
  //
  if (m_buf != m_staticBuf && m_len*4 >= m_bufSize*3)
  {
    res = m_buf;
    m_buf = m_staticBuf;
    m_bufSize = sizeof(m_staticBuf);
    m_len = 0;
  }
  else
  {
    res = new (PointerFreeGC) gc_char[m_len];
    std::memcpy( res, m_buf, m_len );
    reset();
  }

  return res;
}

void StringCollector::growBuffer ( unsigned minSize )
{
  unsigned newSize = m_bufSize << 1;
  if (newSize < minSize)
    newSize = minSize;
  gc_char * newBuf = new (PointerFreeGC) gc_char[newSize];
  std::memcpy( newBuf, m_buf, m_len );
  m_buf = newBuf;
  m_bufSize = newSize;
}

void StringCollector::appendUTF8 ( int32_t ch )
{
  ensureFreeSpace( 6 );
  m_len += encodeUTF8( m_buf + m_len, ch );
}