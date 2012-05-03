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
===============================================================================
   Compiler-dependent macros
*/
#ifndef P1_UTIL_COMPILER_H
#define P1_UTIL_COMPILER_H

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

#define container_of( pointer, type, field ) \
  ({ const __typeof( ((type *)0)->field ) *__fptr = (pointer); \
     (type *)( (char *)__fptr - offsetof(type,field) ); })

#ifdef __cplusplus
  #define AUTO_DECL( name, value )  __typeof(value) name = (value)
#endif

#endif // P1_UTIL_COMPILER_H
