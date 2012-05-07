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
#ifndef PS_SMALLS_CODEGEN_SIMPLECODEGEN_HPP
#define	PS_SMALLS_CODEGEN_SIMPLECODEGEN_HPP

#include "p1/util/gc-support.hpp"
#include "p1/util/format-str.hpp"
#include "p1/smalls/ast/SchemeAST.hpp"
#include <iostream>
#include <sstream>
#include <list>

namespace p1 {
namespace smalls {

class SimpleCodeGen : public gc
{
public:
  SimpleCodeGen();

  void setLineInfo ( bool on ) { m_optLineInfo = on; }

  void generate ( std::ostream & os, AstModule * module );

private:
  unsigned m_tmpIndex;
  bool m_optLineInfo;

  typedef std::list<const gc_char *,gc_allocator<const gc_char *> > LineList;

  class Func : public gc
  {
  public:
    SourceCoords const coords;
    const gc_char * const name;
    const gc_char * const decl;
    gc_string locals;
    gc_string contents;

    Func ( const SourceCoords & coords_, const gc_char * name_ )
      : coords(coords_), name(name_),
        decl( formatGCStr("static reg_t %s ( void )", name_) )
    {
      m_tmpIndex = 0;
    }

    const gc_char * nextTmp ( const char * type, const char * prefix )
    {
      const gc_char * name = formatGCStr( "%s%u", prefix, m_tmpIndex++ );
      if (type)
        locals += formatGCStr( "  %s %s;\n", type, name );
      return name;
    }

    void setContents ( const std::string & s )
    {
      this->contents.replace( this->contents.begin(), this->contents.end(), s.begin(), s.end() );
    }

  private:
    unsigned m_tmpIndex;
  };

  class Context : public gc
  {
  public:
    Context * const parent;
    Func * const func;
    const gc_char * const frametmp;
    AstFrame * const frame;

    Context ( Context * parent_, Func * func_, const gc_char * frametmp_, AstFrame * frame_)
      : parent(parent_), func(func_), frametmp(frametmp_), frame(frame_)
    {}
  };

  typedef std::list<Func, gc_allocator<Func> > FuncList;
  FuncList m_funcs;

  const gc_char * nextTmp ( const char * prefix )
  {
    return formatGCStr( "%s%u", prefix, m_tmpIndex++ );
  }

  Func * newFunc ( const SourceCoords & coords, const gc_char * name = 0 )
  {
    m_funcs.push_back( Func( coords, !name?nextTmp("func_"):name) );
    return &m_funcs.back();
  }

  void genTopLevel ( std::ostream & os, AstModule * module );
  Context * genSystem ( std::ostream & os, AstModule * module );

  const gc_char * genBody ( std::ostream & os, Context * parentCtx, Func * func, AstBody * body );
  const gc_char * gen ( std::ostream & os, Context * ctx, Ast * ast );
  const gc_char * genDatum ( std::ostream & os, Context * ctx, AstDatum * ast );
  const gc_char * genClosure ( std::ostream & os, Context * ctx, AstClosure * cl );
  const gc_char * genApply ( std::ostream & os, Context * ctx, AstApply * ap );
  const gc_char * genIf ( std::ostream & os, Context * ctx, AstIf * ia );

  struct VarData : public gc
  {
    unsigned addr;
    VarData ( unsigned addr_ ) : addr(addr_) {};
  };

  static VarData * varData ( AstVariable * var )
  {
    return static_cast<VarData*>(var->data);
  }

  std::string coords ( const SourceCoords & coords );
  std::string coords ( Ast * ast )
  {
    return coords( ast->coords );
  }

  void assignAddresses ( unsigned startAddr, AstFrame * frame );
};

}}

#endif	/* PS_SMALLS_CODEGEN_SIMPLECODEGEN_HPP */
