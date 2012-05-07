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


#ifndef P1_SMALLS_PARSER_SYNTAXREADER_HPP
#define	P1_SMALLS_PARSER_SYNTAXREADER_HPP

#include "Lexer.hpp"
#include "Syntax.hpp"
#include "Keywords.hpp"

namespace p1 {
namespace smalls {

class SyntaxReader : public gc
{
public:
  SyntaxReader ( Lexer & lex, const Keywords & kw );

  Syntax * parseDatum ();

  const Keywords & keywords () const { return m_kw; }

  Syntax * const DAT_EOF;
  Syntax * const DAT_COM;
private:
  Lexer & m_lex;
  Token m_tok;
  const Keywords & m_kw;
  SourceCoords m_coords;

  TokenKind::Enum next ()
  {
    return m_lex.nextToken( m_tok );
  }

  Syntax * readSkipDatCom ( unsigned termSet );
  Syntax * read ( unsigned termSet );
  SyntaxPair * list ( const SourceCoords & coords, TokenKind::Enum terminator, unsigned termSet );
  SyntaxVector * vector ( const SourceCoords & coords, TokenKind::Enum terminator, unsigned termSet );
  SyntaxPair * abbrev ( Symbol * sym, unsigned termSet );

  void error ( const gc_char * msg, ... );
};

}} // namespaces

#endif	/* P1_SMALLS_PARSER_SYNTAXREADER_HPP */

