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


#ifndef P1_SMALLS_PARSER_SCHEMEPARSER_HPP
#define	P1_SMALLS_PARSER_SCHEMEPARSER_HPP

#include "SyntaxReader.hpp"
#include "SymbolTable.hpp"
#include "p1/smalls/ast/SchemeAST.hpp"
#include "p1/smalls/common/AbstractErrorReporter.hpp"

namespace p1 {
namespace smalls {

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

  ListOfAst compileLibraryBody ( Syntax * datum );

private:
  typedef std::list<Syntax *,gc_allocator<SyntaxPair *> > DatumList;

  typedef std::pair<Binding *, Syntax *> DeferredDefine;
  typedef std::list<DeferredDefine,gc_allocator<DeferredDefine> > DeferredDefineList;

  struct Context : public gc
  {
    Scope * scope;
    AstFrame * frame;
    DeferredDefineList defnList; //< the deferred definitions
    DatumList exprList; //< the body expressions

    Context ( Scope * scope_, AstFrame * frame_ ) : scope(scope_), frame(frame_) {};

    bool topLevel () const { return this->scope->level == 0; };
  };

  SymbolTable & m_symbolTable;
  Scope * m_systemScope;
  ReservedBindings m_rsv;
  AbstractErrorReporter & m_errors;

  Binding * m_unspec;
  Mark * const m_antiMark;

  ListOfAst compileBody ( Context * ctx, Syntax * datum );
  void parseBody ( Context * ctx, Syntax * datum);
  void processBodyForm ( Context * ctx, Syntax * datum );
  void recordDefine ( Context * ctx, SyntaxPair * form );

  Syntax * expandMacro ( Context * ctx, Macro * macro, SyntaxPair * pair );

  ListOfAst convertLetRecStar ( Context * ctx );

  ListOfAst compileExpression ( Context * ctx, Syntax * expr );
  ListOfAst compileCall ( Context * ctx, SyntaxPair * call );
  ListOfAst compileResForm ( Context * ctx, SyntaxPair * pair, Binding * bndCar );
  ListOfAst compileBegin ( Context * ctx, SyntaxPair * beginPair );
  ListOfAst compileSetBang ( Context * ctx, SyntaxPair * setPair );
  ListOfAst compileIf ( Context * ctx, SyntaxPair * ifPair );
  ListOfAst compileLambda ( Context * ctx, SyntaxPair * lambdaPair );
  ListOfAst compileLet ( Context * ctx, SyntaxPair * letPair );
  ListOfAst compileBasicLet ( Context * ctx, SyntaxPair * letPair );
  ListOfAst compileNamedLet ( Context * ctx, SyntaxPair * letPair );
  bool splitLetParams ( Syntax * p0, DatumList & varDatums, DatumList & valueDatums );

  ListOfAst makeUnspecified ( Syntax * where );

  bool needParams ( const char * formName, Syntax * datum, unsigned np, Syntax ** params, SyntaxPair ** restp );

  bool bindSyntaxSymbol ( Binding * & res, Scope * scope, SyntaxSymbol * ss );
  Binding * lookupSyntaxSymbol ( SyntaxSymbol * ss );

  SyntaxPair * needPair ( const char * formName, Syntax * datum );
  bool needNil ( const char * formName, Syntax * datum );
  SyntaxPair * isPair ( Syntax * datum );
  Binding * isBinding ( Syntax * datum );

  void error ( Syntax * datum, const char * msg, ... );
};

inline SyntaxPair * SchemeParser::isPair ( Syntax * datum )
{
  return datum->skind == SyntaxKind::PAIR ? static_cast<SyntaxPair*>(datum) : NULL;
}

inline Binding * SchemeParser::isBinding ( Syntax * datum )
{
  if (datum->skind == SyntaxKind::SYMBOL)
    return lookupSyntaxSymbol(static_cast<SyntaxSymbol*>(datum));
  else if (datum->skind == SyntaxKind::BINDING)
    return static_cast<SyntaxBinding*>(datum)->bnd;
  else
    return NULL;
}

}} // namespaces

#endif	/* P1_SMALLS_PARSER_SCHEMEPARSER_HPP */

