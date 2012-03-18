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

#include "lexer.hpp"
#include <cctype>

#define _MK_ENUM(x) #x,
const char * SymCode::s_names[] =
{
  _DEF_SYMCODES
};
#undef _MK_ENUM


Symbol * SymbolMap::special ( const gc_char * name, SymCode::Enum code )
{
  Symbol * sym = new Symbol( name, code );
  if (!m_map.insert( Map::value_type(name, sym) ).second)
    throw std::logic_error( std::string(name) + " already defined" );
  return sym;
}

Symbol * SymbolMap::newSymbol ( const gc_char * name )
{
  Map::iterator it;
  if ( (it = m_map.find( name )) != m_map.end())
    return it->second;
  Symbol * sym = new Symbol( name, SymCode::NONE );
  m_map[name] = sym;
  return sym;
}

SymbolMap::SymbolMap() :
  sym_quote             ( special( "quote", SymCode::QUOTE ) ),
  sym_quasiquore        ( special( "quasiquote", SymCode::NONE /*SymCode::QUASIQUOTE*/ ) ),
  sym_unquote           ( special( "unquote", SymCode::NONE /*SymCode::UNQUOTE*/ ) ),
  sym_unquote_splicing  ( special( "unquote-splicing", SymCode::NONE /*SymCode::UNQUOTE_SPLICING*/ ) ),
  sym_syntax            ( special( "syntax", SymCode::SYNTAX ) ),
  sym_quasisyntax       ( special( "quasisyntax", SymCode::QUASISYNTAX ) ),
  sym_unsyntax          ( special( "unsyntax", SymCode::UNSYNTAX ) ),
  sym_unsyntax_splicing ( special( "unsyntax-splicing", SymCode::UNSYNTAX_SPLICING ) ),

  sym_if                ( special( "if", SymCode::IF ) ),
  sym_begin             ( special( "begin", SymCode::BEGIN ) ),
  sym_lambda            ( special( "lambda", SymCode::LAMBDA ) ),
  sym_define            ( special( "define", SymCode::DEFINE ) ),
  sym_setbang           ( special( "set!", SymCode::SETBANG ) ),
  sym_let               ( special( "let", SymCode::LET ) ),
  sym_letrec            ( special( "letrec", SymCode::LETREC ) ),
  sym_letrec_star       ( special( "letrec*", SymCode::LETREC_STAR ) ),

  sym_builtin           ( special( "__%builtin", SymCode::BUILTIN ) ),
  sym_define_macro      ( special( "define-macro", SymCode::DEFINE_MACRO ) ),
  sym_define_identifier_macro ( special( "define-identifier-macro", SymCode::DEFINE_IDENTIFIER_MACRO ) ),
  sym_define_det_macro  ( special( "define-set-macro", SymCode::DEFINE_SET_MACRO ) ),
  sym_macro_env         ( special( "macro-env", SymCode::MACRO_ENV ) )
{
}

#define _MK_ENUM(name,repr)  #name,
const char * Token::s_names[] =
{
  _DEF_TOKENS
};
#undef _MK_ENUM

#define _MK_ENUM(name,repr)  repr,
const char * Token::s_reprs[] =
{
  _DEF_TOKENS
};
#undef _MK_ENUM


Lexer::Lexer ( FastCharInput & in, const gc_char * fileName, SymbolMap & symbolMap, AbstractErrorReporter & errors )
  : m_fileName( fileName ), m_symbolMap( symbolMap ), m_errors( errors ),
    m_tokCoords( fileName, 0, 0 ), m_streamErrors( *this ), m_decoder( in, m_streamErrors )
{
  m_curChar = 0;
  m_curToken = Token::NONE;
  m_inNestedComment = false;
  m_valueString = NULL;

  m_lineOffset = 0;
  m_line = 1;
  nextChar();
}

void Lexer::StreamErrorReporter::error ( off_t offset, off_t outOffset, const gc_char * message )
{
  m_outer.error( "%s at offset %lu", message, (unsigned long)offset );
}

void Lexer::error ( const gc_char * message, ... )
{
  std::va_list ap;
  va_start( ap, message );
  const gc_char * str = vformatGCStr( message, ap );
  va_end( ap );

  SourceCoords coords( m_fileName, m_line, m_decoder.offset() - m_lineOffset + 1 );
  m_errors.error( coords, str );
}

int32_t Lexer::validateCodePoint ( int32_t ch )
{
  if (!isValidCodePoint(ch))
  {
    error( "Invalid Unicode character 0x%04x", ch );
    ch = ' '; // Simple error recovery
  }
  return ch;
}

/**
 * Read and return the next character. If we are at EOF or on any I/O error returns and keep
 * returning -1. {@link #m_line} is updated if we encounter a new line.
 *
 * <p>All line end characters and combination are translated to '\n' ({@link #LF}).
 *
 * @return the next character, or -1 on EOF or any I/O error
 */
void Lexer::nextChar ()
{
  int32_t ch = m_decoder.get();

  // Translate CR, CR LF, U_NEXT_LINE, U_LINE_SEP, U_PARA_SEP into LF and update the line number
  switch (ch)
  {
  case CR:
    if (m_decoder.peek() == LF) // Collapse CR LF into LF
      m_decoder.advance(1);
    // FALL
  case U_NEXT_LINE:
  case U_LINE_SEP:
  case U_PARA_SEP:
    ch = LF;
    // FALL
  case LF:
    ++m_line;
    m_lineOffset = m_decoder.offset();
    break;
  }

  m_curChar = ch;
}

#define NEWLINE_CASE    case LF
#define WHITESPACE_CASE NEWLINE_CASE: case ' ': case '\f': case '\t': case '\v'


inline bool Lexer::isNewLine ( int32_t ch )
{
  switch (ch)
  {
  NEWLINE_CASE: return true;
  default: return false;
  }
}

inline bool Lexer::isWhitespace ( int32_t ch )
{
  switch (ch)
  {
  WHITESPACE_CASE: return true;
  default: return false;
  };
}

bool Lexer::isDelimiter ( int32_t ch )
{
  switch (ch)
  {
  WHITESPACE_CASE:
  case '(': case ')': case '[': case ']': case '"': case ';': case '#':
  case -1: // EOF is also a delimiter
    return true;
  default:
    return false;
  }
}

static bool  isBaseDigit ( unsigned base, int32_t ch )
{
  switch (base)
  {
  case 2: return ch == '0' || ch == '1';
  case 8: return ch >= '0' && ch <= '7';
  case 10: return ch >= '0' && ch <= '9';
  case 16: ch |= 32; return ch >= '0' && ch <= '9' || ch >= 'a' && ch <= 'f';
  }
  assert( false );
  return false;
}

/**
 * Convert a digit in bases 2,8,10,16 to int. Note that we don't have to specify the base.
 * @param ch
 * @return
 */
static int baseDigitToInt ( int32_t ch )
{
  ch |= 32;
  return ch <= '9' ? ch - '0'  : ch - ('a' - 10);
}

Token::Enum Lexer::_nextToken ()
{
  for(;;)
  {
    Token::Enum res;
    saveCoords();
    switch (m_curChar)
    {
    case -1:
      return Token::EOFTOK;

    case '(': nextChar(); return Token::LPAR;
    case ')': nextChar(); return Token::RPAR;
    case '[': nextChar(); return Token::LSQUARE;
    case ']': nextChar(); return Token::RSQUARE;
    case '\'': nextChar(); return Token::APOSTR;
    case '`': nextChar(); return Token::ACCENT;

    // nested commend end handling.
    case '|':
      nextChar(); // consume the '|'
      if (!m_inNestedComment || m_curChar != '#')
        error( "\"|\" cannot start a lexeme" );
      else
      {
        nextChar(); // consume the '#'
        return Token::NESTED_COMMENT_END;
      }
      break;

    case ',':
      nextChar();
      if (m_curChar == '@')
      {
        nextChar();
        return Token::COMMA_AT;
      }
      else
        return Token::COMMA;

    // <string>
    case '"':
      nextChar();
      if ( (res = scanString()) != Token::NONE)
        return res;
      break;

    // <comment>
    case ';':
      // Skip until we reach line end or EOF
      do
        nextChar();
      while (m_curChar != -1 && m_curChar != LF);
      break;

//    // <digit>
//    case '0': case '1': case '2': case '3': case '4':
//    case '5': case '6': case '7': case '8': case '9':
//      if ( (res = scanNumber()) != null)
//        return res;
//      break;
//
//    case '#':
//      nextChar();
//      switch (m_curChar)
//      {
///*
//      case '%': // Extension: system identifiers
//        nextChar();
//        if ( (res = scanRestIdentifier( '#', '%' )) != null)
//          return res;
//        break;
//*/
//      case 'i': case 'I': case 'e': case 'E': // number exactness prefix
//      case 'b': case 'B': // binary number
//      case 'o': case 'O': // octal
//      case 'd': case 'D': // decimal
//      case 'x': case 'X': // hex
//        {
//          ungetChar( '#' );
//          if ( (res = scanNumber()) != null)
//            return res;
//        }
//        break;
//
//      case '|': // #| Nested comment
//        nextChar();
//        if (!m_inNestedComment)
//          scanNestedComment();
//        else
//          return Token::NESTED_COMMENT_START;
//        break;
//
///* TODO:
//    case '!': // #!r6rs
//      ...
//*/
//
//      case ';': // #; datum comment
//        nextChar();
//        return Token::DATUM_COMMENT;
//
//
//      case '(':  /* #( */ nextChar(); return Token::HASH_LPAR;
//      case '\'': /* #' */ nextChar(); return Token::HASH_APOSTR;
//      case '`':  /* #` */ nextChar(); return Token::HASH_ACCENT;
//      case ',':  /* #, #,@ */
//        nextChar();
//        if (m_curChar == '@') // #,@
//        {
//          nextChar();
//          return Token::HASH_COMMA_AT;
//        }
//        else
//          return Token::HASH_COMMA;
//
//      case 't': case 'T': // #t #T
//        nextChar();
//        if (!isDelimiter(m_curChar))
//          error( null, "Bad #x form" );
//        m_valueBool = true;
//        return Token::BOOL;
//      case 'f': case 'F': // #f #F
//        nextChar();
//        if (!isDelimiter(m_curChar))
//          error( null, "Bad #x form" );
//        m_valueBool = false;
//        return Token::BOOL;
//
//      case '\\': // #\ character
//        nextChar();
//        if ( (res = scanCharacter()) != null)
//          return res;
//        break;
//
//      default:
//        error( null, "Illegal lexeme \"%s\"", escapeToString(new int[]{ '#', m_curChar }, 0, 2 ));
//        nextChar();
//        break;
//      }
//      break;
//
//    // <identifier>
//    case '!': case '$': case '%': case '&': case '*': case '/': case ':': case '<':
//    case '=': case '>': case '?': case '^': case '_': case '~':
//      {
//        int saveCh = m_curChar;
//        nextChar();
//        if ( (res = scanRestIdentifier(saveCh)) != null)
//          return res;
//      }
//      break;
//
//    // <peculiar identifier> "+"
//    case '+':
//      nextChar();
//      if (isDelimiter(m_curChar))
//      {
//        if ( (res = identifier( "+" )) != null)
//          return res;
//      }
//      else
//      {
//        ungetChar('+');
//        if ( (res = scanNumber()) != null)
//          return res;
//      }
//      break;
//
//    // <peculiar identifier> "-" "->"
//    case '-':
//      nextChar();
//      if (isDelimiter(m_curChar)) // just '-' ?
//      {
//        if ( (res = identifier("-")) != null)
//          return res;
//      }
//      else if (m_curChar == '>') // "->" ?
//      {
//        nextChar();
//        if ( (res = scanRestIdentifier('-', '>')) != null)
//          return res;
//      }
//      else
//      {
//        ungetChar('-');
//        if ( (res = scanNumber()) != null)
//          return res;
//      }
//      break;
//
//    // <peculiar identifier> "..."
//    case '.':
//      nextChar();
//      if (isDelimiter(m_curChar))
//        return Token::DOT;
//      else if (m_curChar >= '0' && m_curChar <= '9')
//      {
//        ungetChar( '.' );
//        if ( (res = scanNumber()) != null)
//          return res;
//      }
//      else
//      {
//        if ( (res = scanRestIdentifier('.')) != null)
//          return res;
//      }
//      break;
//
//    // <inline hex escape>
//    case '\\':
//      nextChar();
//      if (m_curChar == 'x') // \x -> identifier starting with an inline hex escape
//      {
//        nextChar();
//        if ( (res = scanRestIdentifier( scanInlineHexEscape() )) != null)
//          return res;
//      }
//      else
//        error( null, "\"\\\" cannot start a lexeme" );
//      break;
//
//    default:
//      if (Character.isWhitespace(m_curChar))
//      {
//        // Consume all whitespace here for efficiency
//        do
//          nextChar();
//        while (Character.isWhitespace(m_curChar));
//      }
//      else if (Character.isLetter(m_curChar))
//      {
//        int saveCh = m_curChar;
//        nextChar();
//        if ( (res = scanRestIdentifier(saveCh)) != null)
//          return res;
//      }
//      else
//      {
//        error( null, "\"%s\" cannot start a lexeme", escapeToString( new int[]{ m_curChar }, 0, 1 ) );
//        nextChar();
//      }
//      break;
    }
  }
}

Token::Enum Lexer::scanString ()
{
  m_strBuf.reset();

  for(;;)
  {
    if (m_curChar == '"')
    {
      nextChar();
      break;
    }
    else if (m_curChar < 0)
    {
      error( "Unterminated string lexeme at end of input" );
      break;
    }
    else if (m_curChar == '\\')
    {
      nextChar();
      switch (m_curChar)
      {
      case -1: error( "Unterminated string escape at end of input" ); goto exitLoop;

      case 'a': m_strBuf.append( '\a' ); nextChar(); break;
      case 'b': m_strBuf.append( '\b' ); nextChar(); break;
      case 't': m_strBuf.append( '\t' ); nextChar(); break;
      case 'n': m_strBuf.append( '\n' ); nextChar(); break;
      case 'v': m_strBuf.append( '\v' ); nextChar(); break;
      case 'f': m_strBuf.append( '\f' ); nextChar(); break;
      case 'r': m_strBuf.append( '\r' ); nextChar(); break;
      case '"': m_strBuf.append( '"' ); nextChar(); break;
      case '\\': m_strBuf.append( '\\' ); nextChar(); break;

      case 'x':
        nextChar();
        m_strBuf.append( scanHexEscape() );
        break;
      case 'u': case 'U':
        nextChar();
        m_strBuf.appendCodePoint( scanUnicodeEscape() );
        break;

      default:
        // '\\' <intraline whitespace> '\n'
        while (m_curChar != '\n' && isWhitespace(m_curChar))
          nextChar();

        if (m_curChar == '\n')
          nextChar();
        else if (m_curChar == -1)
        {
          error( "Unterminated string escape at end of input" ); goto exitLoop;
        }
        else // Invalid escape!
        {
          // Slightly tricky. We can't display Unicode characters directly. We need to convert them
          // to utf-8 strings
          char tmp[8];
          tmp[encodeUTF8( tmp, m_curChar )] = 0;
          error( "Invalid string escape \\%s", tmp );
          nextChar();
        }
        break;
      }
    }
    else
    {
      m_strBuf.appendCodePoint( m_curChar );
      nextChar();
    }
  }
exitLoop:

  m_valueString = m_strBuf.createGCString();
  return Token::STR;
}

/**
 * Called after the \u has been scanned, to scan the rest of the Unicode escape character in hex.
 * It must be a valid Unicode character. Returns its validated value.
 *
 * <p>The Unicode character includes up to 8 hex characters after the escape.
 *
 * @return the validated value of the inline hex character.
 */
int32_t Lexer::scanUnicodeEscape ()
{
  uint32_t resultCodePoint = 0;
  bool err = false;

  if (!isBaseDigit(16, m_curChar))
  {
    error( "Invalid Unicode escape" );
    return ' ';
  }

  // Sequence of hex digits
  unsigned i = 0;
  do
  {
    if (!err)
    {
      uint32_t newResult = (resultCodePoint << 4) + baseDigitToInt(m_curChar);
      if (newResult >= resultCodePoint)
        resultCodePoint = newResult;
      else
      {
        error( "Invalid Unicode escape" );
        err = true;
        resultCodePoint = ' ';
      }
    }
    ++i;
    nextChar();
  }
  while(i != 8 && isBaseDigit(16, m_curChar));

  return validateCodePoint( resultCodePoint );
}

/**
 * Scan exactly two hex characters (after \x) and return a byte.
 */
uint8_t Lexer::scanHexEscape()
{
  uint8_t res;
  if (!isBaseDigit(16, m_curChar))
  {
    error( "Invalid hex escape" );
    return ' ';
  }
  res = baseDigitToInt(m_curChar) << 4;
  nextChar();
  if (!isBaseDigit(16, m_curChar))
  {
    error( "Invalid hex escape" );
    return ' ';
  }
  res |= baseDigitToInt(m_curChar);
  nextChar();
  return res;
}
