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
   A quick end dirty hack to track and output indentation in a std::ostream.
   To be removed ASAP.
*/
#ifndef P1_UTIL_OSTREAMINDENT_HPP
#define	P1_UTIL_OSTREAMINDENT_HPP

#include <iostream>

namespace p1
{

struct OStreamSetIndent
{
  int const x;
  OStreamSetIndent ( int x_ ) : x(x_) {};

  static int s_indent_idx;

  static int indent ( std::ostream & os );
  static int indent ( std::ostream & os, int add );
};

inline std::ostream & operator << ( std::ostream & os, OStreamSetIndent si )
{
  OStreamSetIndent::indent( os, si.x );
  return os;
}

struct OStreamIndent {};

std::ostream & operator<< ( std::ostream & os, OStreamIndent w );

} // namespaces

#endif	/* P1_UTIL_OSTREAMINDENT_HPP */

