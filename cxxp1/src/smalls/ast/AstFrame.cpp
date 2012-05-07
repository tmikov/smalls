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
#include "AstFrame.hpp"
#include "p1/util/format-str.hpp"
#include <iostream>

using namespace p1;
using namespace p1::smalls;

std::ostream & p1::smalls::operator<< ( std::ostream & os, const AstVariable & var )
{
  return os << var.name << ':' << var.frame->level;
}

AstVariable * AstFrame::newVariable ( const gc_char * name, const SourceCoords & defCoords )
{
  AstVariable * var = new AstVariable( name, this, defCoords );
  m_vars.push_back( var );
  return var;
}

AstVariable * AstFrame::newAnonymous ( const gc_char * infoPrefix, const SourceCoords & defCoords )
{
  // Note that variable names don't really need to be unique in a frame
  AstVariable * var = new AstVariable( formatGCStr("tmp_%s_%u", infoPrefix, ++m_tmpCount), this, defCoords );
  m_vars.push_back( var );
  return var;
}
