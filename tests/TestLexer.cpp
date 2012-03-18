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
#include "../lexer.hpp"

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
    SymbolMap map;
    ErrorReporter err;
    Lexer lex( t1, "input1", map, err );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( std::strcmp("aaa", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( std::strcmp("\a\b\t\n\v\f\r\a\"\\", lex.getValueString())==0 );

    // Octal
    //
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\1", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\12", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\123", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\123" "4", lex.getValueString())==0 );

    // Hex
    //
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\x12", lex.getValueString())==0 );

    // Unicode
    //
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );

    CPPUNIT_ASSERT( std::strcmp("\x09", lex.getValueString())==0 );
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("\x09", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );

    // End-of-line escape
    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("aaabbb", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( !err.haveErr() );
    CPPUNIT_ASSERT( std::strcmp("aaaccc", lex.getValueString())==0 );

    CPPUNIT_ASSERT( Token::EOFTOK==lex.nextToken() );
  }

  {
    CharBufInput t1(
      "\"aaa\nbbb\""
    );
    SymbolMap map;
    ErrorReporter err;
    Lexer lex( t1, "input1.1", map, err );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
  }
  {
    CharBufInput t1(
      "\"aaa"
    );
    SymbolMap map;
    ErrorReporter err;
    Lexer lex( t1, "input2", map, err );

    CPPUNIT_ASSERT( Token::STR==lex.nextToken() );
    CPPUNIT_ASSERT( err.haveErr() );
  }
  {
    CharBufInput t1(
      "\"aa\\"
    );
    SymbolMap map;
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

    "\"aaa\""

    "/* aaa */,"
    "/* aaa\n"
    "*/,@"
    "/*  /* a */ */,"
    "/*  \"*/\"() */,@"

    "// , ,\n"
    "(\n"
    "; , ,\n"
    "(\n"
  );
  SymbolMap map;
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
  CPPUNIT_ASSERT( std::strcmp("aaa", lex.getValueString())==0 );

  // Test comments
  CPPUNIT_ASSERT( Token::COMMA==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA_AT==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA==lex.nextToken() );
  CPPUNIT_ASSERT( Token::COMMA_AT==lex.nextToken() );

  CPPUNIT_ASSERT( Token::LPAR==lex.nextToken() );
  CPPUNIT_ASSERT( Token::LPAR==lex.nextToken() );

  CPPUNIT_ASSERT( Token::EOFTOK==lex.nextToken() );
}

