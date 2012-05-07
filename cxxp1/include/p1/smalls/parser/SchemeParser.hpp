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
  class Keywords;
}}

namespace p1 {
namespace smalls {

class SchemeParser : public gc
{
public:
  SchemeParser( SymbolTable & symbolTable, const Keywords & kw, AbstractErrorReporter & errors );
  ~SchemeParser();

  AstModule * compileLibraryBody ( Syntax * datum );

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
  AbstractErrorReporter & m_errors;

  Mark * const m_antiMark;
  AstFrame * m_systemFrame; // the frame containing the system symbols
  Binding * m_bindBegin; // the "begin" system binding. We need it occasionally
  Binding * m_unspec;

  AstBody * compileBody ( Context * ctx, Syntax * datum );
  void parseBody ( Context * ctx, Syntax * datum);
  void processBodyForm ( Context * ctx, Syntax * datum );
  void recordDefine ( Context * ctx, SyntaxPair * form );

  Syntax * expandMacro ( Context * ctx, Macro * macro, SyntaxPair * pair );

  Ast * compileExpression ( Context * ctx, Syntax * expr );
  Ast * compileBinding ( Context * ctx, Binding * bnd, Syntax * exprForCoords );
  Ast * compileCall ( Context * ctx, SyntaxPair * call );
  Ast * compileResForm ( Context * ctx, SyntaxPair * pair, Binding * bndCar );
  Ast * compileBegin ( Context * ctx, SyntaxPair * beginPair );
  Ast * compileSetBang ( Context * ctx, SyntaxPair * setPair );
  Ast * compileIf ( Context * ctx, SyntaxPair * ifPair );
  Ast * compileLambda ( Context * ctx, SyntaxPair * lambdaPair );
  Ast * compileLet ( Context * ctx, SyntaxPair * letPair );
  Ast * compileBasicLet ( Context * ctx, SyntaxPair * letPair );
  Ast * compileNamedLet ( Context * ctx, SyntaxPair * letPair );
  bool splitLetParams ( Syntax * p0, DatumList & varDatums, DatumList & valueDatums );

  static Ast * makeUnspecified ( const SourceCoords & coords );
  static Ast * makeUnspecified ( Syntax * where )
  {
    return makeUnspecified( where->coords );
  }

  bool needParams ( const char * formName, Syntax * datum, unsigned np, Syntax ** params, SyntaxPair ** restp );

  bool bindSyntaxSymbol ( Binding * & res, Scope * scope, SyntaxSymbol * ss );
  Binding * lookupSyntaxSymbol ( SyntaxSymbol * ss );

  SyntaxPair * needPair ( const char * formName, Syntax * datum );
  bool needNil ( const char * formName, Syntax * datum );
  Binding * isBinding ( Syntax * datum );

  void error ( Syntax * datum, const char * msg, ... );
};

inline Binding * SchemeParser::isBinding ( Syntax * datum )
{
  if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(datum))
    return lookupSyntaxSymbol(ss);
  else if (SyntaxBinding * b = dyn_cast<SyntaxBinding>(datum))
    return b->bnd;
  else
    return NULL;
}

}} // namespaces

#endif	/* P1_SMALLS_PARSER_SCHEMEPARSER_HPP */

