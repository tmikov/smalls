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
   Support and wrappers for Boehme GC
*/
#ifndef P1_UTIL_GC_SUPPORT
#define P1_UTIL_GC_SUPPORT

#include <gc/gc_cpp.h>
#include <gc/gc_allocator.h>

#include <string>

typedef char gc_char;
typedef std::basic_string<gc_char,std::char_traits<gc_char>, gc_allocator<gc_char> > gc_string;

#endif // P1_UTIL_GC_SUPPORT
