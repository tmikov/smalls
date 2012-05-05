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


#ifndef P1_SMALLS_PARSER_SYNTAX_HPP
#define	P1_SMALLS_PARSER_SYNTAX_HPP

#include "p1/smalls/common/SourceCoords.hpp"
#include <vector>
#include <utility>
#include <cstring>
#include <cassert>
#include <stdint.h>

namespace p1 {
namespace smalls {

class Scope;

struct Mark
{
  int32_t const value; // -1 means anti-mark
  Scope * const scope;
  Mark * const next;

  Mark ( int32_t value_, Scope * scope_, Mark * next_ )
    : value(value_), scope(scope_), next( next_ )
  {
    assert( value < 0 || scope != NULL );
  }

  bool isAntiMark () const { return value < 0; };
  bool isMark () const { return value > 0; };

  bool equal ( const Mark * m ) const;
  void toStream ( std::ostream & os ) const;
};

inline std::ostream & operator << ( std::ostream & os, const Mark & m )
{
  m.toStream( os );
  return os;
}

Mark * concat ( Mark * first, Mark * second );
bool equal ( const Mark * a, const Mark * b );

#define _DEF_SYNTAX_CLASSES \
   _MK_ENUM(NONE) \
   _MK_ENUM(DEOF) \
   _MK_ENUM(COMMENT) \
   _MK_ENUM(REAL) \
   _MK_ENUM(INTEGER) \
   _MK_ENUM(BOOL) \
   _MK_ENUM(STR) \
   _MK_ENUM(SYMBOL) \
   _MK_ENUM(BINDING) \
   _MK_ENUM(PAIR) \
   _MK_ENUM(NIL) \
   _MK_ENUM(VECTOR) \


struct SyntaxKind
{
  #define _MK_ENUM(name)  name,
  enum Enum
  {
    _DEF_SYNTAX_CLASSES
  };
  #undef _MK_ENUM

  static const char * name ( Enum x ) { return s_names[x]; }

private:
  static const char * s_names[];
};

class Syntax : public gc
{
public:
  SyntaxKind::Enum const skind;
  SourceCoords coords;
  Syntax ( SyntaxKind::Enum skind_, const SourceCoords & coords_ ) : skind( skind_ ), coords( coords_ ) {}

  bool isNil () const { return this->skind == SyntaxKind::NIL; }

  virtual Syntax * wrap ( Mark * mark );

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;

  static void toStreamIndented ( std::ostream & os, unsigned indent, const Syntax * dat );
};

Syntax * unwrapCompletely ( Syntax * syntax, Mark * mark = NULL );

inline std::ostream & operator << ( std::ostream & os, const Syntax & dat )
{
  dat.toStream(os);
  return os;
}

class Symbol;
class Binding;

class SyntaxValue : public Syntax
{
public:
  union
  {
    double real;
    int64_t integer;
    bool     vbool;
    const gc_char * str;
  } u;

  SyntaxValue ( SyntaxKind::Enum skind_, const SourceCoords & coords_, double real )
    : Syntax( skind_, coords_ ) { assert(skind == SyntaxKind::REAL); u.real = real; }
  SyntaxValue ( SyntaxKind::Enum skind_, const SourceCoords & coords_, int64_t integer )
    : Syntax( skind_, coords_ ) { assert(skind == SyntaxKind::INTEGER); u.integer = integer; }
  SyntaxValue ( SyntaxKind::Enum skind_, const SourceCoords & coords_, bool vbool )
    : Syntax( skind_, coords_ ) { assert(skind == SyntaxKind::BOOL); u.vbool = vbool; }
  SyntaxValue ( SyntaxKind::Enum skind_, const SourceCoords & coords_, const gc_char * str )
    : Syntax( skind_, coords_ ) { assert(skind == SyntaxKind::STR); u.str = str; }

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

class SyntaxSymbol : public Syntax
{
public:
  Symbol * const symbol;
  Mark * const mark;

  SyntaxSymbol ( const SourceCoords & coords_, Symbol * symbol_, Mark * mark_ = NULL )
    : Syntax( SyntaxKind::SYMBOL, coords_ ), symbol(symbol_), mark(mark_)
  {}

  virtual Syntax * wrap ( Mark * mark );

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

class SyntaxBinding : public Syntax
{
public:
  Binding * bnd;

  SyntaxBinding ( const SourceCoords & coords_, Binding * bnd )
    : Syntax( SyntaxKind::BINDING, coords_ ) { this->bnd = bnd; }

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

class SyntaxPair : public Syntax
{
  mutable Syntax * m_wrappedCar;
  mutable Syntax * m_wrappedCdr;
public:
  Syntax * m_car, * m_cdr;
  Mark * const mark;

  SyntaxPair ( const SourceCoords & coords_, Syntax * car_, Syntax * cdr_, Mark * mark_ = NULL )
    : Syntax( SyntaxKind::PAIR, coords_ ), m_car(car_), m_cdr(cdr_), mark(mark_)
  {
    m_wrappedCar = NULL;
    m_wrappedCdr = NULL;
  }

  Syntax* car() const;

  Syntax* cdr() const;

  void setCar(Syntax* car)
  {
    m_car = car;
    m_wrappedCar = NULL;
  }

  void setCdr(Syntax* cdr)
  {
    m_cdr = cdr;
    m_wrappedCdr = NULL;
  }

  virtual Syntax * wrap ( Mark * mark );

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;

protected:
  SyntaxPair ( SyntaxKind::Enum sclass, const SourceCoords & coords_, Syntax * car_, Syntax * cdr_ )
    : Syntax( sclass, coords_ ), m_car(car_), m_cdr(cdr_), mark(NULL)
  {
    m_wrappedCar = NULL;
    m_wrappedCdr = NULL;
  }
};

struct SyntaxNil : public SyntaxPair
{
  SyntaxNil ( const SourceCoords & coords )
    : SyntaxPair( SyntaxKind::NIL, coords, NULL, NULL ) {}

  virtual Syntax * wrap ( Mark * mark );
  virtual bool equal ( const Syntax * x ) const;
  virtual void toStream ( std::ostream & os ) const;
};

class SyntaxVector : public Syntax
{
  mutable Syntax ** m_wrappedData;
public:
  Syntax ** const m_data;
  unsigned const len;
  Mark * const mark;

  SyntaxVector ( const SourceCoords & coords_, Syntax ** data_, unsigned len_, Mark * mark_ = NULL )
    : Syntax( SyntaxKind::VECTOR, coords ), m_data( data_ ), len( len_ ), mark(mark_) {}

  virtual Syntax * wrap ( Mark * mark );
  Syntax* getElement( unsigned i ) const;

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

namespace detail {

struct StackEntry
{
  int state;
  union
  {
    struct // state=-1, -2
    {
      SyntaxPair * pair;
      Syntax * car;
    };
    struct // state=0..
    {
      SyntaxVector * vec;
      Syntax ** data;
    };
  };

  StackEntry ( int state, SyntaxPair * pair )
  {
    this->state = state;
    this->pair = pair;
    this->car = NULL;
  }
  StackEntry ( int state, SyntaxVector * vec )
  {
    this->state = state;
    this->vec = vec;
    this->data = NULL;
  }
};

inline Syntax * syntaxVisitorNOP ( Syntax * datum )
{
  return NULL;
}

/**
 * Non-recursively visit all datums, potentially non-destructively modifying them.
 * @param datum
 * @param f
 * @return
 */
template <typename PRE, typename POST>
Syntax * syntaxVisitAll ( Syntax * datum, const PRE & pre, const POST & post )
{
  std::vector<StackEntry, gc_allocator<StackEntry> > stack;

recur:
  datum = pre( datum );

  if (datum->skind == SyntaxKind::PAIR)
  {
    SyntaxPair * pair = static_cast<SyntaxPair*>(datum);
    stack.push_back( StackEntry(-1,pair) );
    datum = pair->m_car;
    goto recur;
  }
  else if (datum->skind == SyntaxKind::VECTOR)
  {
    SyntaxVector * vec = static_cast<SyntaxVector*>(datum);
    if (vec->len > 0)
    {
      stack.push_back( StackEntry(0,vec) );
      datum = vec->m_data[0];
      goto recur;
    }
  }

leave:
  datum = post( datum );

  if (stack.empty())
    return datum;

  StackEntry & st = stack.back();
  if (st.state == -1) // pair: processing car
  {
    st.state = -2;
    st.car = datum;
    datum = st.pair->m_cdr;
    goto recur;
  }
  else if (st.state == -2) // pair: processing cdr
  {
    if (st.car != st.pair->m_car || datum != st.pair->m_cdr)
      datum = new SyntaxPair( st.pair->coords, st.car, datum );
    else
      datum = st.pair;
  }
  else // vector
  {
    if (datum != st.vec->m_data[st.state] && !st.data)
    {
      // The first time we must allocate the new data and copy all previous values
      st.data = new (GC) Syntax*[st.vec->len];
      std::memcpy( st.data, st.vec->m_data, st.state * sizeof(st.data[0]) );
    }

    if (st.data)
      st.data[st.state] = datum;

    ++st.state;
    if (st.state < st.vec->len)
    {
      datum = st.vec->m_data[st.state];
      goto recur;
    }

    if (st.data) // Did any of the vector elements change?
      datum = new SyntaxVector( st.vec->coords, st.data, st.vec->len );
    else
      datum = st.vec;
  }

  stack.pop_back();
  goto leave;
}

} // namespace detail

using detail::syntaxVisitAll;
using detail::syntaxVisitorNOP;

}} // namespaces

#endif	/* P1_SMALLS_PARSER_SYNTAX_HPP */

