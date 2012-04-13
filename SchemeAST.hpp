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


#ifndef SCHEMEAST_HPP
#define	SCHEMEAST_HPP

#include <vector>

#include "base.hpp"
#include "LinkedList.hpp"

#include "SourceCoords.hpp"
#include "Frame.hpp"

#define _DEF_AST_CODES \
  _MK_ENUM(NONE) \
  _MK_ENUM(UNSPECIFIED) \
  _MK_ENUM(VAR) \
  _MK_ENUM(DATUM) \
  _MK_ENUM(SET) \
  _MK_ENUM(APPLY) \
  _MK_ENUM(IF) \
  _MK_ENUM(CLOSURE) \
  _MK_ENUM(LET) \
  _MK_ENUM(FIX) \


struct AstCode
{
  #define _MK_ENUM(x)  x,
  enum Enum
  {
    _DEF_AST_CODES
  };
  #undef _MK_ENUM

  static const char * name ( Enum x )  { return s_names[x]; }
private:
  static const char * s_names[];
};

struct Ast : public gc
{
  AstCode::Enum const code;
  SourceCoords coords;
  Ast * prev, * next;

  Ast ( AstCode::Enum code_, const SourceCoords & coords_ ) : code(code_), coords(coords_)
  {
    this->prev = this->next = NULL;
  };

  virtual void toStream ( std::ostream & os ) const;
};

typedef LinkedList<Ast> ListOfAst;

inline ListOfAst makeListOfAst ( Ast * ast )
{
  return ListOfAst( ast );
}

typedef std::vector<ListOfAst, gc_allocator<ListOfAst> > VectorOfListOfAst;

struct OStreamSetIndent
{
  int const x;
  OStreamSetIndent ( int x_ ) : x(x_) {};

  static int s_indent_idx;

  static int indent ( std::ostream & os );
  static int indent ( std::ostream & os, int add );
};

inline std::ostream & operator << ( std::ostream & os, OStreamSetIndent si )
{
  OStreamSetIndent::indent( os, si.x );
  return os;
}

struct OStreamIndent {};

inline std::ostream & operator << ( std::ostream & os, OStreamIndent w )
{
  os.width( OStreamSetIndent::indent(os) );
  os << "";
  return os;
}

std::ostream & operator << ( std::ostream & os, const ListOfAst & lst );

inline std::ostream & operator << ( std::ostream & os, const Ast & ast )
{
  ast.toStream( os );
  return os;
}

struct AstVar : public Ast
{
  Variable * var;

  AstVar ( const SourceCoords & coords_, Variable * var ) : Ast( AstCode::VAR, coords_ )
  {
    this->var = var;
  }

  virtual void toStream ( std::ostream & os ) const;
};

class Syntax;

struct AstDatum : public Ast
{
  Syntax * datum;

  AstDatum ( const SourceCoords & coords_, Syntax * datum ) : Ast ( AstCode::DATUM, coords_ )
  {
    this->datum = datum;
  }

  virtual void toStream ( std::ostream & os ) const;
};

struct AstSet : public Ast
{
  Variable * target;
  ListOfAst rvalue;

  AstSet ( const SourceCoords & coords_, Variable * target, const ListOfAst & rvalue )
    : Ast( AstCode::SET, coords_ )
  {
    this->target = target;
    this->rvalue = rvalue;
  }

  virtual void toStream ( std::ostream & os ) const;
};

struct AstApply : public Ast
{
  ListOfAst target;
  VectorOfListOfAst * params;
  ListOfAst * listParam;

  AstApply ( const SourceCoords & coords_,
             const ListOfAst & target, VectorOfListOfAst * params, ListOfAst * listParam )
    : Ast( AstCode::APPLY, coords_ )
  {
    this->target = target;
    this->params = params;
    this->listParam = listParam;
  }

  virtual void toStream ( std::ostream & os ) const;
};

struct AstIf : public Ast
{
  ListOfAst cond;
  ListOfAst thenAst;
  ListOfAst elseAst;

  AstIf ( const SourceCoords & coords_,
          const ListOfAst & cond, const ListOfAst & thenAst, const ListOfAst & elseAst )
    : Ast( AstCode::IF, coords_ )
  {
    this->cond = cond;
    this->thenAst = thenAst;
    this->elseAst = elseAst;
  }

  virtual void toStream ( std::ostream & os ) const;
};

typedef std::vector<Variable *, gc_allocator<Variable *> > VectorOfVariable;

struct AstClosure : public Ast
{
  Frame * enclosingFrame;
  VectorOfVariable * params;
  Variable * listParam;
  Frame * paramFrame;
  Frame * bodyFrame;
  ListOfAst body;

  AstClosure (
    const SourceCoords & coords_,
    Frame * enclosingFrame,
    VectorOfVariable * params,
    Variable * listParam,
    Frame * paramFrame,
    Frame * bodyFrame,
    const ListOfAst & body
  ) : Ast( AstCode::CLOSURE, coords_ )
  {
    this->enclosingFrame = enclosingFrame;
    this->params = params;
    this->listParam = listParam;
    this->paramFrame = paramFrame;
    this->bodyFrame = bodyFrame;
    this->body = body;
  }

  virtual void toStream ( std::ostream & os ) const;
};

struct AstLet : public Ast
{
  Frame * enclosingFrame;
  VectorOfVariable * params;
  Frame * paramFrame;
  Frame * bodyFrame;
  ListOfAst body;
  VectorOfListOfAst * values;

  AstLet (
    const SourceCoords & coords_,
    Frame * enclosingFrame,
    VectorOfVariable * params,
    Frame * paramFrame,
    Frame * bodyFrame,
    const ListOfAst & body,
    VectorOfListOfAst * values
  ) : Ast( AstCode::LET, coords_ )
  {
    this->enclosingFrame = enclosingFrame;
    this->params = params;
    this->paramFrame = paramFrame;
    this->bodyFrame = bodyFrame;
    this->body = body;
    this->values = values;
  }

  virtual void toStream ( std::ostream & os ) const;

protected:
  AstLet (
    AstCode::Enum code,
    const SourceCoords & coords_,
    Frame * enclosingFrame,
    VectorOfVariable * params,
    Frame * paramFrame,
    Frame * bodyFrame,
    const ListOfAst & body,
    VectorOfListOfAst * values
  ) : Ast( code, coords_ )
  {
    this->enclosingFrame = enclosingFrame;
    this->params = params;
    this->paramFrame = paramFrame;
    this->bodyFrame = bodyFrame;
    this->body = body;
    this->values = values;
  }
};

/**
 * A restricted form of let where the variables are unassigned (never modified), and the init
 * expressions are all lambdas.
 */
struct AstFix : public AstLet
{
  AstFix (
    const SourceCoords & coords_,
    Frame * enclosingFrame,
    VectorOfVariable * params,
    Frame * paramFrame,
    Frame * bodyFrame,
    const ListOfAst & body,
    VectorOfListOfAst * values
  );
};

#endif	/* SCHEMEAST_HPP */

