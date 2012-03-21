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


#include "SyntaxReader.hpp"

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


static inline bool setContains ( unsigned set, Token::Enum tok )
{
  return (set & (1 << tok)) != 0;
}

static inline unsigned setAdd ( unsigned set, Token::Enum tok )
{
  return set | (1 << tok);
}

SyntaxReader::SyntaxReader ( Lexer & lex )
  : m_lex( lex ),
    DAT_EOF( new Syntax(SyntaxClass::DEOF, SourceCoords(NULL,0,0)) ),
    DAT_COM( new Syntax(SyntaxClass::COMMENT, SourceCoords(NULL,0,0)) )
{
  next();
}

void SyntaxReader::error ( const gc_char * msg, ... )
{
  std::va_list ap;
  va_start( ap, msg );
  m_lex.errorReporter().error( m_lex.coords(), vformatGCStr( msg, ap ) );
  va_end( ap );
}

Syntax * SyntaxReader::readSkipDatCom ( unsigned termSet )
{
  // Ignore DATUM_COMMENT-s
  Syntax * res;
  while ( (res = read(termSet)) == DAT_COM)
    {}
  return res;
}

Syntax * SyntaxReader::read ( unsigned termSet )
{
  bool inError = false;

  for(;;)
  {
    Syntax * res;
    switch (m_lex.curToken())
    {
    case Token::EOFTOK: return DAT_EOF;

    case Token::BOOL:    res = new SyntaxValue( SyntaxClass::BOOL,    m_lex.coords(), m_lex.valueBool()    ); next(); return res;
    case Token::INTEGER: res = new SyntaxValue( SyntaxClass::INTEGER, m_lex.coords(), m_lex.valueInteger() ); next(); return res;
    case Token::REAL:    res = new SyntaxValue( SyntaxClass::REAL,    m_lex.coords(), m_lex.valueReal()    ); next(); return res;
    case Token::STR:     res = new SyntaxValue( SyntaxClass::STR,     m_lex.coords(), m_lex.valueString()  ); next(); return res;
    case Token::SYMBOL:  res = new SyntaxValue( SyntaxClass::SYMBOL,  m_lex.coords(), m_lex.valueSymbol()  ); next(); return res;

    case Token::LPAR:
      { SourceCoords coords=m_lex.coords(); next(); return list( coords, Token::RPAR, termSet ); }
    case Token::LSQUARE:
      { SourceCoords coords=m_lex.coords(); next(); return list( coords, Token::RSQUARE, termSet ); }
    case Token::HASH_LPAR:
      { SourceCoords coords=m_lex.coords(); next(); return vector( coords, Token::RPAR, termSet ); }

    case Token::APOSTR:         return abbrev( m_lex.symbolMap().sym_quote, termSet );
    case Token::ACCENT:         return abbrev( m_lex.symbolMap().sym_quasiquore, termSet );
    case Token::COMMA:          return abbrev( m_lex.symbolMap().sym_unquote, termSet );
    case Token::COMMA_AT:       return abbrev( m_lex.symbolMap().sym_unquote_splicing, termSet );
    case Token::HASH_APOSTR:    return abbrev( m_lex.symbolMap().sym_syntax, termSet );
    case Token::HASH_ACCENT:    return abbrev( m_lex.symbolMap().sym_quasisyntax, termSet );
    case Token::HASH_COMMA:     return abbrev( m_lex.symbolMap().sym_unsyntax, termSet );
    case Token::HASH_COMMA_AT:  return abbrev( m_lex.symbolMap().sym_unsyntax_splicing, termSet );

    case Token::DATUM_COMMENT:
      next();
      read( termSet ); // Ignore the next datum
      return DAT_COM;

    case Token::NESTED_COMMENT_END:
    case Token::NESTED_COMMENT_START:
      assert(false);
    case Token::DOT:
    case Token::RPAR:
    case Token::RSQUARE:
      // Skip invalid tokens, reporting only the first one
      if (!inError)
      {
        error( "'%s' isn't allowed here", Token::repr(m_lex.curToken()) );
        inError = true;
      }
      if (setContains(termSet,m_lex.curToken()))
        return new SyntaxNil(m_lex.coords());
      next();
      break;
    }
  }
}

// TODO: this routine uses stack proportional to the size of the list.
// Ways to address that:
//   - mutable pairs
//   - build the list in reverse and reverse it again
//   - use a different intermediate storage and build the list in the end
//
SyntaxPair * SyntaxReader::list ( const SourceCoords & coords, Token::Enum terminator, unsigned termSet )
{
  Syntax * car, * cdr;
  termSet = setAdd(termSet,terminator);
  unsigned carTermSet = setAdd(termSet, Token::DOT);

  // Check for an empty list. It is complicated by having to skip DATUM_COMMENT-s
  do
  {
    if (m_lex.curToken() == terminator)
    {
      next();
      return new SyntaxNil( coords );
    }
  }
  while ( (car = read( carTermSet )) == DAT_COM);

  if (car == DAT_EOF)
  {
    error( "Unterminated list" );
    return new SyntaxNil( coords );
  }
  if (m_lex.curToken() == Token::DOT)
  {
    next();
    if ( (cdr = readSkipDatCom(termSet)) == DAT_EOF)
    {
      error( "Unterminated list" );
      return new SyntaxNil( m_lex.coords() );
    }
    if (m_lex.curToken() != terminator)
      error( "Expected %s", Token::repr(terminator) ); //TODO: skip??
    next();
  }
  else
  {
    SourceCoords tmp = m_lex.coords();
    cdr = list( tmp, terminator, termSet );
  }

  return new SyntaxPair( coords, car, cdr );
}

SyntaxVector * SyntaxReader::vector ( const SourceCoords & coords, Token::Enum terminator, unsigned termSet )
{
  Syntax ** vec = NULL;
  unsigned count = 0, size = 0;
  termSet = setAdd(termSet,terminator);

  while (m_lex.curToken() != terminator)
  {
    Syntax * elem;
    if ( (elem = read( termSet )) == DAT_COM) // skip DATUM_COMMENT-s
      continue;
    if (elem == DAT_EOF)
    {
      error( "Unterminated vector" );
      return new SyntaxVector( coords, NULL, 0 ); // Return an empty vector just for error recovery
    }

    if (!vec)
    {
      vec = new (GC) Syntax*[4];
      size = 4;
    }
    else if (count == size)
    {
      Syntax ** newVec = new (GC) Syntax*[size*2];
      std::memcpy( newVec, vec, sizeof(vec[0])*size );
      vec = newVec;
      size *= 2;
    }
    vec[count++] = elem;
  }
  next(); // skip the closing parren

  if (!vec)
    return new SyntaxVector( coords, NULL, 0 );
  else if (count*4 >= size*3) // If at least 75% full
    return new SyntaxVector( coords, vec, count );
  else
  {
    // Allocate an exact-sized vector
    Syntax ** newVec = new (GC) Syntax*[count];
    std::memcpy( newVec, vec, sizeof(vec[0])*count );
    return new SyntaxVector( coords, newVec, count );
  }
}

SyntaxPair * SyntaxReader::abbrev ( Symbol * sym, unsigned termSet )
{
  SyntaxValue * symdat = new SyntaxValue( SyntaxClass::SYMBOL, m_lex.coords(), sym );

  next();

  Syntax * datum;
  if ( (datum = readSkipDatCom( termSet )) == DAT_EOF)
    error( "Unterminated abbreviation" );

  return
    new SyntaxPair( symdat->coords, symdat, new SyntaxPair( datum->coords, datum, new SyntaxNil( datum->coords ) ) );
}
