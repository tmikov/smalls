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


#include "FastStdioInput.hpp"

FastStdioInput::FastStdioInput ( FILE * f, size_t bufSize )
 : BufferedCharInput( bufSize ),
   m_f( f ), m_own(false)
{
}

FastStdioInput::FastStdioInput ( const char * fileName, const char * mode, size_t bufSize )
 : BufferedCharInput( bufSize ),
   m_f( NULL ), m_own(true)
{
  if ( !(m_f = std::fopen( fileName, mode )) )
    throw io_error(formatStr("fopen %s errno=%d", fileName, errno));
}


FastStdioInput::~FastStdioInput ()
{
  if (m_own && m_f)
    fclose( m_f );
}

void FastStdioInput::doRead ( size_t len )
{
  size_t res = std::fread( m_tail, 1, len, m_f );
  m_tail += res;
  if (res < len)
    if (ferror(m_f))
      throw io_error(formatStr("fread errno=%d", errno));
}

