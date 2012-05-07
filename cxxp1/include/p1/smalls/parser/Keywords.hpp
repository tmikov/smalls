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
#ifndef P1_SMALLS_PARSER_KEYWORDS_HPP
#define	P1_SMALLS_PARSER_KEYWORDS_HPP

#include "p1/util/gc-support.hpp"

namespace p1 {
namespace smalls {
  class Symbol;
  class SymbolTable;
}}

namespace p1 {
namespace smalls {

class Keywords : public gc
{
public:
  SymbolTable & symbolTable;

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

  Keywords ( SymbolTable & symbolTable_ );
};

}} // namespaces

#endif	/* P1_SMALLS_PARSER_KEYWORDS_HPP */
