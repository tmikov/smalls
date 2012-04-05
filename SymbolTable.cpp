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


#include <boost/foreach.hpp>

#include "SymbolTable.hpp"

#define _MK_ENUM(x) #x,
const char * ResWord::s_names[] =
{
  _DEF_RESWORDS
};
#undef _MK_ENUM

#define _MK_ENUM(x) #x,
const char * BindingType::s_names[] =
{
  _DEF_BIND_TYPES
};
#undef _MK_ENUM

bool Scope::bind ( Binding * & res, Symbol * sym )
{
  Binding * bnd;

  if ( (bnd = lookupOnlyHere(sym)) != NULL)
  {
    res = bnd;
    return false;
  }

  bnd = new Binding( sym, this );
  addToBindingList( bnd );
  sym->push( bnd );
  res = bnd;
  return true;
}

Binding * Scope::lookupOnlyHere ( Symbol * sym )
{
  if (sym == NULL)
    return NULL;

  int const ourLevel = level;

  for ( Binding * bind = sym->top; bind != NULL; bind = bind->prev )
  {
    Scope * const scope = bind->scope;
    if (scope == this)
      return bind;
    if (scope->level <= ourLevel)
      break;
  }
  return NULL;
}

void Scope::popBindings()
{
  for ( Binding * bnd = m_bindingList; bnd != NULL; bnd = bnd->prevInScope )
    bnd->sym->pop( bnd );
}

SymbolTable::SymbolTable ()
{
  m_uid = 0;
  m_topScope = NULL;
}

SymbolTable::~SymbolTable ( )
{
}

Symbol * SymbolTable::newSymbol ( const gc_char * name )
{
  Map::iterator it;
  if ( (it = m_map.find( name )) != m_map.end())
    return it->second;
  Symbol * sym = new Symbol( name, m_uid );
  m_map[name] = sym;
  ++m_uid;
  return sym;
}

Scope * SymbolTable::newScope ()
{
  Scope * scope = new Scope( this, m_topScope );
  m_topScope = scope;
  return scope;
}

void SymbolTable::popScope ()
{
  m_topScope->popBindings();
  m_topScope = m_topScope->parent;
}

