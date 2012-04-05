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


#ifndef SCHEMEPARSER_HPP
#define	SCHEMEPARSER_HPP

#include <list>

#include "SyntaxReader.hpp"
#include "AbstractErrorReporter.hpp"
#include "SymbolTable.hpp"

class ReservedBindings : public gc
{
public:
  Scope * const scope;

  ReservedSymbols sym;

  Binding * const bind_quote;
  Binding * const bind_quasiquote;
  Binding * const bind_unquote;
  Binding * const bind_unquote_splicing;
  Binding * const bind_syntax;
  Binding * const bind_quasisyntax;
  Binding * const bind_unsyntax;
  Binding * const bind_unsyntax_splicing;

  Binding * const bind_if;
  Binding * const bind_begin;
  Binding * const bind_lambda;
  Binding * const bind_define;
  Binding * const bind_setbang;
  Binding * const bind_let;
  Binding * const bind_letrec;
  Binding * const bind_letrec_star;

  Binding * const bind_builtin;
  Binding * const bind_define_macro;
  Binding * const bind_define_identifier_macro;
  Binding * const bind_define_set_macro;
  Binding * const bind_macro_env;

  ReservedBindings ( SymbolTable & map, Scope * scope );

private:
  static Binding * bind ( Scope * scope, Symbol * sym, ResWord::Enum resCode );
};

class SchemeParser : public gc
{
public:
  SchemeParser( SymbolTable & symbolTable, AbstractErrorReporter & errors );
  ~SchemeParser();

  void parseLibraryBody ( Syntax * datum );

private:
  SymbolTable m_symbolTable;
  ReservedBindings m_rsv;
  AbstractErrorReporter & m_errors;

  typedef std::list<SyntaxPair *,gc_allocator<SyntaxPair *> > PairList;

  PairList m_defnList; //< the deferred definitions
  PairList m_exprList; //< the body expressions

  /** A placeholder for expressions coverted to (define <unused> ...) */
  SyntaxValue * m_unusedSymbol;
  Syntax * m_unspec; // A datum containing the representation of #unspecified

  void parseBody ( Syntax * datum );
  void processBodyForm ( Syntax * datum );
  void recordDefine ( SyntaxPair * form );
  SyntaxPair * convertToLetrecStar ();

  SyntaxPair * needPair ( Syntax * datum );
  void error ( Syntax * datum, const char * msg, ... );
};

#endif	/* SCHEMEPARSER_HPP */

