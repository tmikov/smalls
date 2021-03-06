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
#include "SymbolTable.hpp"
#include "ListBuilder.hpp"
#include "p1/util/format-str.hpp"
#include "Keywords.hpp"

using namespace p1;
using namespace p1::smalls;
using namespace p1::smalls::detail;

static inline bool setContains ( unsigned set, TokenKind::Enum tok )
{
  return (set & (1 << tok)) != 0;
}

static inline unsigned setAdd ( unsigned set, TokenKind::Enum tok )
{
  return set | (1 << tok);
}

SyntaxReader::SyntaxReader ( Lexer & lex, const Keywords & kw )
  : DAT_EOF( new Syntax(SyntaxKind::DEOF, SourceCoords(NULL,0,0)) ),
    DAT_COM( new Syntax(SyntaxKind::COMMENT, SourceCoords(NULL,0,0)) ),
    m_lex( lex ),
    m_kw( kw )
{
  assert( &m_kw.symbolTable == &m_lex.symbolTable() );
  next();
}

void SyntaxReader::error ( const gc_char * msg, ... )
{
  std::va_list ap;
  va_start( ap, msg );
  m_lex.errorReporter().error( m_tok.coords(), vformatGCStr( msg, ap ) );
  va_end( ap );
}

Syntax * SyntaxReader::parseDatum ()
{
  return readSkipDatCom( setAdd(0, TokenKind::EOFTOK) );
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
    switch (m_tok.kind())
    {
    case TokenKind::EOFTOK: return DAT_EOF;

    case TokenKind::BOOL:    res = new SyntaxValue( SyntaxKind::BOOL,    m_tok.coords(), m_tok.vbool() ); next(); return res;
    case TokenKind::INTEGER: res = new SyntaxValue( SyntaxKind::INTEGER, m_tok.coords(), m_tok.integer() ); next(); return res;
    case TokenKind::REAL:    res = new SyntaxValue( SyntaxKind::REAL,    m_tok.coords(), m_tok.real()    ); next(); return res;
    case TokenKind::STR:     res = new SyntaxValue( SyntaxKind::STR,     m_tok.coords(), m_tok.string()  ); next(); return res;
    case TokenKind::SYMBOL:  res = new SyntaxSymbol( m_tok.coords(), m_tok.symbol()  ); next(); return res;

    case TokenKind::LPAR:
      { SourceCoords coords=m_tok.coords(); next(); return list( coords, TokenKind::RPAR, termSet ); }
    case TokenKind::LSQUARE:
      { SourceCoords coords=m_tok.coords(); next(); return list( coords, TokenKind::RSQUARE, termSet ); }
    case TokenKind::HASH_LPAR:
      { SourceCoords coords=m_tok.coords(); next(); return vector( coords, TokenKind::RPAR, termSet ); }

    case TokenKind::APOSTR:         return abbrev( m_kw.sym_quote, termSet );
    case TokenKind::ACCENT:         return abbrev( m_kw.sym_quasiquote, termSet );
    case TokenKind::COMMA:          return abbrev( m_kw.sym_unquote, termSet );
    case TokenKind::COMMA_AT:       return abbrev( m_kw.sym_unquote_splicing, termSet );
    case TokenKind::HASH_APOSTR:    return abbrev( m_kw.sym_syntax, termSet );
    case TokenKind::HASH_ACCENT:    return abbrev( m_kw.sym_quasisyntax, termSet );
    case TokenKind::HASH_COMMA:     return abbrev( m_kw.sym_unsyntax, termSet );
    case TokenKind::HASH_COMMA_AT:  return abbrev( m_kw.sym_unsyntax_splicing, termSet );

    case TokenKind::DATUM_COMMENT:
      next();
      read( termSet ); // Ignore the next datum
      return DAT_COM;

    case TokenKind::NESTED_COMMENT_END:
    case TokenKind::NESTED_COMMENT_START:
      assert(false);
    case TokenKind::DOT:
    case TokenKind::RPAR:
    case TokenKind::RSQUARE:
    case TokenKind::NONE:
      // Skip invalid tokens, reporting only the first one
      if (!inError)
      {
        error( "'%s' isn't allowed here", TokenKind::repr(m_tok.kind()) );
        inError = true;
      }
      if (setContains(termSet,m_tok.kind()))
        return new SyntaxNil(m_tok.coords());
      next();
      break;
    }
  }
}

SyntaxPair * SyntaxReader::list ( const SourceCoords & coords, TokenKind::Enum terminator, unsigned termSet )
{
  ListBuilder lb;
  termSet = setAdd(termSet,terminator);
  unsigned carTermSet = setAdd(termSet, TokenKind::DOT);

  lb << coords;

  for(;;)
  {
    Syntax * car;

    // Check for end of list. It is complicated by having to skip DATUM_COMMENT-s
    do
    {
      if (m_tok.kind() == terminator)
      {
        lb << m_tok.coords();
        next();
        return lb.toList();
      }
    }
    while ( (car = read( carTermSet )) == DAT_COM);

    if (car == DAT_EOF)
    {
      error( "Unterminated list" );
      return lb.toList();
    }

    lb << car;

    if (m_tok.kind() == TokenKind::DOT)
    {
      Syntax * cdr;

      next();
      if ( (cdr = readSkipDatCom(termSet)) == DAT_EOF)
      {
        error( "Unterminated list" );
        return lb.toList();
      }

      if (m_tok.kind() == terminator)
        next();
      else
      {
        error( "Expected %s", TokenKind::repr(terminator) );
        // skip until terminator
        assert( setContains(termSet, TokenKind::EOFTOK) ); // all sets should include EOF
        for(;;)
        {
          if (m_tok.kind() == terminator)
          {
            next();
            break;
          }
          if (setContains(termSet, m_tok.kind()))
            break;
          next();
        }
      }

      return lb.toList( cdr );
    }
  }
}

SyntaxVector * SyntaxReader::vector ( const SourceCoords & coords, TokenKind::Enum terminator, unsigned termSet )
{
  Syntax ** vec = NULL;
  unsigned count = 0, size = 0;
  termSet = setAdd(termSet,terminator);

  while (m_tok.kind() != terminator)
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
  SyntaxValue * symdat = new SyntaxValue( SyntaxKind::SYMBOL, m_tok.coords(), sym );

  next();

  Syntax * datum;
  if ( (datum = readSkipDatCom( termSet )) == DAT_EOF)
    error( "Unterminated abbreviation" );

  return
    new SyntaxPair( symdat->coords, symdat, new SyntaxPair( datum->coords, datum, new SyntaxNil( datum->coords ) ) );
}
