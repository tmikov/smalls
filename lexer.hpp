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

#ifndef LEXER_HPP
#define LEXER_HPP

#include <iostream>

#include <boost/unordered_map.hpp>

#include "IErrorReporter.hpp"
#include "utf-8.hpp"
#include "StringCollector.hpp"

#define _DEF_SYMCODES \
  _MK_ENUM(NONE) \
  _MK_ENUM(QUOTE) \
/* Handled by a macro \
  _MK_ENUM(QUASIQUOTE) \
  _MK_ENUM(UNQUOTE) \
  _MK_ENUM(UNQUOTE_SPLICING) \
*/ \
  _MK_ENUM(SYNTAX) \
  _MK_ENUM(QUASISYNTAX) \
  _MK_ENUM(UNSYNTAX) \
  _MK_ENUM(UNSYNTAX_SPLICING) \
  \
  _MK_ENUM(IF) \
  _MK_ENUM(BEGIN) \
  _MK_ENUM(LAMBDA) \
  _MK_ENUM(DEFINE) \
  _MK_ENUM(SETBANG) \
  _MK_ENUM(LET) \
  _MK_ENUM(LETREC) \
  _MK_ENUM(LETREC_STAR) \
  \
  _MK_ENUM(BUILTIN) \
  _MK_ENUM(DEFINE_MACRO) \
  _MK_ENUM(DEFINE_IDENTIFIER_MACRO) \
  _MK_ENUM(DEFINE_SET_MACRO) \
  _MK_ENUM(MACRO_ENV)

struct SymCode
{
  #define _MK_ENUM(x)  x,
  enum Enum
  {
    _DEF_SYMCODES
  };
  #undef _MK_ENUM

  static const char * name ( Enum x )  { return s_names[x]; }
private:
  static const char * s_names[];
};

class Symbol : public gc
{
public:
  const gc_char * const name;
  SymCode::Enum const code;
  
  Symbol ( const gc_char * name_, SymCode::Enum code_ ) 
    : name( name_ ), code( code_ )
  {}
};

struct gc_charstr_equal : public std::binary_function<const gc_char *,const gc_char *,bool>
{
  bool operator () ( const gc_char * a, const gc_char * b ) const
  {
    return std::strcmp( a, b ) == 0;
  }
};

struct gc_charstr_hash : std::unary_function<const gc_char *, std::size_t>
{
  std::size_t operator () ( const gc_char * a ) const
  {
    std::size_t seed = 0;
    unsigned t;
    while ( (t = *((unsigned char *)a)) != 0)
    {
      boost::hash_combine( seed, t );
      ++a;
    }
    return seed;
  }
};

// We need to separate this in a base class so we can access it during
// const member initialization in the constructor
class SymbolMapBase : public gc
{
protected:  
  typedef boost::unordered_map<const gc_char *,
                               Symbol *,
                               gc_charstr_hash, 
                               gc_charstr_equal,
                               gc_allocator<const gc_char *> > Map;
  Map m_map;
};

class SymbolMap : public SymbolMapBase
{
  Symbol * special ( const gc_char * name, SymCode::Enum code );
public:
  SymbolMap ();
  
  Symbol * newSymbol ( const gc_char * name );
  
  Symbol * const sym_quote;
  Symbol * const sym_quasiquore;
  Symbol * const sym_unquote;
  Symbol * const sym_unquote_splicing;
  Symbol * const sym_syntax;
  Symbol * const sym_quasisyntax;
  Symbol * const sym_unsyntax;
  Symbol * const sym_unsyntax_splicing;

  Symbol * const sym_if;
  Symbol * const sym_begin;
  Symbol * const sym_lambda;
  Symbol * const sym_define;
  Symbol * const sym_setbang;
  Symbol * const sym_let;
  Symbol * const sym_letrec;
  Symbol * const sym_letrec_star;

  Symbol * const sym_builtin;
  Symbol * const sym_define_macro;
  Symbol * const sym_define_identifier_macro;
  Symbol * const sym_define_det_macro;
  Symbol * const sym_macro_env;
};


#define _DEF_TOKENS \
   _MK_ENUM(NONE,"<NONE>") \
   _MK_ENUM(EOFTOK,"<EOF>") \
   _MK_ENUM(IDENT,"identifier") \
   _MK_ENUM(BOOL,"#t or #f") \
   _MK_ENUM(NUMBER,"number") \
   _MK_ENUM(CHAR,"character") \
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

struct Token
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

class Lexer : public gc
{
  const gc_char * m_fileName;
  SymbolMap & m_symbolMap;
  IErrorReporter & m_errors;
  
  /**
   * The stream offset of the beginning of the line. We use that to calculate the columns
   * position of all characters in a line instead of incrementing a column.
   */
  off_t m_lineOffset;
  unsigned m_line;
  /**
   * The coordinates of the token we just returned.
   */
  SourceCoords m_tokCoords;
  
  class StreamErrorReporter : public IStreamDecoderErrorReporter
  {
    Lexer & m_outer;
  public:
    StreamErrorReporter ( Lexer & outer ) : m_outer( outer ) {};
    void error ( off_t offset, off_t outOffset, const gc_char * message );
  };
  
  StreamErrorReporter m_streamErrors;
  UTF8StreamDecoder m_decoder;
  
  int32_t m_curChar;
  
  bool m_inNestedComment;
  StringCollector m_strBuf;
  
  Token::Enum m_curToken;
  const gc_char * m_valueString; 
  
  static const int32_t HT = 9;
  static const int32_t LF = '\n';
  static const int32_t CR = '\r';
  static const int32_t VT = 0x0B;
  static const int32_t FF = 0x0C;
  static const int32_t U_NEXT_LINE = 0x85;
  static const int32_t U_LINE_SEP = 0x2028;
  static const int32_t U_PARA_SEP = 0x2029;
  
public:
  Lexer ( BufferedCharInput & in, const gc_char * fileName, SymbolMap & symbolMap, IErrorReporter & errors );
  
  Token::Enum nextToken ()
  {
    return m_curToken = _nextToken();
  }
  
private:
  void error ( const gc_char * message, ... );
  
  int32_t validateCodePoint ( int32_t cp );
  
  /**
  * Unget the character in {@link #m_curChar} and replace it with another one. The next
  * {@link #nextChar()} will return the value that used to be in {@link #m_curChar}. It function
  * <b>MUST</b> not be used to unget a line feed!
  *
  * <p>Source coordinates of the "ungotten" character are determined by assuming that it is the
  * previous character, before {@link #m_curChar}, on the current line. They are the current column - 1.
  * (That is why line feed must not be ungotten).
  *
  * <p>In general this function is needed for convenience, to enable an extra character lookahead in
  * some rare cases. We need just one char. In theory it could be avoided with the cost of
  * significantly more code, expanding the DFA. The pattern of usage is:
  * <pre>
  *   if (m_curChar = '1')
  *   {
  *     nextChar();
  *     if (m_curChar == '2'))
  *     {
  *       ungetChar( '1' );
  *       scan(); // scan() will first see '1' (in m_curChar) and only afterwards will obtain '2'
  *     }
  *   }
  * </pre>
  *
  *
  * @param ch the char to unget.
  */
  void ungetChar ( int32_t ch )
  {
    m_decoder.unget( m_curChar );
    m_curChar = ch;
  }

  int nextChar ();
  
  void saveCoords ()
  {
    m_tokCoords.line = m_line;
    m_tokCoords.column = m_decoder.offset() - m_lineOffset + 1;
  }
  
  Token::Enum _nextToken ();
  
  Token::Enum scanString ();
  int32_t scanInlineHexEscape ();

  static bool isNewLine ( int32_t ch );
  static bool isWhitespace ( int32_t ch );
  static bool isDelimiter ( int32_t ch );
};

#endif
