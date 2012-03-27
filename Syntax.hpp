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
  SyntaxClass::Enum sclass;
  SourceCoords coords;
  Syntax ( SyntaxClass::Enum dclass_, const SourceCoords & coords_ ) : sclass( dclass_ ), coords( coords_ ) {}

  bool isNil () const;
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
    Binding * bnd;
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
  SyntaxValue ( SyntaxClass::Enum dclass_, const SourceCoords & coords_, Binding * bnd )
    : Syntax( dclass_, coords_ ) { u.bnd = bnd; }

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

struct SyntaxPair : public Syntax
{
  Syntax * car, * cdr;

  SyntaxPair ( const SourceCoords & coords_, Syntax * car_, Syntax * cdr_ )
    : Syntax( SyntaxClass::PAIR, coords_ ), car(car_), cdr(cdr_) {}

  bool isNil () const { return this->car == NULL; }

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

struct SyntaxNil : public SyntaxPair
{
  SyntaxNil ( const SourceCoords & coords )
    : SyntaxPair( coords, NULL, NULL ) {}

  virtual bool equal ( const Syntax * x ) const;
};

inline bool Syntax::isNil () const
{
  return this->sclass == SyntaxClass::PAIR && static_cast<const SyntaxPair*>(this)->isNil();
}

struct SyntaxVector : public Syntax
{
  Syntax ** data;
  unsigned len;

  SyntaxVector ( const SourceCoords & coords_, Syntax ** data_, unsigned len_ )
    : Syntax( SyntaxClass::VECTOR, coords ), data( data_ ), len( len_ ) {}

  virtual void toStream ( std::ostream & os ) const;
  virtual bool equal ( const Syntax * x ) const;
};

#endif	/* SYNTAX_HPP */

