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


#include "FastFileInput.hpp"

FastFileInput::FastFileInput ( int handle, size_t bufSize )
 : BufferedCharInput( bufSize ),
   m_handle( handle ), m_own( false )
{
}

FastFileInput::FastFileInput ( const char * fileName, int oflags, size_t bufSize )
 : BufferedCharInput( bufSize ),
   m_handle( -1 ), m_own( true )
{
  if ( (m_handle = ::open( fileName, oflags )) == -1 )
    throw io_error(formatStr("open %s errno=%d", fileName, errno));
}


FastFileInput::~FastFileInput ()
{
  if (m_own && m_handle != -1)
    ::close( m_handle );
}

void FastFileInput::doRead ( size_t len )
{
  do
  {
    ssize_t res = ::read( m_handle, m_tail, len );
    if (res == -1)
    {
      if (errno != EINTR)
        throw io_error( formatStr("read errno=%d", errno) );
    }
    else if (res == 0)
      break;
    else
    {
      m_tail += res;
      len -= res;
    }
  }
  while (len);
}

