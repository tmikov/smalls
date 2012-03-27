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


#include "Syntax.hpp"
#include "SymbolTable.hpp"

#define _MK_ENUM(x) #x,
const char * SyntaxClass::s_names[] =
{
  _DEF_SYNTAX_CLASSES
};
#undef _MK_ENUM

void Syntax::toStream ( std::ostream & os ) const
{
  os << SyntaxClass::name(sclass);
}

bool Syntax::equal ( const Syntax * x ) const
{
  return sclass == x->sclass;
}

void SyntaxValue::toStream ( std::ostream & os ) const
{
  switch (sclass)
  {
  case SyntaxClass::REAL:    os << u.real; break;
  case SyntaxClass::INTEGER: os << u.integer; break;
  case SyntaxClass::BOOL:    os << u.vbool; break;
  case SyntaxClass::STR:     os << "\"" << u.str << "\""; break; // FIXME: utf-8 decoding & escaping!
  case SyntaxClass::SYMBOL:  os << u.symbol->name; break;
  case SyntaxClass::BINDING:  os << "bnd/" << u.bnd->sym->name; break;
  default:
    assert( false );
  }
}

bool SyntaxValue::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (sclass != x->sclass)
    return false;
  const SyntaxValue * p = (const SyntaxValue *)x;

  switch (sclass)
  {
  case SyntaxClass::REAL:    return u.real == p->u.real;
  case SyntaxClass::INTEGER: return u.integer == p->u.integer;
  case SyntaxClass::BOOL:    return u.vbool == p->u.vbool;
  case SyntaxClass::STR:     return std::strcmp( u.str, p->u.str) == 0;
  case SyntaxClass::SYMBOL:  return u.symbol == p->u.symbol;
  case SyntaxClass::BINDING:  return u.bnd == p->u.bnd;
  default:
    assert( false );
    return false;
  }
}

void SyntaxPair::toStream ( std::ostream & os ) const
{
  os << '(';
  const SyntaxPair * p = this;
  while (!p->isNil())
  {
    p->car->toStream( os );
    if (p->cdr->sclass == SyntaxClass::PAIR)
    {
      p = (const SyntaxPair *)p->cdr;
      os << " ";
    }
    else
    {
      os << " . " << *p->cdr;
      break;
    }
  }
  os << ')';
}

bool SyntaxPair::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (x->sclass != SyntaxClass::PAIR)
    return false;
  const SyntaxPair * p = (const SyntaxPair *)x;

  if (p->isNil())
    return false;

  return car->equal( p->car ) && cdr->equal( p->cdr );
}

bool SyntaxNil::equal ( const Syntax * x ) const
{
  return x->sclass == SyntaxClass::PAIR && static_cast<const SyntaxPair*>(x)->isNil();
}

void SyntaxVector::toStream ( std::ostream & os ) const
{
  os << "#(";

  for ( unsigned i = 0; i != len; ++i )
  {
    if (i != 0)
      os << " ";
    os << *data[i];
  }
  os << ")";
}

bool SyntaxVector::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (sclass != x->sclass)
    return false;
  const SyntaxVector * v = (const SyntaxVector *)x;
  if (len != v->len)
    return false;
  for ( unsigned i = 0; i != len; ++i )
    if (!data[i]->equal( v->data[i]))
      return false;
  return true;
}

void Syntax::toStreamIndented ( std::ostream & os, unsigned indent, const Syntax * datum )
{
  //os << datum->coords.line << ':' << datum->coords.column << ':';
  if (datum->isNil())
    os << "()";
  else if (datum->sclass == SyntaxClass::VECTOR)
  {
    const SyntaxVector * vec = (const SyntaxVector*)datum;
    os << "#(";
    for ( unsigned i = 0; i != vec->len; ++i )
    {
      if (i > 0)
        os << ' ';
      toStreamIndented( os, indent, vec->data[i] );
    }
    os << ')';
  }
  else if (datum->sclass == SyntaxClass::PAIR)
  {
    const SyntaxPair * p = (const SyntaxPair*)datum;
    os << '(';
    indent += 4;
    for(;;)
    {
      if (p != datum)
      {
        os << std::endl;
        for ( unsigned i = 0; i < indent; ++i )
          os << ' ';
      }

      toStreamIndented( os, indent, p->car);
      if (p->cdr->isNil())
        break;

      if (p->cdr->sclass == SyntaxClass::PAIR)
      {
        p = (const SyntaxPair *) p->cdr;
        os << ' ';
      }
      else
      {
        os << " . ";
        toStreamIndented( os, indent, p->cdr );
        break;
      }
    }
    os << ')';
    indent -= 4;
  }
  else
    os << *datum;
}

