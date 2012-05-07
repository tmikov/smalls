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
#include "Keywords.hpp"
#include "SymbolTable.hpp"

namespace p1 {
namespace smalls {

Keywords::Keywords ( SymbolTable & symbolTable_ ) :
  symbolTable( symbolTable_),
  sym_quote             ( symbolTable.newSymbol( "quote" ) ),
  sym_quasiquote        ( symbolTable.newSymbol( "quasiquote" ) ),
  sym_unquote           ( symbolTable.newSymbol( "unquote" ) ),
  sym_unquote_splicing  ( symbolTable.newSymbol( "unquote-splicing" ) ),
  sym_syntax            ( symbolTable.newSymbol( "syntax" ) ),
  sym_quasisyntax       ( symbolTable.newSymbol( "quasisyntax" ) ),
  sym_unsyntax          ( symbolTable.newSymbol( "unsyntax" ) ),
  sym_unsyntax_splicing ( symbolTable.newSymbol( "unsyntax-splicing" ) ),

  sym_if                ( symbolTable.newSymbol( "if" ) ),
  sym_begin             ( symbolTable.newSymbol( "begin" ) ),
  sym_lambda            ( symbolTable.newSymbol( "lambda" ) ),
  sym_define            ( symbolTable.newSymbol( "define" ) ),
  sym_setbang           ( symbolTable.newSymbol( "set!" ) ),
  sym_let               ( symbolTable.newSymbol( "let" ) ),
  sym_letrec            ( symbolTable.newSymbol( "letrec" ) ),
  sym_letrec_star       ( symbolTable.newSymbol( "letrec*" ) ),

  sym_builtin           ( symbolTable.newSymbol( "__%builtin" ) ),
  sym_define_macro      ( symbolTable.newSymbol( "define-macro" ) ),
  sym_define_identifier_macro ( symbolTable.newSymbol( "define-identifier-macro" ) ),
  sym_define_set_macro  ( symbolTable.newSymbol( "define-set-macro" ) ),
  sym_macro_env         ( symbolTable.newSymbol( "macro-env" ) )
{}

}} // namespaces
