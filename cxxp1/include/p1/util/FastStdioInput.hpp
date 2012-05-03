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


#ifndef P1_UTIL_FASTSTDIOINPUT_HPP
#define P1_UTIL_FASTSTDIOINPUT_HPP

#include "FastInput.hpp"

namespace p1 {

class FastStdioInput : public BufferedCharInput
{
  FILE * m_f;
  bool m_own;
public:
  FastStdioInput ( const char * fileName, const char * mode, size_t bufSize = DEFAULT_BUFSIZE );
  FastStdioInput ( FILE * f, size_t bufSize = DEFAULT_BUFSIZE );
  virtual ~FastStdioInput ();
protected:
  /**
   * Read up to len bytes into m_tail and advance m_tail. Throw io_error on error.
   * @param len
   */
  virtual void doRead ( size_t len );
};

} // namespaces

#endif /* P1_UTIL_FASTSTDIOINPUT_HPP */
