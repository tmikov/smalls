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
#include <boost/smart_ptr/scoped_array.hpp>

namespace p1 {
namespace smalls {

bool Mark::equal ( const Mark * m ) const
{
  if (m == this)
    return true;
  if (!m)
    return false;
  if (this->value != m->value)
    return false;
  if (this->next)
    return this->next->equal( m->next );
  return !m->next;
}

void Mark::toStream ( std::ostream & os ) const
{
  os << "Mark:" << this->value;
  for ( const Mark * n = this->next; n; n = n->next )
  if (this->next)
    os << "," << n->value;
}

Mark * concat ( Mark * first, Mark * second )
{
  if (!first)
    return second;
  if (!second)
    return first;

  Mark * next = concat( first->next, second );

  if (first->isMark() && next->isAntiMark()) // mark before anti-mark cancel each other
    return next->next;
  if (next != first->next)                 // did we change?
    return new Mark( first->value, first->scope, next );
  else
    return first;
}

bool equal ( const Mark * a, const Mark * b )
{
  if (a)
    return a->equal( b );
  return !b;
}


#define _MK_ENUM(x) #x,
const char * SyntaxKind::s_names[] =
{
  _DEF_SYNTAX_CLASSES
};
#undef _MK_ENUM

Syntax * Syntax::wrap ( Mark * mark )
{
  return this;
}

Syntax * unwrapCompletely ( Syntax * d, Mark * mark )
{
  if (SyntaxPair * p = dyn_cast<SyntaxPair>(d))
  {
    Mark * newMark = concat(mark,p->mark);
    Syntax * car = unwrapCompletely( p->m_car, newMark );
    Syntax * cdr = unwrapCompletely( p->m_cdr, newMark );
    if (car != p->m_car || cdr != p->m_cdr || p->mark != NULL)
      d = new SyntaxPair( p->coords, car, cdr, NULL );
  }
  else if (SyntaxVector * v = dyn_cast<SyntaxVector>(d))
  {
    Mark * newMark = concat(mark,v->mark);
    Syntax ** data = NULL;
    for ( unsigned i = 0; i != v->len; ++i )
    {
      Syntax * tmp = unwrapCompletely( v->m_data[i], newMark );
      if (tmp != v->m_data[i] && !data)
      {
        data = new (GC) Syntax*[v->len];
        std::memcpy( data, v->m_data, sizeof(data[0])*i );
      }
      if (data)
        data[i] = tmp;
    }
    if (data || v->mark != NULL)
      d = new SyntaxVector( v->coords, data, v->len, NULL );
  }
  else
    d = d->wrap(mark);

  return d;
}


void Syntax::toStream ( std::ostream & os ) const
{
  os << SyntaxKind::name(skind);
}

bool Syntax::equal ( const Syntax * x ) const
{
  return skind == x->skind;
}

void SyntaxValue::toStream ( std::ostream & os ) const
{
  switch (skind)
  {
  case SyntaxKind::REAL:    os << u.real; break;
  case SyntaxKind::INTEGER: os << u.integer; break;
  case SyntaxKind::BOOL:    os << u.vbool; break;
  case SyntaxKind::STR:     os << "\"" << u.str << "\""; break; // FIXME: utf-8 decoding & escaping!
  default:
    assert( false );
  }
}

bool SyntaxValue::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (skind != x->skind)
    return false;
  const SyntaxValue * p = (const SyntaxValue *)x;

  switch (skind)
  {
  case SyntaxKind::REAL:    return u.real == p->u.real;
  case SyntaxKind::INTEGER: return u.integer == p->u.integer;
  case SyntaxKind::BOOL:    return u.vbool == p->u.vbool;
  case SyntaxKind::STR:     return std::strcmp( u.str, p->u.str) == 0;
  default:
    assert( false );
    return false;
  }
}

/** Create the chain of marked symbols in the symbol table */
static Symbol * wrapSymbol ( Mark * mark, Symbol * symbol )
{
  Symbol * parentSymbol = mark->next ? wrapSymbol( mark->next, symbol ) : symbol;
  if (mark->isAntiMark())
    return parentSymbol;
  return mark->scope->symbolTable->newSymbol( parentSymbol, mark->value );
}

Syntax * SyntaxSymbol::wrap ( Mark * mark )
{
  if (!mark)
    return this;
  Mark * newMark = concat( mark, this->mark );
  return
    new SyntaxSymbol( this->coords, newMark ? wrapSymbol(newMark, this->symbol) : this->symbol, newMark );
}

void SyntaxSymbol::toStream ( std::ostream & os ) const
{
  os << this->symbol->name;
  if (this->symbol->markStamp)
    os << '@' << this->symbol->uid;
  if (this->mark)
    os << "{" << *this->mark << '}';
}

bool SyntaxSymbol::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (SyntaxKind::SYMBOL != x->skind)
    return false;
  const SyntaxSymbol * p = (const SyntaxSymbol *)x;
  return this->symbol == p->symbol && p1::smalls::equal(this->mark, p->mark);
}

void SyntaxBinding::toStream ( std::ostream & os ) const
{
  os << bnd->sym->name << ':' << bnd->scope->level;
}

bool SyntaxBinding::equal ( const Syntax * x ) const
{
  if (x->skind != SyntaxKind::BINDING)
    return false;
  const SyntaxBinding * p = (const SyntaxBinding *)x;
  return p->bnd == bnd;
}

Syntax* SyntaxPair::car() const
{
  if (!this->mark)
    return m_car;
  if (m_wrappedCar)
    return m_wrappedCar;
  return m_wrappedCar = m_car->wrap( this->mark );
}

Syntax* SyntaxPair::cdr() const
{
  if (!this->mark)
    return m_cdr;
  if (m_wrappedCdr)
    return m_wrappedCdr;
  return m_wrappedCdr = m_cdr->wrap( this->mark );
}

Syntax * SyntaxPair::wrap ( Mark * mark )
{
  return mark ? new SyntaxPair( this->coords, m_car, m_cdr, concat(mark,this->mark) ) : this;
}

void SyntaxPair::toStream ( std::ostream & os ) const
{
  os << '(';
  if (this->mark)
    os << "{" << *this->mark << '}';
  const SyntaxPair * p = this;
  for(;;)
  {
    p->m_car->toStream( os );
    if (isa<SyntaxNil>(p->m_cdr))
    {
      break;
    }
    else if (isa<SyntaxPair>(p->m_cdr))
    {
      p = cast<SyntaxPair>(p->m_cdr);
      os << " ";
    }
    else
    {
      os << " . " << *p->m_cdr;
      break;
    }
  }
  os << ')';
}

bool SyntaxPair::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (x->skind != SyntaxKind::PAIR)
    return false;
  const SyntaxPair * p = (const SyntaxPair *)x;

  return p1::smalls::equal(this->mark, p->mark) && m_car->equal( p->m_car ) && m_cdr->equal( p->m_cdr );
}

Syntax * SyntaxNil::wrap ( Mark * mark )
{
  return this;
}

bool SyntaxNil::equal ( const Syntax * x ) const
{
  return x->skind == SyntaxKind::NIL;
}

void SyntaxNil::toStream ( std::ostream & os ) const
{
  os << "()";
}

Syntax * SyntaxVector::wrap ( Mark * mark )
{
  return mark ? new SyntaxVector( this->coords, m_data, this->len, concat(mark,this->mark) ) : this;
}

Syntax* SyntaxVector::getElement( unsigned i ) const
{
  if (!this->mark)
    return m_data[i];

  Syntax * wrappedElem;

  if (!m_wrappedData)
  {
    wrappedElem = m_data[i]->wrap(this->mark);
    if (wrappedElem == m_data[i]) // avoid allocating if the element is the same
      return wrappedElem;

    m_wrappedData = new (GC) Syntax*[this->len];
    std::memset( m_wrappedData, 0, sizeof(m_wrappedData[0])*this->len );

    m_wrappedData[i] = wrappedElem;
  }
  else if ((wrappedElem = m_wrappedData[i]) == NULL) // this element not yet lazily wrapped?
    m_wrappedData[i] = wrappedElem = m_data[i]->wrap(this->mark);
  else
    assert( wrappedElem != NULL );

  return wrappedElem;
}


void SyntaxVector::toStream ( std::ostream & os ) const
{
  os << "#(";
  if (this->mark)
    os << "{" << *this->mark << '}';

  for ( unsigned i = 0; i != len; ++i )
  {
    if (i != 0)
      os << " ";
    os << *m_data[i];
  }
  os << ")";
}

bool SyntaxVector::equal ( const Syntax * x ) const
{
  if (x == this)
    return true;
  if (skind != x->skind)
    return false;
  const SyntaxVector * v = (const SyntaxVector *)x;
  if (!p1::smalls::equal(this->mark, v->mark))
    return false;
  if (len != v->len)
    return false;
  for ( unsigned i = 0; i != len; ++i )
    if (!m_data[i]->equal( v->m_data[i]))
      return false;
  return true;
}

void Syntax::toStreamIndented ( std::ostream & os, unsigned indent, const Syntax * datum )
{
  //os << datum->coords.line << ':' << datum->coords.column << ':';
  if (isa<SyntaxNil>(datum))
    os << "()";
  else if (datum->skind == SyntaxKind::VECTOR)
  {
    const SyntaxVector * vec = (const SyntaxVector*)datum;
    os << "#(";
    for ( unsigned i = 0; i != vec->len; ++i )
    {
      if (i > 0)
        os << ' ';
      toStreamIndented( os, indent, vec->m_data[i] );
    }
    os << ')';
  }
  else if (datum->skind == SyntaxKind::PAIR)
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

      toStreamIndented( os, indent, p->m_car);
      if (isa<SyntaxNil>(p->m_cdr))
        break;

      if (p->m_cdr->skind == SyntaxKind::PAIR)
      {
        p = (const SyntaxPair *) p->m_cdr;
        os << ' ';
      }
      else
      {
        os << " . ";
        toStreamIndented( os, indent, p->m_cdr );
        break;
      }
    }
    os << ')';
    indent -= 4;
  }
  else
    os << *datum;
}

}} // namespaces
