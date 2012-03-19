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
  : m_fileName( fileName ), m_symbolMap( symbolMap ), m_errors( &errors ),
    m_tokCoords( fileName, 0, 0 ), m_streamErrors( *this ), m_decoder( in, m_streamErrors )
{
  m_curChar = 0;
  m_curToken = Token::NONE;
  m_inNestedComment = false;
  m_valueString = NULL;
  m_valueIdent = NULL;

  m_lineOffset = 0;
  m_line = 1;
  nextChar();
}

void Lexer::StreamErrorReporter::error ( off_t offset, off_t outOffset, const gc_char * message )
{
  m_outer.error( 0, "%s at offset %lu", message, (unsigned long)offset );
}

void Lexer::error ( int ofs, const gc_char * message, ... )
{
  std::va_list ap;
  va_start( ap, message );
  const gc_char * str = vformatGCStr( message, ap );
  va_end( ap );

  SourceCoords coords( m_fileName, m_line, m_decoder.offset() - m_lineOffset + ofs );
  m_errors->error( coords, str );
}

int32_t Lexer::validateCodePoint ( int32_t ch )
{
  if (!isValidCodePoint(ch))
  {
    error( 0, "Invalid Unicode character 0x%04x", ch );
    ch = ' '; // Simple error recovery
  }
  return ch;
}

std::string & Lexer::escapeStringChar ( std::string & buf, uint32_t ch )
{
  if (ch >= 32 && ch <= 127)
  {
    buf.push_back( (char)ch );
    return buf;
  }
  switch (ch)
  {
  case '\a': return buf.append( "\\a" );
  case '\b': return buf.append( "\\b" );
  case '\t': return buf.append( "\\t" );
  case '\n': return buf.append( "\\n" );
  case '\v': return buf.append( "\\v" );
  case '\f': return buf.append( "\\f" );
  case '\r': return buf.append( "\\r" );
  case '\"': return buf.append( "\\\"" );
  case '\\': return buf.append( "\\\\" );
  default:
    if (isValidCodePoint(ch))
    {
      // TODO: we shouldn't escape printable Unicode characters
      return buf.append( formatStr(ch<=0xFFFF?"\\u%04x":"\\U%08x", ch) );
    }
    else
    {
      do
      {
        buf.append( "\\x" );
        buf.push_back( (ch >> 4) & 0xF );
        buf.push_back( ch & 0xF );
      }
      while ((ch>>=8) != 0);
      return buf;
    }
  }
}

std::string Lexer::escapeStringChar ( uint32_t ch )
{
  std::string res;
  return escapeStringChar( res, ch );
}

/**
 * Generate a properly escaped string representation of a code point sequence.
 */
std::string Lexer::escapeToString ( const int32_t * codePoints, unsigned count )
{
  std::string res;
  res.reserve( count );
  for ( ; count != 0; ++codePoints, --count )
    escapeStringChar( res, *codePoints );
  return res;
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
    case '*':
      nextChar();
      if (m_curChar == '/')
      {
        nextChar();
        if (m_inNestedComment)
          return Token::NESTED_COMMENT_END;
        else
          m_errors->error( m_tokCoords, "Unexpected */" );
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append( '*' );
        if ( (res = scanRemainingIdentifier()) != Token::NONE)
          return res;
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
    line_comment:
      // Skip until we reach line end or EOF
      do
        nextChar();
      while (m_curChar != -1 && m_curChar != LF);
      break;

    // C++ comments
    case '/':
      {
        nextChar();
        if (m_curChar == '/')
          goto line_comment;
        else if (m_curChar == '*')
        {
          nextChar();
          if (!m_inNestedComment)
            scanNestedComment();
          else
            return Token::NESTED_COMMENT_START;
        }
        else
        {
          m_strBuf.reset();
          m_strBuf.append( '/' );
          if ( (res = scanRemainingIdentifier()) != Token::NONE)
            return res;
        }
      }
      break;


    // <digit>
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if ( (res = scanNumber(3)) != Token::NONE)
        return res;
      break;

    case '#':
      nextChar();
      switch (m_curChar)
      {
      case ';': // #; datum comment
        nextChar();
        return Token::DATUM_COMMENT;

      case '(':  /* #( */ nextChar(); return Token::HASH_LPAR;
      case '\'': /* #' */ nextChar(); return Token::HASH_APOSTR;
      case '`':  /* #` */ nextChar(); return Token::HASH_ACCENT;
      case ',':  /* #, #,@ */
        nextChar();
        if (m_curChar == '@') // #,@
        {
          nextChar();
          return Token::HASH_COMMA_AT;
        }
        else
          return Token::HASH_COMMA;

      case 't': case 'T': // #t #T
        nextChar();
        if (!isDelimiter(m_curChar))
          error( 0, "Bad #x form" );
        m_valueBool = true;
        return Token::BOOL;
      case 'f': case 'F': // #f #F
        nextChar();
        if (!isDelimiter(m_curChar))
          error( 0, "Bad #x form" );
        m_valueBool = false;
        return Token::BOOL;

      case '"': //  character constant
        nextChar();
        if ( (res = scanCharacterConstant()) != Token::NONE)
          return res;
        break;

      default:
        {
          int32_t tmp[] = { '#', m_curChar };
          m_errors->error( m_tokCoords, formatGCStr("Illegal lexeme \"%s\"", escapeToString(tmp,2 ).c_str()) );
          nextChar();
        }
        break;
      }
      break;

    // <identifier>
    case '!': case '$': case '%': case '&':/*case '*': case '/':*/case ':': case '<':
    case '=': case '>': case '?': case '^': case '_': case '~':
    case '|':
      {
        m_strBuf.reset();
        m_strBuf.append( (char)m_curChar );
        nextChar();
        if ( (res = scanRemainingIdentifier()) != Token::NONE)
          return res;
      }
      break;

    // <peculiar identifier> "+"
    case '+':
      nextChar();
      if (m_curChar >= '0' && m_curChar <= '9' || m_curChar == '.')
      {
        if ( (res = scanNumber(1)) != Token::NONE)
          return res;
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append( '+' );
        if ( (res = scanRemainingIdentifier()) != Token::NONE)
          return res;
      }
      break;

    // <peculiar identifier> "-" "->"
    case '-':
      nextChar();
      if (m_curChar >= '0' && m_curChar <= '9' || m_curChar == '.')
      {
        if ( (res = scanNumber(2)) != Token::NONE)
          return res;
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append( '-' );
        if ( (res = scanRemainingIdentifier()) != Token::NONE)
          return res;
      }
      break;

    // <peculiar identifier> "..."
    case '.':
      nextChar();
      if (m_curChar >= '0' && m_curChar <= '9')
      {
        if ( (res = scanNumber(4)) != Token::NONE)
          return res;
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append('.');
        if ( (res = scanRemainingIdentifier()) != Token::NONE)
          return res;
      }
      break;

    // <inline hex escape>
    case '\\':
      if ( (res = scanRemainingIdentifier()) != Token::NONE)
        return res;
      break;

    default:
      if (isWhitespace(m_curChar))
      {
        // Consume all whitespace here for efficiency
        do
          nextChar();
        while (isWhitespace(m_curChar));
      }
      else if (isAlpha(m_curChar))
      {
        m_strBuf.reset();
        m_strBuf.appendCodePoint( m_curChar );
        nextChar();
        if ( (res = scanRemainingIdentifier()) != Token::NONE)
          return res;
      }
      else
      {
        error( 0, "\"%s\" cannot start a lexeme", escapeStringChar( m_curChar ).c_str() );
        nextChar();
      }
      break;
    }
  }
}

namespace {

class NullReporter : public AbstractErrorReporter
{
  virtual void error ( const SourceCoords & coords, const gc_char * message )
  {}
};

NullReporter s_nullReporter;

}

/**
 * Scan a nested multi-line comment. The comment terminator is treated as a token within a stream of
 * tokens. Thus we won't be looking within strings.
 *
 * <p>It works by resetting the error reporter to one that ignores all errors and calling the
 * scanner recursively. It however does not rely on recursion to handle the nested comments themselves
 */
void Lexer::scanNestedComment ()
{
  SourceCoords nestedCommentStart( m_tokCoords );

  assert( !m_inNestedComment );

  AbstractErrorReporter * saveReporter = m_errors;
  m_errors = &s_nullReporter;
  m_inNestedComment = true;
  try
  {
    int level = 1;
    for(;;)
    {
      switch (nextToken())
      {
      case Token::NESTED_COMMENT_START:
        ++level;
        break;
      case Token::NESTED_COMMENT_END:
        if (--level == 0)
          goto exitLoop;
        break;
      case Token::EOFTOK:
        goto exitLoop; // we must report the error after we have restored the error reporter
      default:
        break;
      }
    }
exitLoop:;
  }
  catch(...)
  {
    m_errors = saveReporter;
    m_inNestedComment = false;
    throw;
  }
  m_errors = saveReporter;
  m_inNestedComment = false;

  if (m_curToken == Token::EOFTOK)
    error( 0, "EOF in comment started on line "+ nestedCommentStart.line );
}

Token::Enum Lexer::scanCharacterConstant ()
{
  if (m_curChar == '"')
  {
    error( 0, "Invalid empty character constant" );
    nextChar();
    m_valueInteger = ' ';
    return Token::INTEGER;
  }
  switch (scanSingleCharacter())
  {
  case Token::INTEGER: // 8-bit character
  case Token::STR:     // Unicode codepoint
    break;
  case Token::NONE:    // error
    m_valueInteger = ' ';
    break;
  case Token::EOFTOK:  // eof/error
    m_valueInteger = ' ';
    return Token::INTEGER;
  default:
    assert( false );
  }

  if (m_curChar != '"')
    error( 0, "Character constant not closed" );
  else
  {
    nextChar();
    if (!isDelimiter(m_curChar))
      error( 0, "Character constant not followed by a delimiter" );
  }

  return Token::INTEGER;
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
    switch (scanSingleCharacter())
    {
    case Token::INTEGER:
      m_strBuf.append( (char)m_valueInteger );
      break;
    case Token::STR:
      m_strBuf.appendCodePoint( (int32_t)m_valueInteger );
      break;
    case Token::EOFTOK:
      goto exitLoop;
    case Token::NONE:
      break;
    default:
      assert( false );
    }
  }
exitLoop:

  if (!isDelimiter(m_curChar))
    error( 0, "String not followed by a delimiter" );

  m_strBuf.append( 0 );
  m_valueString = m_strBuf.createGCString();
  return Token::STR;
}

/**
 * Process one character and store it in m_valueInteger. The return value:
 * <ul>
 * <li>Token::EOFTOK - stop processing the string constant (we reached EOF, LF)</li>
 * <li>Token::NONE - error handling this character
 * <li>Token::INTEGER - a 8-bit integer (possibly invalid Unicode character)
 * <li>Token::STRING - a 32-bit validated Unicode codepoint
 * </ul>
 */
Token::Enum Lexer::scanSingleCharacter ()
{
loop: // We loop only if we encounter \ LF sequence.
  if (m_curChar < 0)
  {
    error( 0, "Unterminated string constant at end of input. String started on line %u column %u",
            m_tokCoords.line, m_tokCoords.column );
    return Token::EOFTOK;
  }
  else if (m_curChar == '\n')
  {
    m_errors->error( m_tokCoords, "Unterminated string constant" );
    return Token::EOFTOK;
  }
  else if (m_curChar == '\\')
  {
    nextChar();
    switch (m_curChar)
    {
    case -1:
      error( 0, "Unterminated string escape at end of input. String started on line %u column %u",
              m_tokCoords.line, m_tokCoords.column );
      return Token::EOFTOK;

    case 'a': m_valueInteger = '\a'; nextChar(); return Token::INTEGER;
    case 'b': m_valueInteger = '\b'; nextChar(); return Token::INTEGER;
    case 't': m_valueInteger = '\t'; nextChar(); return Token::INTEGER;
    case 'n': m_valueInteger = '\n'; nextChar(); return Token::INTEGER;
    case 'v': m_valueInteger = '\v'; nextChar(); return Token::INTEGER;
    case 'f': m_valueInteger = '\f'; nextChar(); return Token::INTEGER;
    case 'r': m_valueInteger = '\r'; nextChar(); return Token::INTEGER;
    case '"': m_valueInteger = '"'; nextChar();  return Token::INTEGER;
    case '\\': m_valueInteger = '\\'; nextChar(); return Token::INTEGER;

    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
      m_valueInteger = scanOctalEscape();
      return Token::INTEGER;
    case 'x':
      nextChar();
      m_valueInteger = scanHexEscape();
      return Token::INTEGER;
    case 'u':
      nextChar();
      m_valueInteger = scanUnicodeEscape(4);
      return Token::STR;
    case 'U':
      nextChar();
      m_valueInteger = scanUnicodeEscape(8);
      return Token::STR;

    default:
      // '\\' <intraline whitespace> '\n'
      while (m_curChar != '\n' && isWhitespace(m_curChar))
        nextChar();

      if (m_curChar == '\n')
      {
        nextChar();
        goto loop;
      }
      else if (m_curChar == -1)
      {
        error( 0, "Unterminated string escape at end of input. String started on line %u column %u",
                m_tokCoords.line, m_tokCoords.column );
        return Token::EOFTOK;
      }
      else // Invalid escape!
      {
        // Slightly tricky. We can't display Unicode characters directly. We need to convert them
        // to utf-8 strings
        char tmp[8];
        tmp[encodeUTF8( tmp, m_curChar )] = 0;
        error( 0, "Invalid string escape \\%s", tmp );
        nextChar();
        return Token::NONE;
      }
      break;
    }
  }
  else
  {
    m_valueInteger =  m_curChar;
    nextChar();
    return Token::STR;
  }
}

/**
 * Called after the \u or \U has been scanned, to scan the rest of the Unicode escape character in hex.
 * It must be a valid Unicode character. Returns its validated value.
 *
 * @return the validated value of the inline hex character.
 */
int32_t Lexer::scanUnicodeEscape ( unsigned len )
{
  uint32_t resultCodePoint = 0;
  bool err = false;

  // Sequence of hex digits
  for ( unsigned i = 0; i != len; ++i )
  {
    if (!isBaseDigit(16, m_curChar))
    {
      error( 0, "Invalid Unicode escape" );
      return ' ';
    }
    if (!err)
    {
      uint32_t newResult = (resultCodePoint << 4) + baseDigitToInt(m_curChar);
      if (newResult >= resultCodePoint)
        resultCodePoint = newResult;
      else
      {
        error( 0, "Invalid Unicode escape" );
        err = true;
        resultCodePoint = ' ';
      }
    }

    nextChar();
  }

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
    error( 0, "Invalid hex escape" );
    return ' ';
  }
  res = baseDigitToInt(m_curChar) << 4;
  nextChar();
  if (!isBaseDigit(16, m_curChar))
  {
    error( 0, "Invalid hex escape" );
    return ' ';
  }
  res |= baseDigitToInt(m_curChar);
  nextChar();
  return res;
}

uint8_t Lexer::scanOctalEscape ()
{
  uint8_t res = 0;
  unsigned i = 0;
  do
  {
    res = (res << 3) + baseDigitToInt(m_curChar);
    nextChar();
  }
  while (++i != 3 && m_curChar >= '0' && m_curChar <= '7');
  return res;
}

Token::Enum Lexer::scanRemainingIdentifier ()
{
  for(;;)
  {
    switch (m_curChar)
    {
    // <digit>
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    // <special subsequent>
    case '+': case '-': case '.': case '@':
    // <special initial>
    case '!': case '$': case '%': case '&': case '*': case '/': case ':': case '<':
    case '=': case '>': case '?': case '^': case '_': case '~':
    case '|':
      m_strBuf.append( (char)m_curChar );
      nextChar();
      break;

    // <inline hex escape>
    case '\\':
      nextChar();
      if (m_curChar == 'u') // \u -> identifier starting with an inline Unicode escape
      {
        nextChar();
        m_strBuf.appendCodePoint(scanUnicodeEscape(4));
      }
      else if (m_curChar == 'U') // \U -> identifier starting with an inline Unicode escape
      {
        nextChar();
        m_strBuf.appendCodePoint(scanUnicodeEscape(8));
      }
      else
      {
        error( 0, "Invalid escape in an identifier" );
        // Leave the character to be processed in the next iteration
      }
      break;

    default:
      if (isAlpha(m_curChar))
      {
        m_strBuf.appendCodePoint(m_curChar);
        nextChar();
        break;
      }
      else
        goto exitLoop;
    }
  }
exitLoop:

  m_strBuf.append( 0 );
  const gc_char * name = m_strBuf.createGCString();

  if (!isDelimiter(m_curChar))
    error( 0, "Identifier \"%s\" not terminated by a delimiter", name );

  return identifier( name );
}

Token::Enum Lexer::identifier ( const gc_char * name )
{
  m_valueIdent = m_symbolMap.newSymbol( name );
  return Token::IDENT;
}

Token::Enum Lexer::scanNumber ( unsigned state )
{
  unsigned base = 10;
  bool real = false;
  bool err = false;
  bool nnum = false; // Have we seen numeric characters
  bool dpoint = false;
  bool expo = false;

  m_strBuf.reset();

  switch (state)
  {
  default:
    assert( false );
  case 0: goto state_0;
  case 1: goto state_1;
  case 2: goto state_2;
  case 3: goto state_3;
  case 4: goto state_4;
  }

state_0:

  if (m_curChar == '+')
  {
    nextChar();
state_1:;
  }
  else if (m_curChar == '-')
  {
    nextChar();
state_2:
    m_strBuf.append( '-' );
  }


  if (m_curChar >= '0' && m_curChar <= '9')
  {
state_3:
    if (m_curChar == '0')
    {
      nextChar();
      if ((m_curChar | 32) == 'x')
      {
        m_strBuf.append( "0x", 2 );
        nextChar();
        base = 16;
      }
      else if ((m_curChar | 32) == 'b')
      {
        nextChar();
        base = 2;
      }
      else
      {
        nnum = true;
        m_strBuf.append( '0' );
        if (!err && m_curChar >= '0' && m_curChar <= '7')
        {
          err = true;
          error( -1, "C-style octal numbers are not supported" );
        }
      }
    }
    nnum |= scanUInt( base );
  }

  if (m_curChar == '.')
  {
    nextChar();
state_4:
    m_strBuf.append( '.' );
    dpoint = true;
    real = true;

    if (!err && base != 10 && base != 16)
    {
      err = true;
      m_errors->error( m_tokCoords, "Invalid floating point constant" );
    }
    if (!scanUInt( base ) && !nnum && !err) // check for something like  "+.e10"
    {
      err = true;
      m_errors->error( m_tokCoords, "Invalid floating point constant" );
    }
  }
  else
  {
    if (!err && !nnum)
    {
      err = true;
      m_errors->error( m_tokCoords, "Invalid numeric constant" );
    }
  }

  if ((m_curChar | 32) == 'e')
  {
    real = true;
    expo = true;
    if (!err && base != 10)
    {
      err = true;
      m_errors->error( m_tokCoords, "Invalid decimal floating point number" );
    }
    m_strBuf.append( 'e' );
    nextChar();
    if (m_curChar == '+')
      nextChar();
    else if (m_curChar == '-')
    {
      m_strBuf.append( '-' );
      nextChar();
    }
    scanUInt( 10 );
  }
  else if ((m_curChar | 32) == 'p')
  {
    real = true;
    expo = true;
    if (!err && base != 16)
    {
      err = true;
      m_errors->error( m_tokCoords, "Invalid hexadecimal floating point constant" );
    }
    m_strBuf.append( 'p' );
    nextChar();
    if (m_curChar == '+')
      nextChar();
    else if (m_curChar == '-')
    {
      m_strBuf.append( '-' );
      nextChar();
    }
    scanUInt( 10 );
  }

  m_strBuf.append( 0 );

  if (!err && real && base == 16)
  {
    if (!expo)
    {
      err = true;
      m_errors->error( m_tokCoords, "Exponent required in hexadecimal floating point constant" );
    }
  }

  if (!err && !isDelimiter(m_curChar))
  {
    err = true;
    m_errors->error( m_tokCoords, "Invalid number" );
  }

  if (!err)
  {
    if (!real)
    {
      errno = 0;
      m_valueInteger = std::strtoll(m_strBuf.buf(),NULL,base);
      if (errno != 0)
      {
        m_errors->error( m_tokCoords, "Integer constant overflow" );
        err = true;
      }
    }
    else
    {
      errno = 0;
      m_valueReal = std::strtod(m_strBuf.buf(),NULL);
      if (errno != 0)
      {
        m_errors->error( m_tokCoords, "Floating point constant overflow" );
        err = true;
      }
    }
  }

  if (!real)
  {
    if (err)
      m_valueInteger = 0;
    return Token::INTEGER;
  }
  else
  {
    if (err)
      m_valueReal = 0.0 / 0.0;
    return Token::REAL;
  }
}

bool Lexer::scanUInt ( unsigned base )
{
  bool nnum = false;
  for(;;)
  {
    if (m_curChar == '_')
      nextChar();
    else if (isBaseDigit(base,m_curChar))
    {
      nnum = true;
      m_strBuf.append( m_curChar );
      nextChar();
    }
    else
      break;
  }
  return nnum;
}
