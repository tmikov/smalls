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


#ifndef P1_UTIL_FASTFILEINPUT_HPP
#define P1_UTIL_FASTFILEINPUT_HPP

#include "FastInput.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace p1 {

class FastFileInput : public BufferedCharInput
{
  int m_handle;
  bool m_own;

public:
  FastFileInput ( const char * fileName, int oflags, size_t bufSize = DEFAULT_BUFSIZE );
  FastFileInput ( int handle, size_t bufSize = DEFAULT_BUFSIZE );
  virtual ~FastFileInput ();
protected:
  /**
   * Read up to len bytes into m_tail and advance m_tail. Throw io_error on error.
   * @param len
   */
  virtual void doRead ( size_t len );
};

} // namespaces

#endif /*  P1_UTIL_FASTFILEINPUT_HPP */
