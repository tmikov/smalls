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
#include "SchemeAST.hpp"
#include "p1/smalls/parser/Syntax.hpp" // FIXME: should not depend on this
#include "p1/util/OStreamIndent.hpp"
#include <boost/foreach.hpp>
#include <iostream>
#include <iomanip>

namespace p1 {
namespace smalls {
namespace ast {

#define _MK_ENUM(x) #x,
const char * AstKind::s_names[] =
{
  _DEF_AST_CODES
};
#undef _MK_ENUM

std::ostream & printListOfAst ( std::ostream & os, const ListOfAst & lst, const char * tag )
{
  if (lst.empty())
    return os;

  if (!lst.next(lst.first()))
    return os << *lst.first();

  if (tag)
    os << "(" << tag << OStreamSetIndent(+4);

  for ( ListOfAst::const_iterator ast = lst.begin(); ast != lst.end(); ++ast )
    os << '\n' << OStreamIndent() << *ast;

  if (tag)
  {
    os << ")";
    os << OStreamSetIndent(-4);
  }

  return os;
}

std::ostream & operator<< ( std::ostream & os, const ListOfAst & lst )
{
  return printListOfAst( os, lst, "begin" );
}

void Ast::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << ')';
}

void AstVar::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << ' ' << *this->var << ')';
}

void AstDatum::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << ' ';
  this->datum->toStream( os );
  os << ')';
}

void AstSet::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << '\n'
     << OStreamSetIndent(+4)
     << OStreamIndent() << *this->target << '\n'
     << OStreamIndent() << *this->rvalue << ')'
     << OStreamSetIndent(-4);
}

void AstApply::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << ' ' << *this->target << ' ';

  BOOST_FOREACH( Ast * ast, *this->params )
    os << *ast << ' ';

  if (this->listParam)
    os << *this->listParam;
  else
    os << "'()";

  os << ")";
}

void AstIf::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << '\n'
     << OStreamSetIndent(+4)
     << OStreamIndent() << *this->cond << '\n'
     << OStreamIndent() << *this->thenAst;
  if (this->elseAst)
  {
    os << '\n'
       << OStreamIndent() << *this->elseAst;
  }

  os << ')'
     << OStreamSetIndent(-4);
}

void AstBegin::toStream ( std::ostream & os ) const
{
  os << m_exprList;
}

void AstBody::toStream ( std::ostream & os ) const
{
  if (m_defs.empty())
  {
    printListOfAst( os, m_exprList, "BODY");
    return;
  }

  os << "(LETREC*\n"
     << OStreamSetIndent(+4)
     << OStreamIndent() << "("
     << OStreamSetIndent(+4);

  BOOST_FOREACH( const AstBody::Definition & defn, m_defs )
  {
    os << '\n'
       << OStreamIndent() << '(';

    if (defn.first)
      os << *defn.first;
    else
      os << "<unused>";
    os << " ";
    os << *defn.second;

    os << ')';
  }

  os << ')'
     << OStreamSetIndent(-8)
     << "\n"
     << OStreamSetIndent(+4)
     << OStreamIndent();
  printListOfAst( os, m_exprList, NULL );
  os << OStreamSetIndent(-4)
     << ')';
}

void AstClosure::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << ' ';

  os << '(';
  unsigned c = 0;
  BOOST_FOREACH( ast::Variable * var, *this->params )
  {
    if (c++ > 0)
      os << ' ';
    os << *var;
  }
  if (this->listParam)
  {
    os << " . " << *listParam;
  }
  os << ')' << '\n'
     << OStreamSetIndent(+4)
     << OStreamIndent() << *this->body
     << ')'
     << OStreamSetIndent(-4);
}

void AstLet::toStream ( std::ostream & os ) const
{
  os << '(' << AstKind::name( this->kind ) << OStreamSetIndent(+4);

  os << '\n'<< OStreamIndent() << '(' << OStreamSetIndent(+4);

  for ( unsigned i = 0, end = params->size(); i != end; ++i )
  {
    os << '\n' << OStreamIndent() << '(' << *(*params)[i] << ' ' << *(*values)[i] << ')';
  }
  os << ')' << OStreamSetIndent(-4);

  os << '\n'
     << OStreamIndent() << *this->body
     << ')'
     << OStreamSetIndent(-4);
}

AstFix::AstFix (
  const SourceCoords & coords_,
  ast::Frame * paramFrame,
  VectorOfVariable * params,
  AstBody * body_,
  VectorOfAst * values
) : AstLet(
      AstKind::FIX,
      coords_,
      paramFrame,
      params,
      body_,
      values
    )
{
#ifndef NDEBUG
  BOOST_FOREACH( Ast * init, *values )
  {
    assert( init && init->kind == AstKind::CLOSURE );
  }
#endif
}

std::ostream & operator<< ( std::ostream & os, AstModule & mod )
{
  return os << *mod.body();
}

}}} // namespaces

