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


#ifndef SYNTAX_HPP
#define	SYNTAX_HPP

#include "base.hpp"
#include "AbstractErrorReporter.hpp"

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
   _MK_ENUM(VECTOR)

struct SyntaxClass
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
  SyntaxClass::Enum const sclass;
  SourceCoords coords;
  Syntax ( SyntaxClass::Enum dclass_, const SourceCoords & coords_ ) : sclass( dclass_ ), coords( coords_ ) {}

  bool isNil () const { return this->sclass == SyntaxClass::NIL; }
  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;

  static void toStreamIndented ( std::ostream & os, unsigned indent, const Syntax * dat );
};

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
    Symbol * symbol;
  } u;

  SyntaxValue ( SyntaxClass::Enum dclass_, const SourceCoords & coords_, double real )
    : Syntax( dclass_, coords_ ) { u.real = real; }
  SyntaxValue ( SyntaxClass::Enum dclass_, const SourceCoords & coords_, int64_t integer )
    : Syntax( dclass_, coords_ ) { u.integer = integer; }
  SyntaxValue ( SyntaxClass::Enum dclass_, const SourceCoords & coords_, bool vbool )
    : Syntax( dclass_, coords_ ) { u.vbool = vbool; }
  SyntaxValue ( SyntaxClass::Enum dclass_, const SourceCoords & coords_, const gc_char * str )
    : Syntax( dclass_, coords_ ) { u.str = str; }
  SyntaxValue ( SyntaxClass::Enum dclass_, const SourceCoords & coords_, Symbol * symbol )
    : Syntax( dclass_, coords_ ) { u.symbol = symbol; }

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

class SyntaxBinding : public Syntax
{
public:
  Binding * bnd;

  SyntaxBinding ( const SourceCoords & coords_, Binding * bnd )
    : Syntax( SyntaxClass::BINDING, coords_ ) { this->bnd = bnd; }

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

struct SyntaxPair : public Syntax
{
  Syntax * car, * cdr;

  SyntaxPair ( const SourceCoords & coords_, Syntax * car_, Syntax * cdr_ )
    : Syntax( SyntaxClass::PAIR, coords_ ), car(car_), cdr(cdr_) {}

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;

protected:
  SyntaxPair ( SyntaxClass::Enum sclass, const SourceCoords & coords_, Syntax * car_, Syntax * cdr_ )
    : Syntax( sclass, coords_ ), car(car_), cdr(cdr_) {}
};

struct SyntaxNil : public SyntaxPair
{
  SyntaxNil ( const SourceCoords & coords )
    : SyntaxPair( SyntaxClass::NIL, coords, NULL, NULL ) {}

  virtual bool equal ( const Syntax * x ) const;
  virtual void toStream ( std::ostream & os ) const;
};

struct SyntaxVector : public Syntax
{
  Syntax ** data;
  unsigned len;

  SyntaxVector ( const SourceCoords & coords_, Syntax ** data_, unsigned len_ )
    : Syntax( SyntaxClass::VECTOR, coords ), data( data_ ), len( len_ ) {}

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

#include <vector>
#include <utility>

/**
 * Non-recursively visit all datums in DFS order, potentially non-destructively modifying them.
 * @param datum
 * @param f
 * @return
 */
template <typename F>
Syntax * syntaxVisitAllDFS ( Syntax * datum, const F & f )
{
  // NOTE: we process the lists non-recursively
  // StackEntry::first is the car of the SyntaxPair we are processing
  // StackEntry::second is the SyntaxPair itself
  typedef std::pair<Syntax*,SyntaxPair*> StackEntry;
  std::vector<StackEntry, gc_allocator<StackEntry> > stack;

recur:
  datum = f( datum );

  if (datum->sclass == SyntaxClass::PAIR)
  {
    SyntaxPair * pair = static_cast<SyntaxPair*>(datum);
    stack.push_back( StackEntry(NULL,pair) );
    datum = pair->car;
    goto recur;
  }

leave:
  if (stack.empty())
    return datum;

  StackEntry & st = stack.back();
  if (!st.first)
  {
    st.first = datum;
    datum = st.second->cdr;
    goto recur;
  }
  else
  {
    if (st.first != st.second->car || datum != st.second->cdr)
      datum = new SyntaxPair( st.second->coords, st.first, datum );
    else
      datum = st.second;
    stack.pop_back();
    goto leave;
  }
}

#endif	/* SYNTAX_HPP */

