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


#include "SourceCoords.hpp"
#include <sstream>

using namespace p1::smalls;

std::ostream & p1::smalls::operator<< ( std::ostream & os, const SourceCoords & sc )
{
  if (sc.fileName)
    os << sc.fileName;
  if (sc.line)
    os << '(' << sc.line << ')';
  if (sc.column)
    os << '.' << sc.column;
  return os;
}

std::string SourceCoords::toString () const
{
  std::stringstream os;
  os << *this;
  return os.str();
}
