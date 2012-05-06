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


#ifndef P1_SMALLS_AST_SCHEMEAST_HPP
#define	P1_SMALLS_AST_SCHEMEAST_HPP

#include "AstFrame.hpp"
#include "p1/smalls/common/SourceCoords.hpp"
#include "p1/adt/LinkedList.hpp"
#include <vector>

namespace p1 {
namespace smalls {
  class Syntax;
}}

namespace p1 {
namespace smalls {

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

typedef p1::LinkedList<Ast> ListOfAst;

inline ListOfAst makeListOfAst ( Ast * ast )
{
  return ListOfAst( ast );
}

typedef std::vector<ListOfAst, gc_allocator<ListOfAst> > VectorOfListOfAst;

std::ostream & operator<< ( std::ostream & os, const ListOfAst & lst );

inline std::ostream & operator << ( std::ostream & os, const Ast & ast )
{
  ast.toStream( os );
  return os;
}

struct AstVar : public Ast
{
  AstVariable * var;

  AstVar ( const SourceCoords & coords_, AstVariable * var ) : Ast( AstCode::VAR, coords_ )
  {
    this->var = var;
  }

  virtual void toStream ( std::ostream & os ) const;
};

struct AstDatum : public Ast
{
  Syntax * datum; // FIXME: should not depend on this

  AstDatum ( const SourceCoords & coords_, Syntax * datum ) : Ast ( AstCode::DATUM, coords_ )
  {
    this->datum = datum;
  }

  virtual void toStream ( std::ostream & os ) const;
};

struct AstSet : public Ast
{
  AstVariable * target;
  ListOfAst rvalue;

  AstSet ( const SourceCoords & coords_, AstVariable * target, const ListOfAst & rvalue )
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

typedef std::vector<AstVariable *, gc_allocator<AstVariable *> > VectorOfVariable;

struct AstClosure : public Ast
{
  AstFrame * enclosingFrame;
  VectorOfVariable * params;
  AstVariable * listParam;
  AstFrame * paramFrame;
  AstFrame * bodyFrame;
  ListOfAst body;

  AstClosure (
    const SourceCoords & coords_,
    AstFrame * enclosingFrame,
    VectorOfVariable * params,
    AstVariable * listParam,
    AstFrame * paramFrame,
    AstFrame * bodyFrame,
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
  AstFrame * enclosingFrame;
  VectorOfVariable * params;
  AstFrame * paramFrame;
  AstFrame * bodyFrame;
  ListOfAst body;
  VectorOfListOfAst * values;

  AstLet (
    const SourceCoords & coords_,
    AstFrame * enclosingFrame,
    VectorOfVariable * params,
    AstFrame * paramFrame,
    AstFrame * bodyFrame,
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
    AstFrame * enclosingFrame,
    VectorOfVariable * params,
    AstFrame * paramFrame,
    AstFrame * bodyFrame,
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
    AstFrame * enclosingFrame,
    VectorOfVariable * params,
    AstFrame * paramFrame,
    AstFrame * bodyFrame,
    const ListOfAst & body,
    VectorOfListOfAst * values
  );
};

}} // namespaces

#endif	/* P1_SMALLS_AST_SCHEMEAST_HPP */

