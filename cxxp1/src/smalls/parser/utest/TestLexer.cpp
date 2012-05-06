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
#include "TestLexer.hpp"
#include "Lexer.hpp"

using namespace p1;
using namespace p1::smalls;

CPPUNIT_TEST_SUITE_REGISTRATION ( TestLexer );

TestLexer::TestLexer ( )
{
}

TestLexer::~TestLexer ( )
{
}

void TestLexer::setUp ( )
{
}

void TestLexer::tearDown ( )
{
}

namespace
{
;

class ErrorReporter : public AbstractErrorReporter
{
public:
  int count;
  bool err;
  ErrorInfo * ei;

  ErrorReporter () : count( 0 ), err( false ), ei(NULL) {}
  virtual void error ( const ErrorInfo & errI )
  {
    ei = new ErrorInfo( errI );
    err = true;
    ++count;
    std::cerr << "  " << ei->formatMessage() << std::endl;
  }

  bool haveErr ()
  {
    bool e = err;
    err = false;
    return e;
  }

  bool coords ( unsigned line, unsigned column )
  {
    ErrorInfo * e = ei;
    ei = NULL;
    return e && e->coords.line == line && e->coords.column == column;
  }
};


};

void TestLexer::testStrings ()
{
  {
    CharBufInput t1(
      "\"aaa\"\n"
      "\"\\w\"\n"
      "\"\\a\\b\\t\\n\\v\\f\\r\\a\\\"\\\\\"\n"

      "\"\\8\""
      "\"\\1\""
      "\"\\12\""
      "\"\\123\""
      "\"\\1234\"\n"

      "\"\\xt\""
      "\"\\x0t\""
      "\"\\x12\"\n"

      "\"\\ut\""
      "\"\\u000t\""
      "\"\\u0009\""
      "\"\\U0009\""
      "\"\\U00000009\""
      "\"\\U0011FFFF\"\n"

      "\"aaa\\\nbbb\""
      "\"aaa\\  \nccc\""
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input1", map, err );
    Token tok;

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( std::strcmp("aaa", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( std::strcmp("\a\b\t\n\v\f\r\a\"\\", tok.string())==0 );

    // Octal
    //
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\1", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\12", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\123", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\123" "4", tok.string())==0 );

    // Hex
    //
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\x12", tok.string())==0 );

    // Unicode
    //
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );

    CPPUNIT_ASSERT( std::strcmp("\x09", tok.string())==0 );
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\x09", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );

    // End-of-line escape
    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("aaabbb", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("aaaccc", tok.string())==0 );

    CPPUNIT_ASSERT( TokenKind::EOFTOK==lex.nextToken( tok ) );
  }

  {
    CharBufInput t1(
      "\"aaa\nbbb\""
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input1.1", map, err );
    Token tok;

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );
  }
  {
    CharBufInput t1(
      "\"aaa"
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input2", map, err );
    Token tok;

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );
  }
  {
    CharBufInput t1(
      "\"aa\\"
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input3", map, err );
    Token tok;

    CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
    CPPUNIT_ASSERT( err.haveErr() );
  }
}

void TestLexer::testLexer ( )
{
  CharBufInput t1(
    "( ) [ ] ' ` \n"
    "*/(\n"
    ", ,@"

    "\"aaa\" "

    "/* aaa */,"
    "/* aaa\n"
    "*/,@"
    "/*  /* a */ */,"
    "/*  \"*/\"() */,@"

    "// , ,\n"
    "(\n"
    "; , ,\n"
    "(\n"

    "aaa - -- ->\n"

    "#\"a\"\n"

    ".\n"
  );
  SymbolTable map;
  ErrorReporter err;
  Lexer lex( t1, "input4", map, err );
  Token tok;

  CPPUNIT_ASSERT( TokenKind::LPAR==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::RPAR==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::LSQUARE==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::RSQUARE==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::APOSTR==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::ACCENT==lex.nextToken( tok ) );

  CPPUNIT_ASSERT( TokenKind::LPAR==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( err.coords( 2, 1 ) );

  CPPUNIT_ASSERT( TokenKind::COMMA==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::COMMA_AT==lex.nextToken( tok ) );

  // Test strings
  CPPUNIT_ASSERT( TokenKind::STR==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( std::strcmp("aaa", tok.string())==0 );

  // Test comments
  CPPUNIT_ASSERT( TokenKind::COMMA==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::COMMA_AT==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::COMMA==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::COMMA_AT==lex.nextToken( tok ) );

  CPPUNIT_ASSERT( TokenKind::LPAR==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( TokenKind::LPAR==lex.nextToken( tok ) );

  //  "aaa - -- ->\n"
  CPPUNIT_ASSERT( TokenKind::SYMBOL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( strcmp("aaa", tok.symbol()->name) == 0 );
  CPPUNIT_ASSERT( TokenKind::SYMBOL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( strcmp("-", tok.symbol()->name) == 0 );
  CPPUNIT_ASSERT( TokenKind::SYMBOL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( strcmp("--", tok.symbol()->name) == 0 );
  CPPUNIT_ASSERT( TokenKind::SYMBOL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( strcmp("->", tok.symbol()->name) == 0 );

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 'a'==tok.integer() );

  CPPUNIT_ASSERT( TokenKind::DOT==lex.nextToken( tok ) );

  CPPUNIT_ASSERT( TokenKind::EOFTOK==lex.nextToken( tok ) );
}

void TestLexer::testLexer2 ( )
{
  CharBufInput t1(
  "1\n"
  "+1 -1 +100 -100\n"
  "0x100 -0x100\n"
  "0 -0 +0\n"
  "0b1010 -0b1010\n"

  "1.0 2.34\n"
  "+0.2 -0.2\n"
  ".2 +.2 +1. -2. 2.\n"
  "3e0 4e+0 5e-0 6.0e0 7.0e-0\n"
  "3e1 4e+1 5e-1 6.0e1 7.0e-1\n"
  "1.2e3\n"

  "0x1.0p2 0x1p-3\n"
  "0xp2 0x.3\n"

  "01 +.e10 1.2p10\n"
  "0x1.2e10 0b10.0\n"
  );
  SymbolTable map;
  ErrorReporter err;
  Lexer lex( t1, "input5", map, err );
  Token tok;

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 1==tok.integer() );

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 1==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( -1==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 100==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( -100==tok.integer() );

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 0x100==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( -0x100==tok.integer() );

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 0==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 0==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 0==tok.integer() );

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( 0xA==tok.integer() );
  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT( -0xA==tok.integer() );

  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 1.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 2.34, tok.real() );

  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0.2, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( -0.2, tok.real() );

  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0.2, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0.2, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 1.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( -2.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 2.0, tok.real() );

  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 3.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 4.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 5.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 6.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 7.0, tok.real() );

  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 30.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 40.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0.5, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 60.0, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0.7, tok.real() );

  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 1.2e3, tok.real() );

  //"0x1.0p2 0x1p-3\n"
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0x1.0p2, tok.real() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_EQUAL( 0x1p-3, tok.real() );

  //"0xp2 0x.3\n"
  CPPUNIT_ASSERT( !err.haveErr() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "0xp2", err.haveErr() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "0x.3", err.haveErr() );

  CPPUNIT_ASSERT( TokenKind::INTEGER==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "01", err.haveErr() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "+.e10", err.haveErr() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "1.2p10", err.haveErr() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "0x1.2e10", err.haveErr() );
  CPPUNIT_ASSERT( TokenKind::REAL==lex.nextToken( tok ) );
  CPPUNIT_ASSERT_MESSAGE( "0b10.0", err.haveErr() );



  CPPUNIT_ASSERT( TokenKind::EOFTOK==lex.nextToken( tok ) );
}

