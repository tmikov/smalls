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
#include "SimpleCodeGen.hpp"
#include "p1/smalls/parser/Syntax.hpp"
#include <boost/foreach.hpp>
#include <sstream>

namespace p1 {
namespace smalls {

static const unsigned PARAM_COUNT = 32;

SimpleCodeGen::SimpleCodeGen ()
{
  m_tmpIndex = 0;
  m_optLineInfo = false;
}

void SimpleCodeGen::generate ( std::ostream & os, ast::AstModule * module )
{
  os << "#include \"smalls.h\"\n";
  os << "\n";

  genTopLevel( os, module );

  BOOST_FOREACH( Func & f, m_funcs )
  {
    os << f.decl << ";\n";
  }
  os << "\n\n";
  BOOST_FOREACH( Func & f, m_funcs )
  {
    os << coords(f.coords) << f.decl << " {\n";
    os << f.locals;
    if (!f.locals.empty())
      os << "\n";
    os << f.contents;
    os << "}\n\n";
  }
  os << "int main ( void ) {\n"
        "  return (int)module_init();\n"
        "}\n\n";
}

void SimpleCodeGen::genTopLevel ( std::ostream & os, ast::AstModule * module )
{
  Context * sysctx = genSystem( os, module );

  Func * f = newFunc( SourceCoords(), "module_init" );
  std::stringstream ss;
  const gc_char * restmp = genBody( ss, sysctx, f, module->body() );
  if (!restmp)
    restmp = "0";
  ss << "  return "<<restmp<<";\n";
  f->setContents( ss.str() );
}

SimpleCodeGen::Context * SimpleCodeGen::genSystem ( std::ostream & os, ast::AstModule * module )
{
  return NULL;
}

const gc_char * SimpleCodeGen::genBody ( std::ostream & os, Context * parentCtx, Func * func, ast::AstBody * body )
{
  Context * ctx = new Context( parentCtx, func, func->nextTmp("reg_t *", "frame_"), body->frame() );

  // Assign addresses to all variables in the frame
  assignAddresses( 1, ctx->frame );

  os << "  "<<ctx->frametmp<<" = (reg_t *)ALLOC( sizeof(reg_t)*" << ctx->frame->length()+1 << " );\n";
  if (parentCtx)
    os << "  "<<ctx->frametmp<<"[0] = (reg_t)"<<parentCtx->frametmp<<";\n";
  else
    os << "  "<<ctx->frametmp<<"[0] = (reg_t)0;\n";
  BOOST_FOREACH( ast::AstBody::Definition & defn, body->defs() )
  {
    const gc_char * tmp = gen( os, ctx, defn.second );
    if (defn.first && tmp)
      os << "  " << ctx->frametmp << "[" << varData(defn.first)->addr << "] = (reg_t)"<< tmp << "; //" << *defn.first << "\n";
  }

  const char * result = NULL;
  BOOST_FOREACH( ast::Ast & ast, body->exprList() )
    result = gen( os, ctx, &ast );
  return result;
}

const gc_char * SimpleCodeGen::gen ( std::ostream & os, Context * ctx, ast::Ast * ast )
{
  if (ast::AstClosure * cl = dyn_cast<ast::AstClosure>(ast))
    return genClosure( os, ctx, cl );
  else if (ast::AstApply * ap = dyn_cast<ast::AstApply>(ast))
    return genApply( os, ctx, ap );
  else if (ast::AstDatum * dt = dyn_cast<ast::AstDatum>(ast))
    return genDatum( os, ctx, dt );
  else if (isa<ast::AstUnspecified>(ast))
    return "0";
  else if (ast::AstVar * v = dyn_cast<ast::AstVar>(ast))
  {
    // Iteratively access parent frames
    Context * curCtx = ctx;
    const gc_char * frametmp = ctx->frametmp;
    for ( ; curCtx->frame != v->var->frame; curCtx = curCtx->parent )
    {
      const gc_char * tmp = ctx->func->nextTmp( "reg_t *", "pframe_" );
      os << coords(v) << "  "<<tmp<<" = (reg_t *)"<<frametmp<<"[0];\n";
      frametmp = tmp;

      assert( curCtx->parent );
    }

    std::stringstream sstmp;
    sstmp << *v->var;
    return formatGCStr( "%s[%u]/*%s*/", frametmp, varData(v->var)->addr, sstmp.str().c_str() );
  }
  else if (ast::AstIf * ia = dyn_cast<ast::AstIf>(ast))
    return genIf( os, ctx, ia );

  return formatGCStr( "?%s", ast::AstKind::name(ast->kind) );
}

const gc_char * SimpleCodeGen::genDatum ( std::ostream & os, Context * ctx, ast::AstDatum * ast )
{
  if (SyntaxValue * v = dyn_cast<SyntaxValue>(ast->datum))
  {
    switch (v->skind)
    {
    case SyntaxKind::REAL:
      return formatGCStr("?%f", v->u.real); // FIXME

    case SyntaxKind::INTEGER:
      return formatGCStr("MK_SMALLINT(%lld)", v->u.integer);

    case SyntaxKind::BOOL:
      return formatGCStr("MK_SMALLINT(%d)", v->u.vbool);

    default:
      // FIXME
      return formatGCStr("??%s", SyntaxKind::name(ast->datum->skind)); // FIXME
    }
  }
  else
    return formatGCStr("??%s", SyntaxKind::name(ast->datum->skind)); // FIXME
}

const gc_char * SimpleCodeGen::genClosure ( std::ostream & os, Context * ctx, ast::AstClosure * cl )
{
  std::stringstream ss;
  Func * cf = newFunc( cl->coords );
  Context * paramCtx = new Context(ctx,cf,cf->nextTmp("reg_t *", "params_"),cl->paramFrame);

  // Assign addresses to all variables in the frame
  assignAddresses( 1, paramCtx->frame );

  // Check the number of parameters
  unsigned pcount = cl->params ? cl->params->size() : 0;

  if (!cl->listParam)
  {
    // Must match exactly
    ss<<coords(cl)<<"  if (g_pcount != "<<pcount<<
        ") {error(\"Parameter count mismatch\"); return 0;}\n";
  }
  else
  {
    // Must have at least that many params
    if (pcount)
    {
      ss<<coords(cl)<<"  if (g_pcount < "<<pcount<<
          ") {error(\"Parameter count mismatch\"); return 0;}\n";
    }
  }

  // Allocate the parameter frame
  ss << "  "<<paramCtx->frametmp<<" = (reg_t *)ALLOC( sizeof(reg_t)*" << paramCtx->frame->length()+1 << " );\n";
  ss << "  "<<paramCtx->frametmp <<"[0] = (reg_t)g_penv;\n";

  // Extract all parameters into the frame
  if (pcount)
  {
    unsigned i = 0;
    BOOST_FOREACH( ast::Variable * param, *cl->params )
    {
      unsigned addr = varData(param)->addr;
      ss << "  " << paramCtx->frametmp << "[" << addr << "] = g_param["<<i<< "]; //" << *param << "\n";
      ++i;
    }
  }
  ss << "\n";

  // listParam handling. Must build a list from the actual params
  if (cl->listParam)
  {
    const gc_char * lptmp = cf->nextTmp("pair_t *", "pair_");
    const gc_char * ctmp = cf->nextTmp("int", "i_");
    ss<<coords(cl)<<"  "<<lptmp<<" = MK_PAIR(0);\n";
    ss<<coords(cl)<<"  for ( "<<ctmp<<" = g_pcount; --"<<ctmp<<" >= "<<pcount<<"; )\n";
    ss<<coords(cl)<<"    "<<lptmp<<" = make_pair( g_param["<<ctmp<<"], (reg_t)"<<lptmp<<" );\n";
    ss<<coords(cl)<<"  "<<paramCtx->frametmp<<"["<<varData(cl->listParam)->addr<< "] = (reg_t)"<<lptmp<<
        "; //"<<*cl->listParam<<"\n";
    ss << "\n";
  }

  const gc_char * rettmp = genBody( ss, paramCtx, cf, cl->body );
  if (rettmp)
    ss << "  return (reg_t)"<<rettmp<<";\n";
  else
    ss << "  return 0;\n";
  cf->setContents( ss.str() );

  const gc_char * cltmp = ctx->func->nextTmp( "closure_t *", "closure_" );
  os << "  "<<cltmp<<" = (closure_t*)ALLOC( sizeof(closure_t) );\n";
  os << "  "<<cltmp<<"->fp = "<<cf->name<<";\n";
  os << "  "<<cltmp<<"->env = "<< ctx->frametmp <<";\n";

  return cltmp;
}

const gc_char * SimpleCodeGen::genApply ( std::ostream & os, Context * ctx, ast::AstApply * ap )
{
  const gc_char * targtmp = gen( os, ctx, ap->target );
  const gc_char * cltmp = ctx->func->nextTmp( "closure_t *", "closure_" );
  const gc_char * result = ctx->func->nextTmp( "reg_t", "result_" );


  os << coords(ap->target) << "  "<<cltmp<<" = CHK_CLOSURE("<<targtmp<<");\n";

  // Store the parameter temporaries here
  std::vector<const gc_char *,gc_allocator<const gc_char*> > paramTmps;
  if (ap->params)
  {
    BOOST_FOREACH( ast::Ast * ast, *ap->params )
      paramTmps.push_back( gen( os, ctx, ast ) );
  }

  os << coords(ap->target) << "  g_penv = "<<cltmp<<"->env;\n";
  unsigned i = 0;
  if (ap->params)
  {
    BOOST_FOREACH( ast::Ast * ast, *ap->params )
    {
      const gc_char * tmp = paramTmps[i];
      if (!tmp)
        tmp = "0";
      os<<coords(ast)<<"  g_param["<<i<<"] = "<<tmp<<";\n";
      ++i;
    }
  }

  os<<coords(ap)<<"  g_pcount = "<<i<<";\n";
  if (ap->listParam)
  {
    const gc_char * listParam = gen(os, ctx, ap->listParam);
    const gc_char * ptmp = ctx->func->nextTmp( "pair_t *", "pair_" );
    os<<coords(ap)<<"  "<<ptmp<<" = CHK_PAIR("<<listParam<<")";
    os<<coords(ap)<<"  while ("<<ptmp<<") {\n";
    os<<coords(ap)<<"    if (g_pcount == GPARAM_COUNT) {error(\"Too many parameters\");break;}\n";
    os<<coords(ap)<<"    g_param[g_pcount++] = "<<ptmp<<"->car;\n";
    os<<coords(ap)<<"    "<<ptmp<<" = CHK_PAIR("<<ptmp<<"->cdr);\n";
    os<<coords(ap)<<"  }\n";
  }

  os <<coords(ap)<< "  "<<result<<" = (*"<<cltmp<<"->fp)();\n";

  return result;
}

const gc_char * SimpleCodeGen::genIf ( std::ostream & os, Context * ctx, ast::AstIf * ia )
{
  const gc_char * cndtmp = gen( os, ctx, ia->cond );
  const gc_char * exitlab = ctx->func->nextTmp(0,"lab_");
  const gc_char * elselab = ia->elseAst ? ctx->func->nextTmp(0,"lab_") : exitlab;

  os<<coords(ia)<<"  if (!"<<cndtmp<<") goto "<<elselab<<";\n";
  const gc_char * thentmp = gen( os, ctx, ia->thenAst );
  if (!ia->elseAst)
  {
    os<<exitlab<<":\n";
    return thentmp;
  }

  const gc_char * restmp = ctx->func->nextTmp( "reg_t", "tmp_" );
  os<<"  "<<restmp<<" = (reg_t)"<<thentmp<<";\n";
  os << "  goto "<<exitlab<<";\n";

  os<<elselab<<":\n";
  const gc_char * elsetmp = gen( os, ctx, ia->elseAst );
  os<<"  "<<restmp<<" = (reg_t)"<<elsetmp<<";\n";
  os<<exitlab<<":\n";
  return restmp;
}

std::string SimpleCodeGen::coords ( const SourceCoords & coords )
{
  if (m_optLineInfo && coords.full())
  {
    std::stringstream os;
    os << "#line " << coords.line << " \"" << coords.fileName << "\" " << " // column:" << coords.column << '\n';
    return os.str();
  }
  else
    return "";
}

void SimpleCodeGen::assignAddresses ( unsigned startAddr, ast::Frame * frame )
{
  // Assign addresses to all variables in the frame
  BOOST_FOREACH( ast::Variable & var, *frame )
  {
    assert( var.data == NULL && "Variable already assigned an address" );
    var.data = new (GC) VarData( startAddr++ );
  }
}

}} // namespaces
