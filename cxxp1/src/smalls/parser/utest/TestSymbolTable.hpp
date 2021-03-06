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


#ifndef TESTSYMBOLTABLE_HPP
#define	TESTSYMBOLTABLE_HPP

#include <cppunit/extensions/HelperMacros.h>

class TestSymbolTable : public CPPUNIT_NS::TestFixture {
  CPPUNIT_TEST_SUITE(TestSymbolTable);
  CPPUNIT_TEST(testMethod);
  CPPUNIT_TEST_SUITE_END();

public:
  TestSymbolTable();
  virtual ~TestSymbolTable();
  void setUp();
  void tearDown();

private:
  void testMethod();
};

#endif	/* TESTSYMBOLTABLE_HPP */

