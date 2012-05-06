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
#include "OStreamIndent.hpp"

namespace p1
{

int OStreamSetIndent::s_indent_idx = std::ostream::xalloc();

int OStreamSetIndent::indent ( std::ostream & os )
{
  return os.iword(OStreamSetIndent::s_indent_idx);
}

int OStreamSetIndent::indent ( std::ostream & os, int add )
{
  return os.iword(OStreamSetIndent::s_indent_idx) += add;
}

std::ostream & operator<< ( std::ostream & os, OStreamIndent w )
{
  os.width( OStreamSetIndent::indent(os) );
  os << "";
  return os;
}

} // namespace p1
