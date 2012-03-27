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


#ifndef BASE_HPP
#define	BASE_HPP

#include <stdint.h>
#include <limits>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <errno.h>
#include <cstdarg>
#include <cstdio>
#include <cassert>

#include <string>

#include <boost/noncopyable.hpp>

#include "gc/gc_cpp.h"
#include "gc/gc_allocator.h"

#ifdef __GNUC__
  #define DEFINITION_IS_NOT_USED __attribute__ ((unused))
#else
  #define DEFINITION_IS_NOT_USED
#endif

#ifdef __GNUC__
  #ifndef __forceinline
    #define __forceinline __attribute__((__always_inline__)) inline
  #endif
  #ifndef __neverinline
    #define __neverinline __attribute__((__noinline__)) inline
  #endif
#else
    #define __forceinline inline
#endif

#if defined(_MSC_VER)
  #define COMPILE_TIME_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#elif defined(__GNUC__)
  #define COMPILE_TIME_ASSERT(e) extern char __C_ASSERT__[(e)?1:-1] __attribute__((unused))
#else
  #define COMPILE_TIME_ASSERT(e)
#endif

#if defined(__GNUC__)
  #define likely( x )   __builtin_expect( !!(x), true )
  #define unlikely( x ) __builtin_expect( !!(x), false )
#else
  #define likely( x )    x
  #define unlikely( x )  x
#endif


typedef std::basic_string<char,std::char_traits<char>, gc_allocator<char> > gc_string;

typedef char gc_char;

std::string & vappendFormatStr ( std::string & res, const char * message, std::va_list ap );
std::string & appendFormatStr ( std::string & res, const char * message, ... );

std::string vformatStr ( const char * message, std::va_list ap );
std::string formatStr ( const char * message, ... );

const gc_char * vformatGCStr ( const char * message, std::va_list ap );
const gc_char * formatGCStr ( const char * message, ... );


#endif	/* BASE_HPP */

