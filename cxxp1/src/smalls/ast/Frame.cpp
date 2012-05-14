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
#include "Frame.hpp"
#include "p1/util/format-str.hpp"
#include <iostream>

namespace p1 {
namespace smalls {
namespace ast {

std::ostream & operator<< ( std::ostream & os, const Variable & var )
{
  return os << var.name << ':' << var.frame->level;
}

Variable * Frame::newVariable ( const gc_char * name, const SourceCoords & defCoords )
{
  Variable * var = new Variable( name, this, defCoords );
  m_vars.push_back( var );
  ++m_varCount;
  return var;
}

Variable * Frame::newAnonymous ( const gc_char * infoPrefix, const SourceCoords & defCoords )
{
  // Note that variable names don't really need to be unique in a frame
  Variable * var = new Variable( formatGCStr("tmp_%s_%u", infoPrefix, m_varCount), this, defCoords );
  m_vars.push_back( var );
  ++m_varCount;
  return var;
}

}}} // namespaces