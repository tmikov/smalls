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
#include "p1/util/casting.hpp"
#include <vector>
#include <list>

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
  _MK_ENUM(BEGIN) \
  _MK_ENUM(BODY) \
  _MK_ENUM(CLOSURE) \
  _MK_ENUM(LET) \
  _MK_ENUM(FIX) \


struct AstKind
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

class Ast : public gc
{
public:
  AstKind::Enum const kind;
  SourceCoords const coords;

  Ast ( AstKind::Enum kind_, const SourceCoords & coords_ ) : kind(kind_), coords(coords_)
  {
    this->m_prev = this->m_next = NULL;
  };

  static bool classof ( const Ast * ) { return true; }

  virtual void toStream ( std::ostream & os ) const;

  struct LinkAccessor
  {
    static Ast * & prev ( Ast * item ) { return item->m_prev; }
    static Ast * & next ( Ast * item ) { return item->m_next; }
    static Ast * prev ( const Ast * item ) { return item->m_prev; }
    static Ast * next ( const Ast * item ) { return item->m_next; }
  };

private:
  Ast * m_prev, * m_next;
};

inline std::ostream & operator << ( std::ostream & os, const Ast & ast )
{
  ast.toStream( os );
  return os;
}

typedef p1::LinkedList<Ast,Ast::LinkAccessor> ListOfAst;

std::ostream & printListOfAst ( std::ostream & os, const ListOfAst & lst, const char * tag );
std::ostream & operator<< ( std::ostream & os, const ListOfAst & lst );

typedef std::vector<Ast*, gc_allocator<Ast*> > VectorOfAst;

class AstVar : public Ast
{
public:
  AstVariable * const var;

  AstVar ( const SourceCoords & coords_, AstVariable * var_ )
    : Ast( AstKind::VAR, coords_ ), var( var_ )
  {}

  static bool classof ( const AstVar * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::VAR; }

  virtual void toStream ( std::ostream & os ) const;
};

class AstDatum : public Ast
{
public:
  Syntax * const datum; // FIXME: should not depend on this

  AstDatum ( const SourceCoords & coords_, Syntax * datum_ )
    : Ast ( AstKind::DATUM, coords_ ), datum( datum_ )
  {}

  static bool classof ( const AstDatum * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::DATUM; }

  virtual void toStream ( std::ostream & os ) const;
};

class AstSet : public Ast
{
public:
  AstVariable * const target;
  Ast * const rvalue;

  AstSet ( const SourceCoords & coords_, AstVariable * target_, Ast * rvalue_ )
    : Ast( AstKind::SET, coords_ ), target(target_), rvalue(rvalue_)
  {}

  static bool classof ( const AstSet * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::SET; }

  virtual void toStream ( std::ostream & os ) const;
};

class AstApply : public Ast
{
public:
  Ast * const target;
  VectorOfAst * const params;
  Ast * const listParam;

  AstApply ( const SourceCoords & coords_,
             Ast * target_, VectorOfAst * params_, Ast * listParam_ )
    : Ast( AstKind::APPLY, coords_ ), target(target_), params(params_), listParam(listParam_)
  {}

  static bool classof ( const AstApply * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::APPLY; }

  virtual void toStream ( std::ostream & os ) const;
};

class AstIf : public Ast
{
public:
  Ast * const cond;
  Ast * const thenAst;
  Ast * const elseAst;

  AstIf ( const SourceCoords & coords_,
          Ast * cond_, Ast * thenAst_, Ast * elseAst_ )
    : Ast( AstKind::IF, coords_ ), cond(cond_),thenAst(thenAst_),elseAst(elseAst_)
  {}

  static bool classof ( const AstIf * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::IF; }

  virtual void toStream ( std::ostream & os ) const;
};

class AstBegin : public Ast
{
public:
  AstBegin ( const SourceCoords & coords_ )
    : Ast( AstKind::BEGIN, coords_ )
  {}

  static bool classof ( const AstBegin * ) { return true; }
  static bool classof ( const Ast * t )   { return t->kind == AstKind::BEGIN; }

  ListOfAst & exprList () { return m_exprList; }

  virtual void toStream ( std::ostream & os ) const;

protected:
  ListOfAst m_exprList;

  AstBegin ( AstKind::Enum kind_, const SourceCoords & coords_ )
    : Ast( kind_, coords_ )
  {}

};

class AstBody : public AstBegin
{
public:
  typedef std::pair<AstVariable *, Ast *> Definition;
  typedef std::list<Definition,gc_allocator<Definition> > DefinitionList;

  AstBody ( const SourceCoords & coords_, AstFrame * frame )
    : AstBegin( AstKind::BODY, coords_ ), m_frame(frame)
  {}

  static bool classof ( const AstBody * ) { return true; }
  static bool classof ( const Ast * t )   { return t->kind == AstKind::BODY; }

  DefinitionList & defs () { return m_defs; }

  AstFrame * frame () { return m_frame; }
  void setFrame ( AstFrame * frame ) { m_frame = frame; }

  virtual void toStream ( std::ostream & os ) const;

private:
  DefinitionList m_defs;
  AstFrame * m_frame;
};

typedef std::vector<AstVariable *, gc_allocator<AstVariable *> > VectorOfVariable;

class AstClosure : public Ast
{
public:
  AstFrame * const paramFrame;
  VectorOfVariable * const params;
  AstVariable * const listParam;
  AstBody * const body;

  AstClosure (
    const SourceCoords & coords_,
    AstFrame * paramFrame_,
    VectorOfVariable * params_,
    AstVariable * listParam_,
    AstBody * body_
  ) : Ast( AstKind::CLOSURE, coords_ ),
      paramFrame( paramFrame_ ),
      params( params_ ),
      listParam( listParam_ ),
      body( body_ )
  {}

  static bool classof ( const AstClosure * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::CLOSURE; }

  virtual void toStream ( std::ostream & os ) const;
};

class AstLet : public Ast
{
public:
  AstFrame * paramFrame;
  VectorOfVariable * params;
  AstBody * const body;
  VectorOfAst * values;

  AstLet (
    const SourceCoords & coords_,
    AstFrame * paramFrame_,
    VectorOfVariable * params_,
    AstBody * body_,
    VectorOfAst * values_
  ) : Ast( AstKind::LET, coords_ ),
      paramFrame( paramFrame_ ),
      params( params_ ),
      body( body_ ),
      values( values_ )
  {}

  static bool classof ( const AstLet * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::LET; }

  virtual void toStream ( std::ostream & os ) const;

protected:
  AstLet (
    AstKind::Enum code,
    const SourceCoords & coords_,
    AstFrame * paramFrame_,
    VectorOfVariable * params_,
    AstBody * body_,
    VectorOfAst * values_
  ) : Ast( code, coords_ ),
      paramFrame( paramFrame_ ),
      params( params_ ),
      body( body_ ),
      values( values_ )
  {}
};

/**
 * A restricted form of let where the variables are unassigned (never modified), and the init
 * expressions are all lambdas. This is a synthetic construct - it is never constructed directly
 * from source but is re-constructed from letrec.
 */
class AstFix : public AstLet
{
public:
  AstFix (
    const SourceCoords & coords_,
    AstFrame * paramFrame_,
    VectorOfVariable * params_,
    AstBody * body_,
    VectorOfAst * values_
  );

  static bool classof ( const AstFix * ) { return true; }
  static bool classof ( const Ast * t ) { return t->kind == AstKind::FIX; }
};

class AstModule : public gc
{
public:
  AstModule ( AstBody * body )
    : m_body( body )
  {}
  AstModule () {}

  AstBody * body () { return m_body; }
  void setBody ( AstBody * body ) { m_body = body; }

private:
  AstBody *  m_body;
};

std::ostream & operator<< ( std::ostream & os, AstModule & mod );

}} // namespaces

#endif	/* P1_SMALLS_AST_SCHEMEAST_HPP */

