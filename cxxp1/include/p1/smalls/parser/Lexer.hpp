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

#ifndef P1_SMALLS_PARSER_LEXER_HPP
#define P1_SMALLS_PARSER_LEXER_HPP

#include "SymbolTable.hpp"
#include "detail/StringCollector.hpp"
#include "p1/smalls/common/AbstractErrorReporter.hpp"
#include "p1/util/utf-8.hpp"

namespace p1 {
namespace smalls {

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

class Lexer : public gc
{
  const gc_char * m_fileName;
  SymbolTable & m_symbolTable;
  AbstractErrorReporter * m_errors;

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

  class StreamErrorReporter : public p1::IStreamDecoderErrorReporter
  {
    Lexer & m_outer;
  public:
    StreamErrorReporter ( Lexer & outer ) : m_outer( outer ) {};
    void error ( off_t offset, off_t outOffset, const gc_char * message );
  };

  StreamErrorReporter m_streamErrors;
  p1::UTF8StreamDecoder m_decoder;

  int32_t m_curChar;

  bool m_inNestedComment;
  detail::StringCollector m_strBuf;

  TokenKind::Enum m_curToken;
  const gc_char * m_valueString;
  Symbol * m_valueSymbol;
  int64_t m_valueInteger;
  double  m_valueReal;
  bool    m_valueBool;

  static const int32_t HT = 9;
  static const int32_t LF = '\n';
  static const int32_t CR = '\r';
  static const int32_t VT = 0x0B;
  static const int32_t FF = 0x0C;
  static const int32_t U_NEXT_LINE = 0x85;
  static const int32_t U_LINE_SEP = 0x2028;
  static const int32_t U_PARA_SEP = 0x2029;

public:
  Lexer ( p1::FastCharInput & in, const gc_char * fileName, SymbolTable & symbolTable, AbstractErrorReporter & errors );

  SymbolTable & symbolTable () { return m_symbolTable; }
  AbstractErrorReporter & errorReporter () { return *m_errors; }

  TokenKind::Enum nextToken ()
  {
    return m_curToken = _nextToken();
  }

  TokenKind::Enum curToken () const { return m_curToken; }
  const SourceCoords & coords () const { return m_tokCoords; }
  const gc_char * valueString () const { return m_valueString; }
  Symbol * valueSymbol () const { return m_valueSymbol; }
  int64_t valueInteger () const { return m_valueInteger; };
  double valueReal () const { return m_valueReal; };
  bool valueBool () const { return m_valueBool; };

  static std::string & escapeStringChar ( std::string & buf, uint32_t ch );
  static std::string escapeStringChar ( uint32_t ch );
  static std::string escapeToString ( const int32_t * codePoints, unsigned count );

private:
  void error ( int ofs, const gc_char * message, ... );

  int32_t validateCodePoint ( int32_t cp );

  void nextChar ();

  void saveCoords ()
  {
    m_tokCoords.line = m_line;
    m_tokCoords.column = m_decoder.offset() - m_lineOffset;
  }

  TokenKind::Enum _nextToken ();

  void scanNestedComment ();
  TokenKind::Enum scanCharacterConstant ();
  TokenKind::Enum scanString ();
  TokenKind::Enum scanSingleCharacter ();
  int32_t scanUnicodeEscape ( unsigned maxLen );
  uint8_t scanHexEscape ();
  uint8_t scanOctalEscape ();
  TokenKind::Enum scanRemainingIdentifier ();
  TokenKind::Enum identifier ( const gc_char * name );
  TokenKind::Enum scanNumber ( unsigned state=0 );
  bool scanUInt ( unsigned base );

  static bool isNewLine ( int32_t ch );
  static bool isWhitespace ( int32_t ch );
  static bool isDelimiter ( int32_t ch );
  static bool isAlpha ( int32_t ch )
  {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
  }
};

}} // namespaces

#endif // P1_SMALLS_PARSER_LEXER_HPP
