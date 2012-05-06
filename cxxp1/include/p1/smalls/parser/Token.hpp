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
===============================================================================
   Token encapsulates all data returned by the Lexer for every token.
 */
#ifndef P1_SMALLS_PARSER_TOKEN_HPP
#define	P1_SMALLS_PARSER_TOKEN_HPP

#include "p1/smalls/common/SourceCoords.hpp"
#include "p1/util/gc-support.hpp"
#include <cassert>
#include <stdint.h>

namespace p1 {
namespace smalls {

class Symbol;

#define _DEF_TOKENS \
   _MK_ENUM(NONE,"<NONE>") \
   _MK_ENUM(EOFTOK,"<EOF>") \
   _MK_ENUM(SYMBOL,"symbol") \
   _MK_ENUM(BOOL,"#t or #f") \
   _MK_ENUM(REAL,"real") \
   _MK_ENUM(INTEGER,"integer") \
   _MK_ENUM(STR,"string") \
   _MK_ENUM(LPAR,"(") _MK_ENUM(RPAR,")") \
   _MK_ENUM(LSQUARE,"[") _MK_ENUM(RSQUARE,"]") \
   _MK_ENUM(HASH_LPAR,"#(") \
   _MK_ENUM(APOSTR,"'") _MK_ENUM(ACCENT,"`") \
   _MK_ENUM(COMMA,",") \
   _MK_ENUM(COMMA_AT,",@") \
   _MK_ENUM(DOT,".") \
   _MK_ENUM(HASH_APOSTR,"#'") \
   _MK_ENUM(HASH_ACCENT,"#`") \
   _MK_ENUM(HASH_COMMA,"#,") \
   _MK_ENUM(HASH_COMMA_AT,"#,@") \
   _MK_ENUM(DATUM_COMMENT,"#;") \
   \
   _MK_ENUM(NESTED_COMMENT_START,"#|") /* internal use only! */ \
   _MK_ENUM(NESTED_COMMENT_END,"|#")   /* internal use only! */

struct TokenKind
{
  #define _MK_ENUM(name,repr)  name,
  enum Enum
  {
    _DEF_TOKENS
  };
  #undef _MK_ENUM

  static const char * name ( Enum x ) { return s_names[x]; }
  static const char * repr ( Enum x ) { return s_reprs[x]; }

private:
  static const char * s_names[];
  static const char * s_reprs[];
};


class Token
{
  TokenKind::Enum m_kind;
  SourceCoords m_coords;

  union
  {
    const gc_char * string;
    Symbol * symbol;
    int64_t integer;
    double  real;
    bool    vbool;
  } m_value;

public:
  Token()
  {
    m_kind = TokenKind::NONE;
  };

  void kind ( TokenKind::Enum kind ) { m_kind = kind; }
  TokenKind::Enum kind () const { return m_kind; }

  void coords ( const SourceCoords & coords ) { m_coords = coords; }
  const SourceCoords & coords () const { return m_coords; }

  void string ( const gc_char * string )
  {
    m_kind = TokenKind::STR;
    m_value.string = string;
  }
  const gc_char * string () const
  {
    assert( m_kind == TokenKind::STR );
    return m_value.string;
  }

  void symbol ( Symbol * symbol )
  {
    m_kind = TokenKind::SYMBOL;
    m_value.symbol = symbol;
  }
  Symbol * symbol () const
  {
    assert( m_kind == TokenKind::SYMBOL );
    return m_value.symbol;
  }

  void integer ( int64_t integer )
  {
    m_kind = TokenKind::INTEGER;
    m_value.integer = integer;
  }
  int64_t integer () const
  {
    assert( m_kind == TokenKind::INTEGER );
    return m_value.integer;
  };

  void real ( double real )
  {
    m_kind = TokenKind::REAL;
    m_value.real = real;
  }
  double real () const
  {
    assert( m_kind == TokenKind::REAL );
    return m_value.real;
  };

  void vbool ( bool vbool )
  {
    m_kind = TokenKind::BOOL;
    m_value.vbool = vbool;
  }
  bool vbool () const
  {
    assert( m_kind == TokenKind::BOOL );
    return m_value.vbool;
  };
};

}} // namespaces

#endif	/* P1_SMALLS_PARSER_TOKEN_HPP */

