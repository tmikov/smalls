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

namespace p1 {
namespace smalls {


class ReservedSymbols : public gc
{
public:
  Symbol * const sym_quote;
  Symbol * const sym_quasiquote;
  Symbol * const sym_unquote;
  Symbol * const sym_unquote_splicing;
  Symbol * const sym_syntax;
  Symbol * const sym_quasisyntax;
  Symbol * const sym_unsyntax;
  Symbol * const sym_unsyntax_splicing;

  Symbol * const sym_if;
  Symbol * const sym_begin;
  Symbol * const sym_lambda;
  Symbol * const sym_define;
  Symbol * const sym_setbang;
  Symbol * const sym_let;
  Symbol * const sym_letrec;
  Symbol * const sym_letrec_star;

  Symbol * const sym_builtin;
  Symbol * const sym_define_macro;
  Symbol * const sym_define_identifier_macro;
  Symbol * const sym_define_set_macro;
  Symbol * const sym_macro_env;

  ReservedSymbols ( SymbolTable & map );
};

class SyntaxReader : public gc
{
public:
  SyntaxReader ( Lexer & lex );

  Syntax * parseDatum ();

  Syntax * const DAT_EOF;
  Syntax * const DAT_COM;
private:
  Lexer & m_lex;
  ReservedSymbols m_rsv;
  SourceCoords m_coords;

  Token::Enum next ()
  {
    return m_lex.nextToken();
  }

  Syntax * readSkipDatCom ( unsigned termSet );
  Syntax * read ( unsigned termSet );
  SyntaxPair * list ( const SourceCoords & coords, Token::Enum terminator, unsigned termSet );
  SyntaxVector * vector ( const SourceCoords & coords, Token::Enum terminator, unsigned termSet );
  SyntaxPair * abbrev ( Symbol * sym, unsigned termSet );

  void error ( const gc_char * msg, ... );
};

}} // namespaces

#endif	/* P1_SMALLS_PARSER_SYNTAXREADER_HPP */

