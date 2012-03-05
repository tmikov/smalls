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


#include "TestSymbolMap.hpp"
#include "../lexer.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION ( TestSymbolMap );

TestSymbolMap::TestSymbolMap ( )
{
}

TestSymbolMap::~TestSymbolMap ( )
{
}

void TestSymbolMap::setUp ( )
{
}

void TestSymbolMap::tearDown ( )
{
}

void TestSymbolMap::testMethod ( )
{
  SymbolMap sm;
  Symbol * t1 = sm.newSymbol( "aaa" );
  Symbol * t2 = sm.newSymbol( "bbb ");
  Symbol * t3 = sm.newSymbol( "aaa" );
  Symbol * t4 = sm.newSymbol( (std::string("aa")+"a").c_str() );
  
  CPPUNIT_ASSERT( t1 != t2 );
  CPPUNIT_ASSERT( t1 == t3 );
  CPPUNIT_ASSERT( t1 == t4 );
}

