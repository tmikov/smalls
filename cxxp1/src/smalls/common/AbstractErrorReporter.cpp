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
#include "AbstractErrorReporter.hpp"
#include "p1/util/format-str.hpp"

using namespace p1;
using namespace p1::smalls;

std::string ErrorInfo::formatMessage() const
{
  std::string res = coords.toString();
  if (!res.empty())
    res += ':';
  res += message;
  return res;
}

void AbstractErrorReporter::errorFormat ( const SourceCoords & coords, const gc_char * message, ... )
{
  std::va_list ap;
  va_start( ap, message );
  verrorFormat( coords, message, ap );
  va_end( ap );
}

void AbstractErrorReporter::verrorFormat ( const SourceCoords & coords, const gc_char * message, std::va_list ap )
{
  error( coords, vformatGCStr( message, ap ) );
}
