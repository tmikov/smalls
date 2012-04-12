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


#ifndef FRAME_HPP
#define	FRAME_HPP

#include "base.hpp"
#include "CircularList.hpp"

class Symbol;
class Variable;
class Frame;

class Variable : public ListEntry, public gc
{
public:
  const gc_char * const name;
  Frame * const frame;

  Variable ( const gc_char * name_, Frame * frame_ )
    : name(name_), frame(frame_)
  {
    ListEntry::debugClear();
  }
};

std::ostream & operator << ( std::ostream & os, const Variable & var );

class Frame : public gc
{
public:
  Frame * const parent;
  int const level;

  Frame ( Frame * parent_ )
    : parent( parent_ ), level( parent_?parent_->level + 1 : 0)
  {}

  Variable * newVariable ( const gc_char * name )
  {
    Variable * var = new Variable( name, this );
    m_vars.push_back( var );
    return var;
  }

  Variable * newAnonymous ( const gc_char * infoPrefix )
  {
    // Note that variable names don't need to be unique in a frame
    Variable * var = new Variable( formatGCStr("tmp_%s", infoPrefix), this );
    m_vars.push_back( var );
    return var;
  }

private:
  typedef CircularList<Variable> VariableList;
  VariableList m_vars;
};

#endif	/* FRAME_HPP */

