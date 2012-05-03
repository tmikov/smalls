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
#include "format-str.hpp"
#include "scopeguard.hpp"
#include "compiler.h"
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace p1 {

std::string & vappendFormatStr ( std::string & res, const char * message, std::va_list ap )
{
  char * buf = NULL;
  if (vasprintf( &buf, message, ap ) == -1)
    throw std::runtime_error("Could not format message");
  ON_BLOCK_EXIT( std::free, buf );

  return res.append( buf );
}

std::string & appendFormatStr ( std::string & res, const char * message, ... )
{
  std::va_list ap;
  va_start( ap, message );
  vappendFormatStr( res, message, ap );
  va_end( ap );

  return res;
}

std::string vformatStr ( const char * message, std::va_list ap )
{
  char * buf = NULL;
  if (vasprintf( &buf, message, ap ) == -1)
    throw std::runtime_error("Could not format message");
  ON_BLOCK_EXIT( free, buf );

  return std::string( buf );
}

std::string formatStr ( const char * message, ... )
{
  std::va_list ap;
  va_start( ap, message );
  std::string res = vformatStr( message, ap );
  va_end( ap );
  return res;
}

const gc_char * vformatGCStr ( const char * message, std::va_list ap )
{
  char * buf = NULL;
  int len;
  if ( (len = vasprintf( &buf, message, ap )) == -1)
    throw std::runtime_error("Could not format message");
  ON_BLOCK_EXIT( std::free, buf );

  gc_char * res = new (PointerFreeGC) gc_char[len+1];
  std::memcpy( res, buf, len+1 );
  return res;
}

const gc_char * formatGCStr ( const char * message, ... )
{
  std::va_list ap;
  va_start( ap, message );
  const gc_char * res = vformatGCStr( message, ap );
  va_end( ap );
  return res;
}

} // namespaces
