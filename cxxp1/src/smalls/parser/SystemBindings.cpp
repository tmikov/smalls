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
#include "SystemBindings.hpp"
#include "Keywords.hpp"
#include "p1/smalls/ast/Frame.hpp"

namespace p1 {
namespace smalls {
namespace detail {

SystemBindings::SystemBindings ( SymbolTable & symTab, const Keywords & kw, Scope * sc ) :
  scope( sc ),
  frame( new ast::Frame(NULL) ),
  bind_quote             ( bindKw( scope, kw.sym_quote, ResWord::QUOTE ) ),
  bind_quasiquote        ( bindKw( scope, kw.sym_quasiquote, ResWord::NONE /*SymCode::QUASIQUOTE*/ ) ),
  bind_unquote           ( bindKw( scope, kw.sym_unquote, ResWord::NONE /*SymCode::UNQUOTE*/ ) ),
  bind_unquote_splicing  ( bindKw( scope, kw.sym_unquote_splicing, ResWord::NONE /*SymCode::UNQUOTE_SPLICING*/ ) ),
  bind_syntax            ( bindKw( scope, kw.sym_syntax, ResWord::SYNTAX ) ),
  bind_quasisyntax       ( bindKw( scope, kw.sym_quasisyntax, ResWord::QUASISYNTAX ) ),
  bind_unsyntax          ( bindKw( scope, kw.sym_unsyntax, ResWord::UNSYNTAX ) ),
  bind_unsyntax_splicing ( bindKw( scope, kw.sym_unsyntax_splicing, ResWord::UNSYNTAX_SPLICING ) ),

  bind_if                ( bindKw( scope, kw.sym_if, ResWord::IF ) ),
  bind_begin             ( bindKw( scope, kw.sym_begin, ResWord::BEGIN ) ),
  bind_lambda            ( bindKw( scope, kw.sym_lambda, ResWord::LAMBDA ) ),
  bind_define            ( bindKw( scope, kw.sym_define, ResWord::DEFINE ) ),
  bind_setbang           ( bindKw( scope, kw.sym_setbang, ResWord::SETBANG ) ),
  bind_let               ( bindKw( scope, kw.sym_let, ResWord::LET ) ),
  bind_letrec            ( bindKw( scope, kw.sym_letrec, ResWord::LETREC ) ),
  bind_letrec_star       ( bindKw( scope, kw.sym_letrec_star, ResWord::LETREC_STAR ) ),

  bind_builtin           ( bindKw( scope, kw.sym_builtin, ResWord::BUILTIN ) ),
  bind_define_macro      ( bindKw( scope, kw.sym_define_macro, ResWord::DEFINE_MACRO ) ),
  bind_define_identifier_macro ( bindKw( scope, kw.sym_define_identifier_macro, ResWord::DEFINE_IDENTIFIER_MACRO ) ),
  bind_define_set_macro  ( bindKw( scope, kw.sym_define_set_macro, ResWord::DEFINE_SET_MACRO ) ),
  bind_macro_env         ( bindKw( scope, kw.sym_macro_env, ResWord::MACRO_ENV ) )
{
  assert( &symTab == &kw.symbolTable );
}

Binding * SystemBindings::bindKw ( Scope * scope, Symbol * sym, ResWord::Enum resCode )
{
  Binding * res;
  if (scope->bind( res, sym, SourceCoords() ))
  {
    res->bindResWord( resCode );
    return res;
  }
  else
  {
    assert( false );
    std::abort();
    return NULL;
  }
}

}}} // namespaces
