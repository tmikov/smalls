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

using namespace p1;
using namespace p1::smalls;

std::ostream & p1::smalls::operator<< ( std::ostream & os, const Variable & var )
{
  return os << var.name << ':' << var.frame->level;
}

Variable * Frame::newVariable ( const gc_char * name )
{
  Variable * var = new Variable( name, this );
  m_vars.push_back( var );
  return var;
}

Variable * Frame::newAnonymous ( const gc_char * infoPrefix )
{
  // Note that variable names don't need to be unique in a frame
  Variable * var = new Variable( formatGCStr("tmp_%s", infoPrefix), this );
  m_vars.push_back( var );
  return var;
}
