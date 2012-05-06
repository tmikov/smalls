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

#define _MK_ENUM(x) #x,
const char * AstCode::s_names[] =
{
  _DEF_AST_CODES
};
#undef _MK_ENUM

std::ostream & operator<< ( std::ostream & os, const ListOfAst & lst )
{
  if (lst.empty())
    return os << "()";

  if (!lst.next(lst.first()))
    return os << *lst.first();

  os << "(begin" << OStreamSetIndent(+4);

  for ( ListOfAst::const_iterator ast = lst.begin(); ast != lst.end(); ++ast )
  {
    os << '\n' << OStreamIndent() << *ast;
  }
  os << ")";

  os << OStreamSetIndent(-4);

  return os;
}

void Ast::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << ')';
}

void AstVar::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << ' ' << *this->var << ')';
}

void AstDatum::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << ' ';
  this->datum->toStream( os );
  os << ')';
}

void AstSet::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << '\n'
     << OStreamSetIndent(+4)
     << OStreamIndent() << *this->target << '\n'
     << OStreamIndent() << this->rvalue << ')'
     << OStreamSetIndent(-4);
}

void AstApply::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << ' ' << this->target << ' ';

  BOOST_FOREACH( ListOfAst & lst, *this->params )
    os << lst << ' ';

  if (this->listParam)
    os << *this->listParam;
  else
    os << "'()";

  os << ")";
}

void AstIf::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << '\n'
     << OStreamSetIndent(+4)
     << OStreamIndent() << this->cond << '\n'
     << OStreamIndent() << this->thenAst << '\n'
     << OStreamIndent() << this->elseAst << ')'
     << OStreamSetIndent(-4);
}

void AstClosure::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << ' ';

  os << '(';
  unsigned c = 0;
  BOOST_FOREACH( Variable * var, *this->params )
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
     << OStreamIndent() << this->body
     << ')'
     << OStreamSetIndent(-4);
}

void AstLet::toStream ( std::ostream & os ) const
{
  os << '(' << AstCode::name( this->code ) << OStreamSetIndent(+4);

  os << '\n'<< OStreamIndent() << '(' << OStreamSetIndent(+4);

  for ( unsigned i = 0, end = params->size(); i != end; ++i )
  {
    os << '\n' << OStreamIndent() << '(' << *(*params)[i] << ' ' << (*values)[i] << ')';
  }
  os << ')' << OStreamSetIndent(-4);

  os << '\n'
     << OStreamIndent() << this->body
     << ')'
     << OStreamSetIndent(-4);
}

AstFix::AstFix (
  const SourceCoords & coords_,
  Frame * enclosingFrame,
  VectorOfVariable * params,
  Frame * paramFrame,
  Frame * bodyFrame,
  const ListOfAst & body,
  VectorOfListOfAst * values
) : AstLet(
      AstCode::FIX,
      coords_,
      enclosingFrame,
      params,
      paramFrame,
      bodyFrame,
      body,
      values
    )
{
#ifndef NDEBUG
  BOOST_FOREACH( ListOfAst & init, *values )
  {
    assert( !init.empty() );
    assert( init.first()->code == AstCode::CLOSURE );
  }
#endif
}

}} // namespaces

