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


#ifndef P1_SMALLS_AST_ASTFRAME_HPP
#define	P1_SMALLS_AST_ASTFRAME_HPP

#include "p1/smalls/common/SourceCoords.hpp"
#include "p1/util/gc-support.hpp"
#include "p1/adt/CircularList.hpp"

namespace p1 {
namespace smalls {
  class Symbol;
}}

namespace p1 {
namespace smalls {

class AstVariable;
class AstFrame;

class AstVariable : public p1::ListEntry, public gc
{
public:
  const gc_char * const name;
  AstFrame * const frame;
  SourceCoords defCoords;
  void * data;

  AstVariable ( const gc_char * name_, AstFrame * frame_, const SourceCoords & defCoords_ )
    : name(name_), frame(frame_), defCoords(defCoords_)
  {
    ListEntry::debugClear();
    this->data = NULL;
  }
};

std::ostream & operator << ( std::ostream & os, const AstVariable & var );

class AstFrame : public gc
{
public:
  AstFrame * const parent;
  int const level;

  AstFrame ( AstFrame * parent_ )
    : parent( parent_ ), level( parent_?parent_->level + 1 : -1)
  {
    m_varCount = 0;
  }

  AstVariable * newVariable ( const gc_char * name, const SourceCoords & defCoords );
  AstVariable * newAnonymous ( const gc_char * infoPrefix, const SourceCoords & defCoords );

  unsigned length () const { return m_varCount; };

  typedef p1::CircularList<AstVariable> VariableList;
  VariableList & vars () { return m_vars; }

private:
  VariableList m_vars;
  unsigned m_varCount;
};

}} // namespaces

#endif	/* P1_SMALLS_AST_ASTFRAME_HPP */

