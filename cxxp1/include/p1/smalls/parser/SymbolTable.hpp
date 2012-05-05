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


#ifndef P1_SMALLS_PARSER_SYMBOLTABLE_HPP
#define	P1_SMALLS_PARSER_SYMBOLTABLE_HPP

#include "p1/smalls/common/SourceCoords.hpp"
#include "p1/util/gc-support.hpp"
#include <boost/unordered_map.hpp>
#include <boost/noncopyable.hpp>
#include <list>
#include <cstring>

namespace p1 {
namespace smalls {

#define _DEF_RESWORDS \
  _MK_ENUM(NONE) \
  _MK_ENUM(QUOTE) \
/* Handled by a macro \
  _MK_ENUM(QUASIQUOTE) \
  _MK_ENUM(UNQUOTE) \
  _MK_ENUM(UNQUOTE_SPLICING) \
*/ \
  _MK_ENUM(SYNTAX) \
  _MK_ENUM(QUASISYNTAX) \
  _MK_ENUM(UNSYNTAX) \
  _MK_ENUM(UNSYNTAX_SPLICING) \
  \
  _MK_ENUM(IF) \
  _MK_ENUM(BEGIN) \
  _MK_ENUM(LAMBDA) \
  _MK_ENUM(DEFINE) \
  _MK_ENUM(SETBANG) \
  _MK_ENUM(LET) \
  _MK_ENUM(LETREC) \
  _MK_ENUM(LETREC_STAR) \
  \
  _MK_ENUM(BUILTIN) \
  _MK_ENUM(DEFINE_MACRO) \
  _MK_ENUM(DEFINE_IDENTIFIER_MACRO) \
  _MK_ENUM(DEFINE_SET_MACRO) \
  _MK_ENUM(MACRO_ENV) \
  \
  _MK_ENUM(UNSPECIFIED)

struct ResWord
{
  #define _MK_ENUM(x)  x,
  enum Enum
  {
    _DEF_RESWORDS
  };
  #undef _MK_ENUM

  static const char * name ( Enum x )  { return s_names[x]; }
private:
  static const char * s_names[];
};

class Binding;
class Symbol;
class Scope;
class SymbolTable;

#define _DEF_BIND_TYPES \
  _MK_ENUM(NONE) \
  _MK_ENUM(RESWORD) \
  _MK_ENUM(VAR) \
  _MK_ENUM(MACRO)

struct BindingKind
{
  #define _MK_ENUM(x)  x,
  enum Enum
  {
    _DEF_BIND_TYPES
  };
  #undef _MK_ENUM

  static const char * name ( Enum x )  { return s_names[x]; }
private:
  static const char * s_names[];
};

class Scope : public gc
{
public:
  Scope ( SymbolTable * symbolTable, Scope * parent_ )
    : m_symbolTable(symbolTable), parent( parent_ ), level(parent?parent->level+1:-1)
  {
    m_bindingList = NULL;
    m_active = false;
  }

  /**
    *
    * @param res
    * @param sym
    * @return true if a new symbol was defined, false if it was already present in the scope
    */
  bool bind ( Binding * & res, Symbol * sym, BindingKind::Enum btype, const SourceCoords & defCoords );
  Binding * lookupOnlyHere ( Symbol * sym );
  Binding * lookupHereAndUp ( Symbol * sym );

  void addToBindingList ( Binding * bnd );

  bool isActive () const { return m_active; }


public:
  SymbolTable * const m_symbolTable;
  Scope * const parent;
  int const level;

private:
  Binding * m_bindingList; //< linking Binding::prevInScope
  bool m_active;

  void popBindings ();

  friend class SymbolTable;
};

class Syntax;

class Macro : public gc
{
public:
  Scope * const scope;
  Macro ( Scope * scope_ ) : scope(scope_) {}
  virtual Syntax * expand ( Syntax * datum ) = 0;
};

class Binding : public gc
{
public:
  Binding ( Symbol * sym_, Scope * scope_, BindingKind::Enum btype_, const SourceCoords & defCoords_ )
    : sym(sym_), scope(scope_), bkind(btype_), defCoords(defCoords_)
  {
    this->prev = NULL;
  }

  Symbol * const sym;
  Scope * const scope;

  const BindingKind::Enum bkind;
  union
  {
    ResWord::Enum resWord;
    class Variable * var;
    Macro * macro;
  } u;

  SourceCoords defCoords; //< coordinates of the source definition

private:
  Binding * prev; //< the same symbol in the previous scope
  Binding * prevInScope; //< link to the prev binding in our scope

  friend class Scope;
  friend class Symbol;
};

std::ostream & operator << ( std::ostream & os, Binding & bnd );

inline void Scope::addToBindingList ( Binding * bnd )
{
  assert( bnd->prevInScope == NULL );
  bnd->prevInScope = m_bindingList;
  m_bindingList = bnd;
}


class Symbol : public gc, public boost::noncopyable
{
  Symbol ( const gc_char * name_, uint32_t uid_, Symbol * parentSymbol_ = NULL, uint32_t markStamp_ = 0 )
    : name( name_ ), uid( uid_ ), parentSymbol( parentSymbol_ ), markStamp( markStamp_ )
  {
    top = NULL;
  }

  void push ( Binding * bind )
  {
    bind->prev = this->top;
    this->top = bind;
  }

  void pop ( Binding * bnd )
  {
    assert( this->top == bnd );
    this->top = this->top->prev;
  }

public:
  const gc_char * const name;
  uint32_t const uid; //< different value for each symbol
  /**
   * If not NULL, links to the parent in the family of symbols with the same name but different
   * marks generated by a macro.
   */
  Symbol * const parentSymbol;
  /** Non-0 if this was a symbol generated from a macro */
  uint32_t const markStamp;

private:
  /** The currently active binding */
  Binding * top;

  friend class SymbolTable;
  friend class Scope;
};

class SymbolTable : public gc
{
public:
  SymbolTable();
  ~SymbolTable();

  Symbol * newSymbol ( const gc_char * name );
  Symbol * newSymbol ( Symbol * parentSymbol, uint32_t markStamp );

  Binding * lookup ( const Symbol * sym )
  {
    return sym->top;
  }

  Scope * newScope ();

  void popThisScope ( Scope * scope )
  {
    assert( m_topScope == scope );
    popScope();
  }

  void popScope ();

  Scope * topScope () const { return m_topScope; }

  uint32_t nextMarkStamp () { return ++m_markStamp; };

private:
  struct gc_charstr_equal : public std::binary_function<const gc_char *,const gc_char *,bool>
  {
    bool operator () ( const gc_char * a, const gc_char * b ) const
    {
      return std::strcmp( a, b ) == 0;
    }
  };

  struct gc_charstr_hash : std::unary_function<const gc_char *, std::size_t>
  {
    std::size_t operator () ( const gc_char * a ) const
    {
      std::size_t seed = 0;
      unsigned t;
      while ( (t = *((unsigned char *)a)) != 0)
      {
        boost::hash_combine( seed, t );
        ++a;
      }
      return seed;
    }
  };

  /** Map from <const gc_char *> to <Symbol *> */
  typedef boost::unordered_map<const gc_char *,
                               Symbol *,
                               gc_charstr_hash,
                               gc_charstr_equal,
                               gc_allocator<const gc_char *> > Map;

  struct MarkKey
  {
    uint32_t const markStamp;
    uint32_t const parentSymbolUid;

    MarkKey ( uint32_t markStamp_, uint32_t parentSymbolUid_ )
      : markStamp( markStamp_ ), parentSymbolUid( parentSymbolUid_ )
    {}

    bool operator== ( const MarkKey & x ) const
    {
      return this->markStamp == x.markStamp && this->parentSymbolUid == x.parentSymbolUid;
    }
  };

  struct MarkKey_hash : std::unary_function<const MarkKey &, std::size_t>
  {
    std::size_t operator () ( const MarkKey & a ) const
    {
      std::size_t seed = 0;
      boost::hash_combine( seed, a.markStamp );
      boost::hash_combine( seed, a.parentSymbolUid );
      return seed;
    }
  };

  /** Map from <const gc_char *> to <Symbol *> */
  typedef boost::unordered_map<MarkKey,
                               Symbol *,
                               MarkKey_hash,
                               std::equal_to<const MarkKey &>,
                               gc_allocator<MarkKey> > MarkMap;

  Map m_map;
  MarkMap m_markMap;
  uint32_t m_uid;
  Scope * m_topScope;
  /** Used for marking macro-expanded symbols */
  uint32_t m_markStamp;
};

class ScopePopper
{
public:
  ScopePopper ( SymbolTable & symbolTable, Scope * scope ) : m_symbolTable(symbolTable), m_scope(scope) {};

  ~ScopePopper ()
  {
    m_symbolTable.popThisScope(m_scope);
  }
private:
  SymbolTable & m_symbolTable;
  Scope * m_scope;
};

}} // namespaces

#endif	/* P1_SMALLS_PARSER_SYMBOLTABLE_HPP */
