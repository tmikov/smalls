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
=============================================================================
   String formatting convenience functions
*/

#ifndef P1_FORMAT_STR_HPP
#define P1_FORMAT_STR_HPP

#include "gc-support.hpp"

#include <cstdarg>
#include <string>

namespace p1 {

std::string & vappendFormatStr ( std::string & res, const char * message, std::va_list ap );
std::string & appendFormatStr ( std::string & res, const char * message, ... );

std::string vformatStr ( const char * message, std::va_list ap );
std::string formatStr ( const char * message, ... );

const gc_char * vformatGCStr ( const char * message, std::va_list ap );
const gc_char * formatGCStr ( const char * message, ... );

} // namespaces

#endif /* P1_FORMAT_STR_HPP */
