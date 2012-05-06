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
#include "SchemeParser.hpp"
#include "ListBuilder.hpp"
#include "p1/util/scopeguard.hpp"
#include <boost/foreach.hpp>
#include <iostream>

using namespace p1;
using namespace p1::smalls;
using namespace p1::smalls::detail;


ReservedBindings::ReservedBindings ( SymbolTable & map, Scope * sc ) :
  scope( sc ),
  sym( map ),
  bind_quote             ( bind( scope, sym.sym_quote, ResWord::QUOTE ) ),
  bind_quasiquote        ( bind( scope, sym.sym_quasiquote, ResWord::NONE /*SymCode::QUASIQUOTE*/ ) ),
  bind_unquote           ( bind( scope, sym.sym_unquote, ResWord::NONE /*SymCode::UNQUOTE*/ ) ),
  bind_unquote_splicing  ( bind( scope, sym.sym_unquote_splicing, ResWord::NONE /*SymCode::UNQUOTE_SPLICING*/ ) ),
  bind_syntax            ( bind( scope, sym.sym_syntax, ResWord::SYNTAX ) ),
  bind_quasisyntax       ( bind( scope, sym.sym_quasisyntax, ResWord::QUASISYNTAX ) ),
  bind_unsyntax          ( bind( scope, sym.sym_unsyntax, ResWord::UNSYNTAX ) ),
  bind_unsyntax_splicing ( bind( scope, sym.sym_unsyntax_splicing, ResWord::UNSYNTAX_SPLICING ) ),

  bind_if                ( bind( scope, sym.sym_if, ResWord::IF ) ),
  bind_begin             ( bind( scope, sym.sym_begin, ResWord::BEGIN ) ),
  bind_lambda            ( bind( scope, sym.sym_lambda, ResWord::LAMBDA ) ),
  bind_define            ( bind( scope, sym.sym_define, ResWord::DEFINE ) ),
  bind_setbang           ( bind( scope, sym.sym_setbang, ResWord::SETBANG ) ),
  bind_let               ( bind( scope, sym.sym_let, ResWord::LET ) ),
  bind_letrec            ( bind( scope, sym.sym_letrec, ResWord::LETREC ) ),
  bind_letrec_star       ( bind( scope, sym.sym_letrec_star, ResWord::LETREC_STAR ) ),

  bind_builtin           ( bind( scope, sym.sym_builtin, ResWord::BUILTIN ) ),
  bind_define_macro      ( bind( scope, sym.sym_define_macro, ResWord::DEFINE_MACRO ) ),
  bind_define_identifier_macro ( bind( scope, sym.sym_define_identifier_macro, ResWord::DEFINE_IDENTIFIER_MACRO ) ),
  bind_define_set_macro  ( bind( scope, sym.sym_define_set_macro, ResWord::DEFINE_SET_MACRO ) ),
  bind_macro_env         ( bind( scope, sym.sym_macro_env, ResWord::MACRO_ENV ) )
{}

Binding * ReservedBindings::bind ( Scope * scope, Symbol * sym, ResWord::Enum resCode )
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

class MacroOr : public Macro
{
  SymbolTable & m_symbolTable;

public:
  MacroOr ( Scope * scope_, SymbolTable & symbolTable )
   : Macro( scope_ ), m_symbolTable( symbolTable )
  {}

  virtual Syntax * expand ( Syntax * datum );
};

#if 0
class MacroTest : public Macro
{
  SymbolTable & m_symbolTable;

public:
  MacroTest ( Scope * scope_, SymbolTable & symbolTable )
   : Macro( scope_ ), m_symbolTable( symbolTable )
  {}

  virtual Syntax * expand ( Syntax * datum );
};
#endif

SchemeParser::SchemeParser ( SymbolTable & symbolTable, AbstractErrorReporter & errors )
  : m_symbolTable( symbolTable ),
    m_systemScope( m_symbolTable.newScope() ),
    m_rsv( symbolTable, m_systemScope ),
    m_errors( errors ),
    m_antiMark( new Mark( -1, NULL, NULL ) )
{
  // Create a synthetic binding for the unspecified value
  m_unspec = new Binding( m_symbolTable.newSymbol( "#unspecified" ), m_systemScope, SourceCoords() );
  m_unspec->bindResWord( ResWord::UNSPECIFIED );

  Binding * orb;
  m_systemScope->bind( orb, m_symbolTable.newSymbol("or"), SourceCoords() );
  orb->bindMacro( new MacroOr( m_systemScope, m_symbolTable ) );
#if 0
  m_systemScope->bind( orb, m_symbolTable.newSymbol("test"), BindingKind::MACRO, SourceCoords() );
  orb->m_u.macro = new MacroTest( m_systemScope, m_symbolTable );
#endif
}

SchemeParser::~SchemeParser ()
{
}

ListOfAst SchemeParser::compileLibraryBody ( Syntax * datum )
{
  Context * ctx = new Context( m_symbolTable.newScope(), new AstFrame(NULL) );
  return compileBody( ctx, datum );
}

ListOfAst SchemeParser::compileBody ( SchemeParser::Context * ctx, Syntax * datum )
{
  parseBody( ctx, datum );
  return convertLetRecStar( ctx );
}

void SchemeParser::parseBody ( SchemeParser::Context * ctx, Syntax * datum )
{
  while (!isa<SyntaxNil>(datum))
  {
    if (SyntaxPair * p = needPair("",datum))
    {
      processBodyForm( ctx, p->car() );
      datum = p->cdr();
    }
    else
      break;
  }
}

void SchemeParser::processBodyForm ( SchemeParser::Context * ctx, Syntax * datum )
{
tail_recursion:
  if (isa<SyntaxNil>(datum))
  {
    error( datum, "Invalid empty form" );
    return;
  }

  if (SyntaxPair * pair = dyn_cast<SyntaxPair>( datum ))
  {
    Syntax * car = pair->car();
    if (Binding * binding = isBinding(car))
    {
      if (binding->kind() == BindingKind::MACRO)
      {
        datum = expandMacro( ctx, binding->macro(), pair );
        if (!datum) // error?
          return;
        goto tail_recursion;
      }
      else if (binding->kind() == BindingKind::RESWORD)
      {
        switch (binding->resWord())
        {
        case ResWord::BEGIN:
          // splice the body of (begin ...)
          parseBody( ctx, pair->cdr() );
          return;
        case ResWord::DEFINE:
          recordDefine( ctx, pair );
          return;
        default:
          break;
        }
      }
    }
  }

  // Defer the expression
  ctx->exprList.push_back( datum );
}

void SchemeParser::recordDefine ( SchemeParser::Context * ctx, SyntaxPair * form )
{
  if (!ctx->topLevel() && !ctx->exprList.empty())
  {
    error( form, "Definition not allowed here" );
    return;
  }

  if (!ctx->exprList.empty())
  {
    // Convert all deferred expressions to (define <unused> (begin expression... #undefined))
    ListBuilder lb;
    SourceCoords & coords = (*ctx->exprList.begin())->coords;
    lb << new SyntaxBinding(coords, m_rsv.bind_begin);
    BOOST_FOREACH( Syntax * expr, ctx->exprList )
      lb << expr;
    lb << new SyntaxBinding(coords, m_unspec);

    ctx->defnList.push_back( DeferredDefine( NULL, lb ) );

    ctx->exprList.clear();
  }

  Syntax * p0;
  SyntaxPair * restp;

  if (!needParams( "define", form->cdr(), 1, &p0, &restp ))
    return;

  Binding * bnd;
  if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(p0))
  {
    if (bindSyntaxSymbol( bnd, ctx->scope, ss ))
    {
      bnd->bindVar( ctx->frame->newVariable( bnd->sym->name ) );
    }
    else
    {
      error( p0, "'%s' already defined at %s", ss->symbol->name, bnd->defCoords().toString().c_str() );
      bnd = NULL;
    }
  }
  else
  {
    error( p0, "symbol required after \"define\"");
    return;
  }

  Syntax * value;
  if (isa<SyntaxNil>(restp))
    value = new SyntaxBinding( restp->coords, m_unspec );
  else
  {
    value = restp->car();
    if (!isa<SyntaxNil>(restp->cdr()))
      error( restp->cdr(), "define must specify only one value" );
  }

  // Append to the list of deferred definitions
  ctx->defnList.push_back( DeferredDefine(bnd,value) );
}

Syntax * SchemeParser::expandMacro ( Context * ctx, Macro * macro, SyntaxPair * pair )
{
  Syntax * wrapped = pair->wrap( m_antiMark );
  std::cout << "\nWrapped:\n" << *wrapped << "\n";
  // FIXME: validate the result - make sure it is non-cyclic
  Syntax * expanded;
  try
  {
    expanded = macro->expand( wrapped );
    std::cout << "\nExpanded:\n" << *expanded << "\n";
  }
  catch (ErrorInfo & ei)
  {
    m_errors.error( ei );
    return NULL;
  }
  Syntax * result = expanded->wrap( new Mark(m_symbolTable.nextMarkStamp(), macro->scope, NULL) );
  std::cout << "\nResult:\n" << *result << "\n";
  std::cout << "\nExpanded-Result:\n" << *unwrapCompletely(result) << "\n\n\n";
  return result;
}

/*
 (define-syntax or
   (syntax-rules ()
     ((_) #t)
     ((_ a) a)
     ((_ a b c ...) (let ((tmp a)) (if tmp tmp (or b c ...))))))
 */
Syntax * MacroOr::expand ( Syntax * datum )
{
  Syntax * s = datum;
  SyntaxPair * pair;

  if (isa<SyntaxNil>(s))
    throw ErrorInfo( s->coords, "Invalid macro pattern" );
  if (!(pair = dyn_cast<SyntaxPair>(s)))
    throw ErrorInfo( s->coords, "Invalid macro pattern" );

  //Syntax * s0 = pair->car->access();
  s = pair->cdr();

  if (isa<SyntaxNil>(s))
    return new SyntaxValue( SyntaxKind::BOOL, datum->coords, true );

  if (!(pair = dyn_cast<SyntaxPair>(s)))
    throw ErrorInfo( s->coords, "Invalid macro pattern" );

  Syntax * s1 = pair->car();
  s = pair->cdr();

  if (isa<SyntaxNil>(s))
    return s1;

  if (!(pair = dyn_cast<SyntaxPair>(s)))
    throw ErrorInfo( s->coords, "Invalid macro pattern" );

  Syntax * s2 = pair->car();
  Syntax * rest = pair->cdr();

  ListBuilder let;
  let << new SyntaxSymbol( datum->coords, m_symbolTable.newSymbol( "let" ) );

  ListBuilder init;
  ListBuilder init1;
  init1 << new SyntaxSymbol( datum->coords, m_symbolTable.newSymbol("tmp") ) << s1;
  init << init1;
  let << init;

  ListBuilder ifl;
  ifl << new SyntaxSymbol( datum->coords, m_symbolTable.newSymbol( "if" ) )
      << new SyntaxSymbol( datum->coords, m_symbolTable.newSymbol( "tmp" ) )
      << new SyntaxSymbol( datum->coords, m_symbolTable.newSymbol( "tmp" ) );

  ListBuilder elsel;
  elsel << new SyntaxSymbol( datum->coords, m_symbolTable.newSymbol( "or" ) )
        << s2;
  ifl << elsel.toList( rest );
  let << ifl;

  return let.toList();
}

ListOfAst SchemeParser::convertLetRecStar ( Context * ctx )
{
  if (ctx->defnList.empty())
  {
    ListOfAst body;

    BOOST_FOREACH( Syntax * expr, ctx->exprList )
    {
      ListOfAst tmp = compileExpression( ctx, expr );
      body.destructiveAppend( tmp );
    }

    if (body.empty())
      body += new Ast(AstKind::UNSPECIFIED, SourceCoords());

    return body;
  }
  else
  {
  /*
  Trivial conversion to let.

    (define-syntax letrec*
      (syntax-rules ()
        ((letrec* ((var1 init1) ...) body1 body2 ...)
          (let ((var1 <undefined>) ...)
            (set! var1 init1)
            ...
            (let () body1 body2 ...)))))
  */

    // Note that we don't generate the inner "(let () body1 body2 ...)" because we know
    // for sure that it isn't necessary in our case (not general letrec*, but generating the body;
    // we know there are no definitions in the body).

    VectorOfVariable * vars = new (GC) VectorOfVariable();
    VectorOfListOfAst * values = new (GC) VectorOfListOfAst();
    AstFrame * paramFrame = ctx->frame;

    // Build the let initialization
    BOOST_FOREACH( DeferredDefine & defn, ctx->defnList )
    {
      if (defn.first)
        vars->push_back( defn.first->var() );
      else
        vars->push_back( ctx->frame->newAnonymous("") );
      values->push_back( makeUnspecified(defn.second) );
    }

    // Create a dummy body frame
    ctx->frame = new AstFrame(ctx->frame);

    ListOfAst body;
    VectorOfVariable::const_iterator vit = vars->begin();
    DeferredDefineList::const_iterator dit = ctx->defnList.begin();
    DeferredDefineList::const_iterator dit_end = ctx->defnList.end();
    for ( ; dit != dit_end; ++vit, ++dit )
    {
      const DeferredDefine & defn = *dit;
      AstVariable * var = *vit;

      body += new AstSet( defn.second->coords, var, compileExpression( ctx, defn.second ) );
    }

    BOOST_FOREACH( Syntax * expr, ctx->exprList )
    {
      ListOfAst tmp = compileExpression( ctx, expr );
      body.destructiveAppend( tmp );
    }

    SourceCoords coords;
    if (!ctx->defnList.empty())
      coords = ctx->defnList.front().second->coords;

    if (body.empty())
      body += new Ast(AstKind::UNSPECIFIED, coords);

    return makeListOfAst(new AstLet(
      coords,
      paramFrame->parent,
      vars,
      paramFrame,
      ctx->frame, // a dummy body frame
      body,
      values
    ));
  }
}

ListOfAst SchemeParser::compileExpression ( SchemeParser::Context * ctx, Syntax * expr )
{
tail_recursion:
  if (SyntaxValue * sv = dyn_cast<SyntaxValue>(expr))
  {
    return makeListOfAst( new AstDatum( expr->coords, sv ) );
  }
  else if (SyntaxVector * svec = dyn_cast<SyntaxVector>(expr))
  {
    (void)svec; // prevent unused warning
    // FIXME
    assert( false && "FIXME: implement vector" );
  }
  else if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(expr))
  {
    if (Binding * bnd = lookupSyntaxSymbol(ss))
      return compileBinding( ctx, bnd, expr );
    else
      error( expr, "Undefined variable '%s'", ss->symbol->name );
  }
  else if (SyntaxBinding * sb = dyn_cast<SyntaxBinding>(expr))
  {
    return compileBinding( ctx, sb->bnd, expr );
  }
  else if (SyntaxPair * pair = dyn_cast<SyntaxPair>(expr))
  {
    // Check for reserved words
    if (Binding * bnd = isBinding(pair->car()))
    {
      if (bnd->kind() == BindingKind::MACRO)
      {
        expr = expandMacro( ctx, bnd->macro(), pair );
        if (!expr) // error?
          goto unspec;
        goto tail_recursion;
      }
      else if (bnd->kind() == BindingKind::RESWORD)
        return compileResForm( ctx, pair, bnd );
    }
    // This is a function application
    return compileCall( ctx, pair );
  }
  else
  {
    error( expr, "Invalid expression" );
  }

unspec:
  return makeUnspecified(expr);
}

ListOfAst SchemeParser::compileBinding ( Context * ctx, Binding * bnd, Syntax * exprForCoords )
{
  if (bnd != m_unspec)
  {
    if (bnd->kind() == BindingKind::VAR)
      return makeListOfAst( new AstVar(exprForCoords->coords, bnd->var()) );
    else
      error( exprForCoords, "Undefined variable '%s'", bnd->sym->name );
  }
  return makeUnspecified(exprForCoords);
}


ListOfAst SchemeParser::compileCall ( SchemeParser::Context * ctx, SyntaxPair * pair )
{
  ListOfAst target = compileExpression( ctx, pair->car() );
  VectorOfListOfAst * params = new VectorOfListOfAst();
  Syntax * n = pair->cdr();
  while (!isa<SyntaxNil>(n))
  {
    SyntaxPair * expr = needPair( "", n );
    if (!expr)
      break;
    params->push_back( compileExpression( ctx, expr->car() ) );
    n = expr->cdr();
  }

  return makeListOfAst( new AstApply( pair->coords, target, params, NULL ) );
}


/**
 * Compile a (reserved-word ...) form
 */
ListOfAst SchemeParser::compileResForm ( SchemeParser::Context * ctx, SyntaxPair * pair, Binding * bndCar )
{
  switch (bndCar->resWord())
  {
  case ResWord::BEGIN: return compileBegin( ctx, pair );
  case ResWord::SETBANG: return compileSetBang( ctx, pair );
  case ResWord::IF: return compileIf( ctx, pair );
  case ResWord::LAMBDA: return compileLambda( ctx, pair );

  case ResWord::LET: return compileLet( ctx, pair );
  case ResWord::LETREC:
  case ResWord::LETREC_STAR:

  // case ResWord::QUOTE: // FIXME

  default:
    error( pair->car(), "Invalid form" );
    return makeUnspecified(pair);
  }
}

ListOfAst SchemeParser::compileBegin ( SchemeParser::Context * ctx, SyntaxPair * beginPair )
{
  ListOfAst result;
  Syntax * n = beginPair->cdr();

  // Don't return an empty list
  if (isa<SyntaxNil>(n))
    return makeUnspecified(beginPair);

  while (!isa<SyntaxNil>(n))
  {
    SyntaxPair * p = needPair( "", n );
    if (!p)
    {
      // We never want to return an empty AST
      if (result.empty())
        result = makeUnspecified(beginPair);
    }

    n = p->cdr();
    ListOfAst tmp = compileExpression( ctx, p->car() );
    result.destructiveAppend( tmp );
  }

  return result;
}

ListOfAst SchemeParser::compileSetBang ( SchemeParser::Context * ctx, SyntaxPair * setPair )
{
  Syntax * ps[2];

  if (!needParams( "set!", setPair->cdr(), 2, ps, NULL ))
    return makeUnspecified(setPair);

  // Target
  //
  Binding * bnd;
  if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(ps[0]))
  {
    bnd = lookupSyntaxSymbol(ss);
    if (!bnd)
    {
      error( ps[0], "Undefined variable '%s'", ss->symbol->name);
      return makeUnspecified(setPair);
    }
  }
  else if (SyntaxBinding * sb = dyn_cast<SyntaxBinding>(ps[0]))
    bnd = sb->bnd;
  else
  {
    error( ps[0], "set! requires a variable" );
    return makeUnspecified(setPair);
  }

  if (bnd->kind() != BindingKind::VAR)
  {
    error( ps[0], "Undefined variable '%s'", bnd->sym->name );
    return makeUnspecified(setPair);
  }

  // Value
  //
  ListOfAst value = compileExpression( ctx, ps[1] );

  return makeListOfAst(new AstSet( setPair->car()->coords, bnd->var(), value ));
}

ListOfAst SchemeParser::compileIf ( SchemeParser::Context * ctx, SyntaxPair * ifPair )
{
  Syntax * ps[2];
  SyntaxPair * restp;

  if (!needParams( "if", ifPair->cdr(), 2, ps, &restp ))
    return makeUnspecified(ifPair);

  ListOfAst cond = compileExpression( ctx, ps[0] );
  ListOfAst thenAst = compileExpression( ctx, ps[1] );
  ListOfAst elseAst;

  if (isa<SyntaxNil>(restp))
    elseAst += new Ast(AstKind::UNSPECIFIED, restp->coords );
  else
  {
    elseAst = compileExpression( ctx, restp->car() );

    if (!isa<SyntaxNil>(restp->cdr()))
      error( restp->cdr(), "if: form list is too long" );
  }

  return makeListOfAst( new AstIf( ifPair->car()->coords, cond, thenAst, elseAst ) );
}

ListOfAst SchemeParser::compileLambda ( SchemeParser::Context * ctx, SyntaxPair * lambdaPair )
{
  Syntax * p0;
  SyntaxPair * restp;

  if (!needParams( "lambda", lambdaPair->cdr(), 1, &p0, &restp ))
    return makeUnspecified(lambdaPair);

  VectorOfVariable * vars = new (GC) VectorOfVariable();
  AstVariable * listParam = NULL;

  Scope * paramScope = m_symbolTable.newScope();
  ON_BLOCK_EXIT_OBJ( m_symbolTable, &SymbolTable::popThisScope, paramScope );
  AstFrame * paramFrame = new AstFrame( ctx->frame );

  if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(p0)) // one formal parameter will accept a list of actual parameters
  {
    Binding * bnd;
    bindSyntaxSymbol( bnd, paramScope, ss );
    bnd->bindVar( paramFrame->newVariable( bnd->sym->name ) );
    listParam = bnd->var();
  }
  else if (SyntaxPair * params = dyn_cast<SyntaxPair>(p0)) // a list of formal parameters
  {
    for(;;)
    {
      Syntax * curParam = params->car();
      if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(curParam))
      {
        Binding * bnd;
        if (bindSyntaxSymbol( bnd, paramScope, ss ))
        {
          bnd->bindVar( paramFrame->newVariable( bnd->sym->name ) );
          vars->push_back( bnd->var() );
        }
        else
        {
          error( curParam, "Duplicated lambda parameter '%s'", ss->symbol->name );
          vars->push_back( paramFrame->newAnonymous( ss->symbol->name ) );
        }
      }
      else
        error( curParam, "Lambda parameter is not an identifier" );

      if (SyntaxPair * next = dyn_cast<SyntaxPair>(params->cdr()))
        params = next;
      else
        break;
    }

    if (!isa<SyntaxNil>(params->cdr()))
    {
      Syntax * curParam = params->cdr();
      if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(curParam))
      {
        Binding * bnd;
        if (bindSyntaxSymbol( bnd, paramScope, ss ))
        {
          bnd->bindVar( paramFrame->newVariable( bnd->sym->name ) );
          listParam = bnd->var();
        }
        else
        {
          error( curParam, "Duplicated lambda parameter '%s'", ss->symbol->name );
          listParam = paramFrame->newAnonymous( ss->symbol->name );
        }
      }
      else
        error( curParam, "Lambda parameter is not an identifier" );
    }
  }
  else
  {
    error( p0, "'lambda' requires at least a parameter list and a body" );
    return makeUnspecified(lambdaPair);
  }


  ListOfAst body;
  Context * bodyCtx = new Context( m_symbolTable.newScope(), new AstFrame(paramFrame) );
  ON_BLOCK_EXIT_OBJ( m_symbolTable, &SymbolTable::popThisScope, bodyCtx->scope );

  if (!isa<SyntaxNil>(restp))
    body = compileBody( bodyCtx, restp );
  else
  {
    error( restp, "lambda requires a body" );
    body = makeUnspecified( restp );
  }

  return makeListOfAst(new AstClosure(
    lambdaPair->car()->coords,
    ctx->frame,
    vars,
    listParam,
    paramFrame,
    bodyCtx->frame,
    body
  ));
}

ListOfAst SchemeParser::compileLet ( SchemeParser::Context * ctx, SyntaxPair * letPair )
{
  if (SyntaxPair * p = dyn_cast<SyntaxPair>(letPair->cdr()))
    if (p->car()->skind == SyntaxKind::PAIR || isa<SyntaxNil>(p->car()))
      return compileBasicLet( ctx, letPair );
//    else if (p->car->sclass == SyntaxClass::SYMBOL)
//      return compileNamedLet( ctx, letPair );

  error( letPair->cdr(), "Invalid let form" );
  return makeUnspecified(letPair);
}

ListOfAst SchemeParser::compileBasicLet ( SchemeParser::Context * ctx, SyntaxPair * letPair )
{
  Syntax * p0;
  SyntaxPair * restp;

  if (!needParams( "let", letPair->cdr(), 1, &p0, &restp) )
    return makeUnspecified(letPair);

  DatumList varDatums;
  DatumList valueDatums;

  if (isa<SyntaxNil>(p0))
  {
    // Empty init list
  }
  else if (SyntaxPair * inits = dyn_cast<SyntaxPair>(p0)) // a list of (symbol init) pairs
  {
    for(;;)
    {
      Syntax * dt[2];
      if (needParams( "let initialization", inits->car(), 2, dt, NULL ))
      {
        varDatums.push_back( dt[0] );
        valueDatums.push_back( dt[1] );
      }

      if (SyntaxPair * next = dyn_cast<SyntaxPair>(inits->cdr()))
        inits = next;
      else if (isa<SyntaxNil>(inits->cdr()))
        break;
      else
      {
        error( inits->cdr(), "let: form must be a proper list" );
        break;
      }
    }
  }
  else
  {
    error( p0, "'let' requires an initialization list" );
    return makeUnspecified(letPair);
  }

  // Compile the initialization expressions
  VectorOfListOfAst * values = new (GC) VectorOfListOfAst();

  BOOST_FOREACH( Syntax * expr, valueDatums )
    values->push_back( compileExpression( ctx, expr ) );

  // Parse the newly created variables
  //
  VectorOfVariable * vars = new (GC) VectorOfVariable();

  Scope * paramScope = m_symbolTable.newScope();
  ON_BLOCK_EXIT_OBJ( m_symbolTable, &SymbolTable::popThisScope, paramScope );
  AstFrame * paramFrame = new AstFrame( ctx->frame );

  BOOST_FOREACH( Syntax * curParam, varDatums )
  {
    if (SyntaxSymbol * ss = dyn_cast<SyntaxSymbol>(curParam))
    {
      Binding * bnd;
      if (bindSyntaxSymbol( bnd, paramScope, ss ))
      {
        bnd->bindVar( paramFrame->newVariable( bnd->sym->name ) );
        vars->push_back( bnd->var() );
      }
      else
      {
        error( curParam, "let: duplicated variable '%s'", ss->symbol->name );
        vars->push_back( paramFrame->newAnonymous( ss->symbol->name ) );
      }
    }
    else
      error( curParam, "let: init variable must be an identifier" );
  }

  // Parse the body
  //
  ListOfAst body;
  Context * bodyCtx = new Context( m_symbolTable.newScope(), new AstFrame(paramFrame) );
  ON_BLOCK_EXIT_OBJ( m_symbolTable, &SymbolTable::popThisScope, bodyCtx->scope );

  if (!isa<SyntaxNil>(restp))
    body = compileBody( bodyCtx, restp );
  else
  {
    error( restp, "let requires a body" );
    body = makeUnspecified( restp );
  }

  return makeListOfAst(new AstLet(
    letPair->car()->coords,
    ctx->frame,
    vars,
    paramFrame,
    bodyCtx->frame,
    body,
    values
  ));
}

// FIXME
ListOfAst SchemeParser::compileNamedLet ( SchemeParser::Context * ctx, SyntaxPair * letPair )
{
  Syntax * ps[2];
  SyntaxPair * restp;

  if (!needParams( "let", letPair->cdr(), 2, ps, &restp) )
    return makeUnspecified(letPair);

  DatumList varDatums;
  DatumList valueDatums;

  if (!splitLetParams( ps[1], varDatums, valueDatums ))
    return makeUnspecified(letPair);

  return makeUnspecified(letPair);
}

bool SchemeParser::splitLetParams ( Syntax * p0, DatumList & varDatums, DatumList & valueDatums )
{
  if (isa<SyntaxNil>(p0))
  {
    // Empty init list
  }
  else if (SyntaxPair * inits = dyn_cast<SyntaxPair>(p0)) // a list of (symbol init) pairs
  {
    for(;;)
    {
      Syntax * dt[2];
      if (needParams( "let initialization", inits->car(), 2, dt, NULL ))
      {
        varDatums.push_back( dt[0] );
        valueDatums.push_back( dt[1] );
      }

      if (SyntaxPair * next = dyn_cast<SyntaxPair>(inits->cdr()))
        inits = next;
      else if (isa<SyntaxNil>(inits->cdr()))
        break;
      else
      {
        error( inits->cdr(), "let: form must be a proper list" );
        break;
      }
    }
  }
  else
  {
    error( p0, "'let' requires an initialization list" );
    return false;
  }

  return true;
}

ListOfAst SchemeParser::makeUnspecified ( Syntax * where )
{
  return makeListOfAst( new Ast(AstKind::UNSPECIFIED, where->coords) );
}

bool SchemeParser::needParams ( const char * formName, Syntax * datum, unsigned np, Syntax ** params, SyntaxPair ** restp )
{
  if (formName && !*formName) // convert "" to NULL
    formName = NULL;

  // Clear the output parameters
  if (params)
    for ( unsigned ti = 0; ti != np; ++ti )
      params[ti] = NULL;
  if (restp)
    *restp = NULL;

  for ( unsigned i = 0; i != np; ++i )
  {
    if (SyntaxPair * p = dyn_cast<SyntaxPair>(datum))
    {
      params[i] = p->car();
      datum = p->cdr();
    }
    else
    {
      if (datum->skind == SyntaxKind::NIL)
        error( datum, "%s%s" "form list is too short", formName?formName:"", formName?":":"" );
      else
        error( datum, "%s%s" "form must be a proper list", formName?formName:"", formName?":":"" );
      return false;
    }
  }

  if (!restp)
  {
    if (datum->skind != SyntaxKind::NIL)
    {
      error( datum, "%s%s" "form list is too long", formName?formName:"", formName?":":"" );
      return false;
    }
  }
  else
  {
    if (isa<SyntaxNil>(datum))
      *restp = cast<SyntaxNil>(datum);
    else if (isa<SyntaxPair>(datum))
      *restp = cast<SyntaxPair>(datum);
    else
    {
      error( datum, "%s%s" "form must be a proper list", formName?formName:"", formName?":":"" );
      return false;
    }
  }

  return true;
}

bool SchemeParser::bindSyntaxSymbol (
    Binding * & res, Scope * scope, SyntaxSymbol * ss
  )
{
  return scope->bind( res, ss->symbol, ss->coords );
}

Binding * SchemeParser::lookupSyntaxSymbol ( SyntaxSymbol * ss )
{
  Binding * bnd;

  if ( (bnd = m_symbolTable.lookup(ss->symbol)) != NULL)
    return bnd;

  // Check for the parent symbol in the mark's scope and go up the mark chain
  if (Mark * mark = ss->mark)
  {
    Symbol * symbol = ss->symbol;
    do
    {
      if (mark->isMark())
      {
        symbol = symbol->parentSymbol;
        if ( (bnd = mark->scope->lookupHereAndUp( symbol )) != NULL)
          return bnd;
      }
    }
    while ( (mark = mark->next) != NULL);
  }

  return NULL;
}

SyntaxPair * SchemeParser::needPair ( const char * formName, Syntax * datum )
{
  if (formName && !*formName) // convert "" to NULL
    formName = NULL;

  if (SyntaxPair * pair = dyn_cast<SyntaxPair>(datum))
    return pair;
  else if (isa<SyntaxNil>(datum))
    error( datum, "%s%s" "form list is too short", formName?formName:"", formName?":":"" );
  else
    error( datum, "%s%s" "form must be a proper list", formName?formName:"", formName?":":"" );
  return NULL;
}

bool SchemeParser::needNil ( const char * formName, Syntax * datum )
{
  if (formName && !*formName) // convert "" to NULL
    formName = NULL;

  if (isa<SyntaxNil>(datum))
    return true;
  else
  {
    error( datum, "%s%s" "form list is too long", formName?formName:"", formName?":":"" );
    return false;
  }
}

void SchemeParser::error ( Syntax * where, const char * msg, ... )
{
  std::va_list ap;
  va_start( ap, msg );
  m_errors.verrorFormat( where->coords, msg, ap );
  va_end( ap );
}
