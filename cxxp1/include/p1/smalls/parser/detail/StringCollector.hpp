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
#ifndef P1_SMALLS_PARSER_DETAIL_STRINGCOLLECTOR_HPP
#define	P1_SMALLS_PARSER_DETAIL_STRINGCOLLECTOR_HPP

#include "p1/util/gc-support.hpp"
#include <stdint.h>

namespace p1 {
namespace smalls {
namespace detail {

class StringCollector
{
  char m_staticBuf[32];
  gc_char * m_buf;
  unsigned m_bufSize, m_len;

public:
  StringCollector ()
  {
    m_buf = m_staticBuf;
    m_bufSize = sizeof(m_staticBuf);
    m_len = 0;
  }

  ~StringCollector()
  {
    if (m_buf != m_staticBuf)
      GC_FREE( m_buf );
  }

  void ensureFreeSpace ( unsigned freeSpace )
  {
    if (m_len + freeSpace > m_bufSize)
      growBuffer( m_len + freeSpace );
  }

  void append ( const char * src, unsigned len );

  void append ( char ch )
  {
    ensureFreeSpace( 1 );
    m_buf[m_len++] = ch;
  }

  void appendCodePoint ( int32_t codePoint )
  {
    if (codePoint < 128)
      append( (char)codePoint );
    else
      appendUTF8( codePoint );
  }

  const gc_char * createGCString ();

  void reset ()
  {
    m_len = 0;
  }

  unsigned length () const
  {
    return m_len;
  }

  /**
   * Temporary access to the buffer
   */
  const char * buf () const
  {
    return m_buf;
  }

private:
  void growBuffer ( unsigned minSize );
  void appendUTF8 ( int32_t ch );
};

}}} // namespaces

#endif	/* P1_SMALLS_PARSER_DETAIL_STRINGCOLLECTOR_HPP */

