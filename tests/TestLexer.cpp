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
#include "../Lexer.hpp"

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
  virtual void error ( const SourceCoords & coords, const gc_char * message )
  {
    ei = new ErrorInfo( coords, message );
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

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( std::strcmp("aaa", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( std::strcmp("\a\b\t\n\v\f\r\a\"\\", lex.valueString())==0 );

    // Octal
    //
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\1", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\12", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\123", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\123" "4", lex.valueString())==0 );

    // Hex
    //
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\x12", lex.valueString())==0 );

    // Unicode
    //
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );

    CPPUNIT_ASSERT( std::strcmp("\x09", lex.valueString())==0 );
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\x09", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    // End-of-line escape
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("aaabbb", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("aaaccc", lex.valueString())==0 );

    CPPUNIT_ASSERT( Token::EOFTOK==lex.nextToken() );
  }

  {
    CharBufInput t1(
      "\"aaa\nbbb\""
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input1.1", map, err );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
  }
  {
    CharBufInput t1(
      "\"aaa"
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input2", map, err );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
  }
  {
    CharBufInput t1(
      "\"aa\\"
    );
    SymbolTable map;
    ErrorReporter err;
    Lexer lex( t1, "input3", map, err );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
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

  CPPUNIT_ASSERT( Token::LPAR==lex.nextToken() );
  CPPUNIT_ASSERT( Token::RPAR==lex.nextToken() );
  CPPUNIT_ASSERT( Token::LSQUARE==lex.nextToken() );
  CPPUNIT_ASSERT( Token::RSQUARE==lex.nextToken() );
  CPPUNIT_ASSERT( Token::APOSTR==lex.nextToken() );
  CPPUNIT_ASSERT( Token::ACCENT==lex.nextToken() );

  CPPUNIT_ASSERT( Token::LPAR==lex.nextToken() );
  CPPUNIT_ASSERT( err.coords( 2, 1 ) );

  CPPUNIT_ASSERT( Token::COMMA==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA_AT==lex.nextToken() );

  // Test strings
  CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
  CPPUNIT_ASSERT( std::strcmp("aaa", lex.valueString())==0 );

  // Test comments
  CPPUNIT_ASSERT( Token::COMMA==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA_AT==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA_AT==lex.nextToken() );

  CPPUNIT_ASSERT( Token::LPAR==lex.nextToken() );
  CPPUNIT_ASSERT( Token::LPAR==lex.nextToken() );

  //  "aaa - -- ->\n"
  CPPUNIT_ASSERT( Token::SYMBOL==lex.nextToken() );
  CPPUNIT_ASSERT( strcmp("aaa", lex.valueSymbol()->name) == 0 );
  CPPUNIT_ASSERT( Token::SYMBOL==lex.nextToken() );
  CPPUNIT_ASSERT( strcmp("-", lex.valueSymbol()->name) == 0 );
  CPPUNIT_ASSERT( Token::SYMBOL==lex.nextToken() );
  CPPUNIT_ASSERT( strcmp("--", lex.valueSymbol()->name) == 0 );
  CPPUNIT_ASSERT( Token::SYMBOL==lex.nextToken() );
  CPPUNIT_ASSERT( strcmp("->", lex.valueSymbol()->name) == 0 );

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 'a'==lex.valueInteger() );

  CPPUNIT_ASSERT( Token::DOT==lex.nextToken() );

  CPPUNIT_ASSERT( Token::EOFTOK==lex.nextToken() );
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

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 1==lex.valueInteger() );

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 1==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( -1==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 100==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( -100==lex.valueInteger() );

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 0x100==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( -0x100==lex.valueInteger() );

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 0==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 0==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 0==lex.valueInteger() );

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( 0xA==lex.valueInteger() );
  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT( -0xA==lex.valueInteger() );

  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 1.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 2.34, lex.valueReal() );

  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0.2, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( -0.2, lex.valueReal() );

  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0.2, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0.2, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 1.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( -2.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 2.0, lex.valueReal() );

  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 3.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 4.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 5.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 6.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 7.0, lex.valueReal() );

  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 30.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 40.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0.5, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 60.0, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0.7, lex.valueReal() );

  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 1.2e3, lex.valueReal() );

  //"0x1.0p2 0x1p-3\n"
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0x1.0p2, lex.valueReal() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_EQUAL( 0x1p-3, lex.valueReal() );

  //"0xp2 0x.3\n"
  CPPUNIT_ASSERT( !err.haveErr() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "0xp2", err.haveErr() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "0x.3", err.haveErr() );

  CPPUNIT_ASSERT( Token::INTEGER==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "01", err.haveErr() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "+.e10", err.haveErr() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "1.2p10", err.haveErr() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "0x1.2e10", err.haveErr() );
  CPPUNIT_ASSERT( Token::REAL==lex.nextToken() );
  CPPUNIT_ASSERT_MESSAGE( "0b10.0", err.haveErr() );



  CPPUNIT_ASSERT( Token::EOFTOK==lex.nextToken() );
}

