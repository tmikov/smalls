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
#ifndef P1_SMALLS_PARSER_SYSTEMBINDINGS_HPP
#define	P1_SMALLS_PARSER_SYSTEMBINDINGS_HPP

#include "p1/util/gc-support.hpp"
#include "SymbolTable.hpp"

namespace p1 {
namespace smalls {
  class Keywords;
}}

namespace p1 {
namespace smalls {
namespace ast {
  class Frame;
}}}

namespace p1 {
namespace smalls {
namespace detail {

class SystemBindings : public gc
{
public:
  Scope * const scope;
  ast::Frame * const frame;

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

  SystemBindings ( SymbolTable & symTab, const Keywords & kw, Scope * scope );

private:
  static Binding * bindKw ( Scope * scope, Symbol * sym, ResWord::Enum resCode );
};

}}}// namespaces

#endif	/* P1_SMALLS_PARSER_SYSTEMBINDINGS_HPP */
