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


#include <boost/foreach.hpp>

#include "SchemeParser.hpp"
#include "ListBuilder.hpp"


ReservedBindings::ReservedBindings ( SymbolTable & map, Scope * sc ) :
  scope( sc ),
  sym( map ),
  bind_quote             ( bind( scope, sym.sym_quote, SymCode::QUOTE ) ),
  bind_quasiquote        ( bind( scope, sym.sym_quasiquote, SymCode::NONE /*SymCode::QUASIQUOTE*/ ) ),
  bind_unquote           ( bind( scope, sym.sym_unquote, SymCode::NONE /*SymCode::UNQUOTE*/ ) ),
  bind_unquote_splicing  ( bind( scope, sym.sym_unquote_splicing, SymCode::NONE /*SymCode::UNQUOTE_SPLICING*/ ) ),
  bind_syntax            ( bind( scope, sym.sym_syntax, SymCode::SYNTAX ) ),
  bind_quasisyntax       ( bind( scope, sym.sym_quasisyntax, SymCode::QUASISYNTAX ) ),
  bind_unsyntax          ( bind( scope, sym.sym_unsyntax, SymCode::UNSYNTAX ) ),
  bind_unsyntax_splicing ( bind( scope, sym.sym_unsyntax_splicing, SymCode::UNSYNTAX_SPLICING ) ),

  bind_if                ( bind( scope, sym.sym_if, SymCode::IF ) ),
  bind_begin             ( bind( scope, sym.sym_begin, SymCode::BEGIN ) ),
  bind_lambda            ( bind( scope, sym.sym_lambda, SymCode::LAMBDA ) ),
  bind_define            ( bind( scope, sym.sym_define, SymCode::DEFINE ) ),
  bind_setbang           ( bind( scope, sym.sym_setbang, SymCode::SETBANG ) ),
  bind_let               ( bind( scope, sym.sym_let, SymCode::LET ) ),
  bind_letrec            ( bind( scope, sym.sym_letrec, SymCode::LETREC ) ),
  bind_letrec_star       ( bind( scope, sym.sym_letrec_star, SymCode::LETREC_STAR ) ),

  bind_builtin           ( bind( scope, sym.sym_builtin, SymCode::BUILTIN ) ),
  bind_define_macro      ( bind( scope, sym.sym_define_macro, SymCode::DEFINE_MACRO ) ),
  bind_define_identifier_macro ( bind( scope, sym.sym_define_identifier_macro, SymCode::DEFINE_IDENTIFIER_MACRO ) ),
  bind_define_set_macro  ( bind( scope, sym.sym_define_set_macro, SymCode::DEFINE_SET_MACRO ) ),
  bind_macro_env         ( bind( scope, sym.sym_macro_env, SymCode::MACRO_ENV ) )
{}

Binding * ReservedBindings::bind ( Scope * scope, Symbol * sym, SymCode::Enum resCode )
{
  Binding * res;
  if (scope->bind( res, sym ))
  {
    res->resCode = resCode;
    return res;
  }
  else
  {
    assert( false );
    std::abort();
    return NULL;
  }
}

SchemeParser::SchemeParser ( SymbolTable & symbolMap, AbstractErrorReporter & errors )
  : m_symbolTable(),
    m_rsv( symbolMap, m_symbolTable.newScope() ),
    m_errors( errors )
{
  SourceCoords c;
  m_unusedSymbol = new SyntaxValue( SyntaxClass::SYMBOL, c, symbolMap.newSymbol("#unused") );

  // TODO: use a 'native' definition
  // (if #f #f) ; declare #unspecified
  m_unspec = ListBuilder()
     << new SyntaxValue( SyntaxClass::BINDING, c, m_rsv.bind_if )
     << new SyntaxValue( SyntaxClass::BOOL, c, false )
     << new SyntaxValue( SyntaxClass::BOOL, c, false );
}

SchemeParser::~SchemeParser ()
{
}

void SchemeParser::parseLibraryBody ( Syntax * datum )
{
  m_defnList.clear();
  m_exprList.clear();

  parseBody( datum );

//  std::cout << "define\n";
//  BOOST_FOREACH( SyntaxPair * defn, m_defnList )
//  {
//    std::cout << "  ";
//    Syntax::toStreamIndented( std::cout, 2, defn );
//    std::cout << std::endl;
//  }
//  std::cout << "\n\nexpr\n";
//  BOOST_FOREACH( SyntaxPair * expr, m_exprList )
//  {
//    std::cout << "  ";
//    Syntax::toStreamIndented( std::cout, 2, expr );
//    std::cout << std::endl;
//  }

  SyntaxPair * body = convertToLetrecStar();

  m_defnList.clear();
  m_exprList.clear();

  Syntax::toStreamIndented( std::cout, 0, body );
  std::cout << std::endl;
}

SyntaxPair * SchemeParser::convertToLetrecStar ()
{
  if (m_defnList.empty()) // If no definitions, convert to (begin ...)
  {
    if (m_exprList.empty())
      return NULL; // FIXME:??? how do we keep track of the name of a file without datums???
    if (m_exprList.size() == 1)
      return *m_exprList.begin();

    ListBuilder begin;
    begin << new SyntaxValue( SyntaxClass::BINDING, (*m_exprList.begin())->coords, m_rsv.bind_begin );
    BOOST_FOREACH( SyntaxPair * expr, m_exprList )
      begin << expr;

    return begin;
  }

  ListBuilder let;
  let << new SyntaxValue( SyntaxClass::BINDING, (*m_defnList.begin())->coords, m_rsv.bind_letrec_star );

  ListBuilder init;
  BOOST_FOREACH( SyntaxPair * defn, m_defnList )
    init << defn;

  let << init;

  ListBuilder body;
  BOOST_FOREACH( SyntaxPair * expr, m_exprList )
    body << expr;

  let << body;
  return let;
}


void SchemeParser::parseBody ( Syntax * datum )
{
  for(;;)
  {
    SyntaxPair * const pair = needPair( datum );
    if (!pair /*not a pair*/ || pair->isNil())
      break;

    processBodyForm( pair->car );
    datum = pair->cdr;
  }
}

void SchemeParser::processBodyForm ( Syntax * datum )
{
  SyntaxPair * const pair = needPair( datum );
  if (!pair)
    return;

  if (pair->isNil())
  {
    error( pair, "Invalid empty form" );
    return;
  }

  Syntax * car = pair->car;
  Binding * binding = NULL;

  if (car->sclass == SyntaxClass::SYMBOL)
    binding = m_symbolTable.lookup(static_cast<SyntaxValue*>(car)->u.symbol);
  else if (car->sclass == SyntaxClass::BINDING)
    binding = static_cast<SyntaxValue*>(car)->u.bnd;

  if (binding == m_rsv.bind_begin)
  {
    // splice the body of (begin ...)
    parseBody( pair->cdr );
    return;
  }
  else if (binding == m_rsv.bind_define)
  {
    recordDefine( pair );
    return;
  }

  // Defer the expression
  m_exprList.push_back( pair );
}

void SchemeParser::recordDefine ( SyntaxPair * form )
{
  if (!m_exprList.empty())
  {
    // Convert all deferred expressions to (define <unused> (begin expression... #undefined))

    ListBuilder lb;
    lb << new SyntaxValue(SyntaxClass::BINDING, (*m_exprList.begin())->coords, m_rsv.bind_begin);
    BOOST_FOREACH( SyntaxPair * expr, m_exprList )
      lb << expr;
    lb << m_unspec;

    m_defnList.push_back( ListBuilder() << m_unusedSymbol << lb );

    m_exprList.clear();
  }

  SyntaxPair * p;
  if (!(p = needPair( form->cdr )))
    return;
  if (p->car->sclass == SyntaxClass::SYMBOL)
  {
    if (!needPair(p->cdr))
      return;
    // FIXME: check for duplicated definition

    // Append to the list of deferred definitions
    m_defnList.push_back( p );
  }
  else
    error( p->car, "symbol required after \"define\"");
}


SyntaxPair * SchemeParser::needPair ( Syntax * datum )
{
  if (datum->sclass != SyntaxClass::PAIR)
  {
    error( datum, "Form must be a proper list" );
    return NULL;
  }
  return static_cast<SyntaxPair*>(datum);
}

void SchemeParser::error ( Syntax * where, const char * msg, ... )
{
  std::va_list ap;
  va_start( ap, msg );
  m_errors.verrorFormat( where->coords, msg, ap );
  va_end( ap );
}
