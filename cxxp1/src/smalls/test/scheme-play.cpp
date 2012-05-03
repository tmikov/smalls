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
#include "p1/util/FastStdioInput.hpp"
#include "p1/smalls/parser/SyntaxReader.hpp"
#include "p1/smalls/parser/SchemeParser.hpp"
#include "ListBuilder.hpp"
#include <iostream>

using namespace p1;
using namespace p1::smalls;
using namespace p1::smalls::detail;

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
    std::cerr << ei->formatMessage() << std::endl;
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


int main ( int argc, const char ** argv )
{
  const char * fileName = argv[1];

  SymbolTable symTab;
  ErrorReporter errors;
  FastStdioInput fi(fileName,"rb");
  Lexer lex( fi, fileName, symTab, errors );
  SyntaxReader dp( lex );

  ListBuilder lb;
  Syntax * d;
  while ((d = dp.parseDatum()) != dp.DAT_EOF)
    lb << d;

  SyntaxPair * body = lb;

  if (false)
  {
    std::cout << "Parsed datum:\n";
    Syntax::toStreamIndented( std::cout, 0, body );
    std::cout << "\n\n";
  }

  SchemeParser par( symTab, errors );
  std::cout << par.compileLibraryBody( body );

  return 0;
}
