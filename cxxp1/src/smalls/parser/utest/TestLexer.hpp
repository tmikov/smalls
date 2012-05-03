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


#ifndef TESTLEXER_HPP
#define	TESTLEXER_HPP

#include <cppunit/extensions/HelperMacros.h>

class TestLexer : public CPPUNIT_NS::TestFixture {
  CPPUNIT_TEST_SUITE(TestLexer);
  CPPUNIT_TEST(testLexer);
  CPPUNIT_TEST(testLexer2);
  CPPUNIT_TEST(testStrings);
  CPPUNIT_TEST_SUITE_END();

public:
  TestLexer();
  virtual ~TestLexer();
  void setUp();
  void tearDown();

private:
  void testLexer();
  void testLexer2();
  void testStrings();
};

#endif	/* TESTLEXER_HPP */

