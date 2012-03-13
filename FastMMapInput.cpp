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

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "FastMMapInput.hpp"

FastMMapInput::FastMMapInput ( const char * fileName )
  : FastCharInput( NULL ),
    m_map(MAP_FAILED)
{
  int handle;
  if ( (handle = ::open( fileName, O_RDONLY )) == -1)
    throw io_error(formatStr("open %s errno=%d", fileName, errno));
  try
  {
    // Extract the file size
    struct stat fb;
    if (::fstat( handle, &fb ))
      throw io_error(formatStr("fstat %s errno=%d", fileName, errno));
    m_length = fb.st_size;
    
    if ( (m_map = ::mmap( NULL, m_length, PROT_READ, MAP_PRIVATE /*| MAP_POPULATE*/, handle, 0 )) == MAP_FAILED)
      throw io_error(formatStr("mmap %s errno=%d", fileName, errno));
    
    ::madvise( m_map, m_length, MADV_SEQUENTIAL /*| MADV_WILLNEED*/ );
  }
  catch(...)
  {
    ::close( handle );
    throw;
  }
  ::close( handle );
  
  m_head = m_buf = (unsigned char *)m_map;
  m_tail = m_head + m_length;
}

FastMMapInput::~FastMMapInput()
{
  if (m_map != MAP_FAILED)
    ::munmap( m_map, m_length );
}

size_t FastMMapInput::fillBuffer ()
{
  return available();
}
