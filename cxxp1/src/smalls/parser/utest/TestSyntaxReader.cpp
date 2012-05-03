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
#include "TestSyntaxReader.hpp"
#include "SyntaxReader.hpp"

using namespace p1;
using namespace p1::smalls;

CPPUNIT_TEST_SUITE_REGISTRATION ( TestSyntaxReader );

TestSyntaxReader::TestSyntaxReader ( )
{
}

TestSyntaxReader::~TestSyntaxReader ( )
{
}

void TestSyntaxReader::setUp ( )
{
}

void TestSyntaxReader::tearDown ( )
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

static void validate ( SymbolTable & map, ErrorReporter & err, Syntax * d )
{
  CPPUNIT_ASSERT( !err.haveErr() );

  std::stringstream st;
  st << *d;
  std::string str = st.str();
  //std::cout << "\nValidating " << str << "\n";

  CharBufInput t1( str );
  Lexer lex( t1, "tmpinput", map, err );
  SyntaxReader parser( lex );

  Syntax * d1 = parser.parseDatum();
  CPPUNIT_ASSERT( !err.haveErr() );
  CPPUNIT_ASSERT( d->equal(d1) );

  std::stringstream st1;
  st1 << *d1;
  std::string str1 = st1.str();
  CPPUNIT_ASSERT_EQUAL( str, str1 );

  std::cerr << "\nValidated " << str << " == " << str1 << "\n";
}

void TestSyntaxReader::testParser ( )
{
  SymbolTable map;
  ErrorReporter err;
  CharBufInput t1(
  "1000\n"
  "(this is a list)\n"
  "(this list (has 1 nested list))\n"
  "(this . (is . (strange . ())))\n"
  "(a . b)\n"
  "\n"
  "(define (fact x)\n"
  "  (let lp ((tot 1) (x x)) ; Scheme-style comment here\n"
  "     (if (< x 2) #;(datum comment here)\n"
  "        tot // end-ofline comment here \n"
  "        (lp ((* tot x) (- x 1)))))) /*this is a C-style comment*/ \n"

  "(a . )\n"
  );
  Lexer lex( t1, "input1", map, err );
  SyntaxReader parser( lex );
  Syntax * d;

  d = parser.parseDatum();
  CPPUNIT_ASSERT( SyntaxClass::INTEGER==d->sclass );
  CPPUNIT_ASSERT( 1000==((SyntaxValue *)d)->u.integer );
  validate( map, err, d );

  d = parser.parseDatum();
  CPPUNIT_ASSERT( SyntaxClass::PAIR==d->sclass );
  validate( map, err, d );

  d = parser.parseDatum();
  CPPUNIT_ASSERT( SyntaxClass::PAIR==d->sclass );
  validate( map, err, d );

  d = parser.parseDatum();
  CPPUNIT_ASSERT( SyntaxClass::PAIR==d->sclass );
  validate( map, err, d );

  d = parser.parseDatum();
  CPPUNIT_ASSERT( SyntaxClass::PAIR==d->sclass );
  validate( map, err, d );

  d = parser.parseDatum();
  CPPUNIT_ASSERT( SyntaxClass::PAIR==d->sclass );
  validate( map, err, d );

  CPPUNIT_ASSERT( !err.haveErr() );
  d = parser.parseDatum();
  CPPUNIT_ASSERT( err.haveErr() );
}
