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

#include "Token.hpp"
#include "SymbolTable.hpp"
#include "detail/StringCollector.hpp"
#include "p1/smalls/common/AbstractErrorReporter.hpp"
#include "p1/util/utf-8.hpp"

namespace p1 {
namespace smalls {

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

  TokenKind::Enum nextToken ( Token & tok )
  {
    _nextToken( tok );
    return tok.kind();
  }

  static std::string & escapeStringChar ( std::string & buf, uint32_t ch );
  static std::string escapeStringChar ( uint32_t ch );
  static std::string escapeToString ( const int32_t * codePoints, unsigned count );

private:
  void error ( int ofs, const gc_char * message, ... );

  int32_t validateCodePoint ( int32_t cp );

  void nextChar ();

  void saveCoords ( Token & tok )
  {
    m_tokCoords.line = m_line;
    m_tokCoords.column = m_decoder.offset() - m_lineOffset;
    tok.coords( m_tokCoords );
  }

  void _nextToken ( Token & tok );

  void scanNestedComment ();
  void scanCharacterConstant ( Token & tok );
  void scanString ( Token & tok );
  TokenKind::Enum scanSingleCharacter ( int32_t & value );
  int32_t scanUnicodeEscape ( unsigned maxLen );
  uint8_t scanHexEscape ();
  uint8_t scanOctalEscape ();
  void scanRemainingIdentifier ( Token & tok );
  void identifier ( Token & tok, const gc_char * name );
  void scanNumber ( Token & tok, unsigned state=0 );
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
