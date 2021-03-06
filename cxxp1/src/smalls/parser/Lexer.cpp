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
#include "Lexer.hpp"
#include "p1/util/format-str.hpp"
#include <cctype>
#include <cerrno>

using namespace p1;
using namespace p1::smalls;

Lexer::Lexer ( FastCharInput & in, const gc_char * fileName, SymbolTable & symbolTable, AbstractErrorReporter & errors )
  : m_fileName( fileName ), m_symbolTable( symbolTable ), m_errors( &errors ),
    m_tokCoords( fileName, 0, 0 ), m_streamErrors( *this ), m_decoder( in, m_streamErrors )
{
  m_curChar = 0;
  m_inNestedComment = false;

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
  case 16: ch |= 32; return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
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

void Lexer::_nextToken ( Token & tok )
{
  for(;;)
  {
    saveCoords( tok );
    switch (m_curChar)
    {
    case -1: tok.kind( TokenKind::EOFTOK ); return;

    case '(': nextChar(); tok.kind( TokenKind::LPAR ); return;
    case ')': nextChar(); tok.kind( TokenKind::RPAR ); return;
    case '[': nextChar(); tok.kind( TokenKind::LSQUARE ); return;
    case ']': nextChar(); tok.kind( TokenKind::RSQUARE ); return;
    case '\'': nextChar(); tok.kind( TokenKind::APOSTR ); return;
    case '`': nextChar(); tok.kind( TokenKind::ACCENT ); return;

    // nested commend end handling.
    case '*':
      nextChar();
      if (m_curChar == '/')
      {
        nextChar();
        if (m_inNestedComment)
        {
          tok.kind( TokenKind::NESTED_COMMENT_END );
          return;
        }
        else
          m_errors->error( m_tokCoords, "Unexpected */" );
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append( '*' );
        scanRemainingIdentifier( tok );
        return;
      }
      break;

    case ',':
      nextChar();
      if (m_curChar == '@')
      {
        nextChar();
        tok.kind( TokenKind::COMMA_AT );
        return;
      }
      else
      {
        tok.kind( TokenKind::COMMA );
        return;
      }

    // <string>
    case '"':
      nextChar();
      scanString( tok );
      return;

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
          {
            tok.kind( TokenKind::NESTED_COMMENT_START );
            return;
          }
        }
        else
        {
          m_strBuf.reset();
          m_strBuf.append( '/' );
          scanRemainingIdentifier( tok );
          return;
        }
      }
      break;


    // <digit>
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      scanNumber(tok, 3);
      return;

    case '#':
      nextChar();
      switch (m_curChar)
      {
      case ';': // #; datum comment
        nextChar();
        tok.kind( TokenKind::DATUM_COMMENT );
        return;

      case '(':  /* #( */ nextChar(); tok.kind( TokenKind::HASH_LPAR ); return;
      case '\'': /* #' */ nextChar(); tok.kind( TokenKind::HASH_APOSTR ); return;
      case '`':  /* #` */ nextChar(); tok.kind( TokenKind::HASH_ACCENT ); return;
      case ',':  /* #, #,@ */
        nextChar();
        if (m_curChar == '@') // #,@
        {
          nextChar();
          tok.kind( TokenKind::HASH_COMMA_AT );
          return;
        }
        else
        {
          tok.kind( TokenKind::HASH_COMMA );
          return;
        }

      case 't': case 'T': // #t #T
        nextChar();
        if (!isDelimiter(m_curChar))
          error( 0, "Bad #x form" );
        tok.vbool( true );
        return;
      case 'f': case 'F': // #f #F
        nextChar();
        if (!isDelimiter(m_curChar))
          error( 0, "Bad #x form" );
        tok.vbool( false );
        return;

      case '"': //  character constant
        nextChar();
        scanCharacterConstant( tok );
        return;

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
        scanRemainingIdentifier( tok );
        return;
      }

    // <peculiar identifier> "+"
    case '+':
      nextChar();
      if ((m_curChar >= '0' && m_curChar <= '9') || m_curChar == '.')
      {
        scanNumber(tok, 1);
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append( '+' );
        scanRemainingIdentifier( tok );
      }
      return;

    // <peculiar identifier> "-" "->"
    case '-':
      nextChar();
      if ((m_curChar >= '0' && m_curChar <= '9') || m_curChar == '.')
      {
        scanNumber(tok, 2);
      }
      else
      {
        m_strBuf.reset();
        m_strBuf.append( '-' );
        scanRemainingIdentifier( tok );
      }
      return;

    // <peculiar identifier> "..."
    case '.':
      nextChar();
      if (isDelimiter(m_curChar))
        tok.kind( TokenKind::DOT );
      else if (m_curChar >= '0' && m_curChar <= '9')
        scanNumber(tok, 4);
      else
      {
        m_strBuf.reset();
        m_strBuf.append('.');
        scanRemainingIdentifier( tok );
      }
      return;

    // <inline hex escape>
    case '\\':
      scanRemainingIdentifier( tok );
      return;

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
        scanRemainingIdentifier( tok );
        return;
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
  virtual void error ( const ErrorInfo & ei )
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
  Token tok;
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
      switch (nextToken( tok ))
      {
      case TokenKind::NESTED_COMMENT_START:
        ++level;
        break;
      case TokenKind::NESTED_COMMENT_END:
        if (--level == 0)
          goto exitLoop;
        break;
      case TokenKind::EOFTOK:
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

  if (tok.kind() == TokenKind::EOFTOK)
    error( 0, "EOF in comment started on line %u", nestedCommentStart.line );
}

void Lexer::scanCharacterConstant ( Token & tok )
{
  if (m_curChar == '"')
  {
    error( 0, "Invalid empty character constant" );
    nextChar();
    tok.integer( ' ' );
    return;
  }
  int32_t value;
  switch (scanSingleCharacter( value ))
  {
  case TokenKind::INTEGER: // 8-bit character
  case TokenKind::STR:     // Unicode codepoint
    break;
  case TokenKind::NONE:    // error
    value = ' ';
    break;
  case TokenKind::EOFTOK:  // eof/error
    tok.integer(' ');
    return;
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

  tok.integer( value );
}

void Lexer::scanString ( Token & tok )
{
  m_strBuf.reset();

  for(;;)
  {
    if (m_curChar == '"')
    {
      nextChar();
      break;
    }
    int32_t value;
    switch (scanSingleCharacter( value ))
    {
    case TokenKind::INTEGER:
      m_strBuf.append( (char)value );
      break;
    case TokenKind::STR:
      m_strBuf.appendCodePoint( (int32_t)value );
      break;
    case TokenKind::EOFTOK:
      goto exitLoop;
    case TokenKind::NONE:
      break;
    default:
      assert( false );
    }
  }
exitLoop:

  if (!isDelimiter(m_curChar))
    error( 0, "String not followed by a delimiter" );

  m_strBuf.append( 0 );
  tok.string( m_strBuf.createGCString() );
}

/**
 * Process one character and store it in {value}. The return value:
 * <ul>
 * <li>Token::EOFTOK - stop processing the string constant (we reached EOF, LF)</li>
 * <li>Token::NONE - error handling this character
 * <li>Token::INTEGER - a 8-bit integer (possibly invalid Unicode character)
 * <li>Token::STRING - a 32-bit validated Unicode codepoint
 * </ul>
 */
TokenKind::Enum Lexer::scanSingleCharacter ( int32_t & value )
{
loop: // We loop only if we encounter \ LF sequence.
  if (m_curChar < 0)
  {
    error( 0, "Unterminated string constant at end of input. String started on line %u column %u",
            m_tokCoords.line, m_tokCoords.column );
    return TokenKind::EOFTOK;
  }
  else if (m_curChar == '\n')
  {
    m_errors->error( m_tokCoords, "Unterminated string constant" );
    return TokenKind::EOFTOK;
  }
  else if (m_curChar == '\\')
  {
    nextChar();
    switch (m_curChar)
    {
    case -1:
      error( 0, "Unterminated string escape at end of input. String started on line %u column %u",
              m_tokCoords.line, m_tokCoords.column );
      return TokenKind::EOFTOK;

    case 'a': value = '\a'; nextChar(); return TokenKind::INTEGER;
    case 'b': value = '\b'; nextChar(); return TokenKind::INTEGER;
    case 't': value = '\t'; nextChar(); return TokenKind::INTEGER;
    case 'n': value = '\n'; nextChar(); return TokenKind::INTEGER;
    case 'v': value = '\v'; nextChar(); return TokenKind::INTEGER;
    case 'f': value = '\f'; nextChar(); return TokenKind::INTEGER;
    case 'r': value = '\r'; nextChar(); return TokenKind::INTEGER;
    case '"': value = '"'; nextChar();  return TokenKind::INTEGER;
    case '\\': value = '\\'; nextChar(); return TokenKind::INTEGER;

    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
      value = scanOctalEscape();
      return TokenKind::INTEGER;
    case 'x':
      nextChar();
      value = scanHexEscape();
      return TokenKind::INTEGER;
    case 'u':
      nextChar();
      value = scanUnicodeEscape(4);
      return TokenKind::STR;
    case 'U':
      nextChar();
      value = scanUnicodeEscape(8);
      return TokenKind::STR;

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
        return TokenKind::EOFTOK;
      }
      else // Invalid escape!
      {
        // Slightly tricky. We can't display Unicode characters directly. We need to convert them
        // to utf-8 strings
        char tmp[8];
        tmp[encodeUTF8( tmp, m_curChar )] = 0;
        error( 0, "Invalid string escape \\%s", tmp );
        nextChar();
        return TokenKind::NONE;
      }
      break;
    }
  }
  else
  {
    value =  m_curChar;
    nextChar();
    return TokenKind::STR;
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

void Lexer::scanRemainingIdentifier ( Token & tok )
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
    case '!': case '$': case '%': case '&': /*case '*':*/case '/': case ':': case '<':
    case '=': case '>': case '?': case '^': case '_': case '~':
    case '|':
      m_strBuf.append( (char)m_curChar );
      nextChar();
      break;

    case '*':
      nextChar();
      if (m_curChar != '/')
        m_strBuf.append( '*' );
      else
      {
        nextChar();
        if (m_inNestedComment)
        {
          tok.kind( TokenKind::NESTED_COMMENT_END );
          return;
        }
        else
          m_errors->error( m_tokCoords, "Unexpected */" );
      }
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

  identifier( tok, name );
}

void Lexer::identifier ( Token & tok, const gc_char * name )
{
  tok.symbol( m_symbolTable.newSymbol( name ) );
}

void Lexer::scanNumber ( Token & tok, unsigned state )
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

  int64_t valueInteger;
  double valueReal;
  if (!err)
  {
    if (!real)
    {
      errno = 0;
      valueInteger = std::strtoll(m_strBuf.buf(),NULL,base);
      if (errno != 0)
      {
        m_errors->error( m_tokCoords, "Integer constant overflow" );
        err = true;
      }
    }
    else
    {
      errno = 0;
      valueReal = std::strtod(m_strBuf.buf(),NULL);
      if (errno != 0)
      {
        m_errors->error( m_tokCoords, "Floating point constant overflow" );
        err = true;
      }
    }
  }

  if (!real)
    tok.integer( !err ? valueInteger : 0 );
  else
    tok.real( !err ? valueReal : 0.0 / 0.0 );
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
