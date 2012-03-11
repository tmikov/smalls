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


#include "TestUTF8.hpp"

#include "../utf-8.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION ( TestUTF8 );

TestUTF8::TestUTF8 ( )
{
}

TestUTF8::~TestUTF8 ( )
{
}

void TestUTF8::setUp ( )
{
}

void TestUTF8::tearDown ( )
{
}

class ErrorReporter : public IStreamErrorReporter
{
public:
  int errorCount;
  
  ErrorReporter () { errorCount = 0; };
  
  virtual void error ( off_t offset, const char * message )
  {
    std::cerr << "\n     error at offset " << offset << ":" << message << std::endl;
    ++this->errorCount;
  }
};

void TestUTF8::testUTF8StreamDecoder ( )
{
  ErrorReporter errors;
  CharBufInput t0("a");
  UTF8StreamDecoder dec0( t0, errors );
  CPPUNIT_ASSERT( 'a' == dec0.get() );
  CPPUNIT_ASSERT_EQUAL( -1, dec0.get() );
  
  CharBufInput t1(
    "ab"
    "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E" 
    "\xEF\xBB\xBF\xF0\xA3\x8E\xB4"
  );
  UTF8StreamDecoder dec1( t1, errors );
  
  CPPUNIT_ASSERT( 'a' == dec1.get() );
  CPPUNIT_ASSERT( 'b' == dec1.get() );

  CPPUNIT_ASSERT_EQUAL(  0x65E5, dec1.get() );
  CPPUNIT_ASSERT_EQUAL(  0x672C, dec1.get() );
  CPPUNIT_ASSERT_EQUAL(  0x8A9E, dec1.get() );
  
  CPPUNIT_ASSERT_EQUAL(  0xFEFF, dec1.get() );
  CPPUNIT_ASSERT_EQUAL( 0x233B4, dec1.get() );
  
  CPPUNIT_ASSERT_EQUAL( -1, dec1.get() );
  CPPUNIT_ASSERT_EQUAL( 0, errors.errorCount );
  
  // Force an error
  CharBufInput t2( 
    "a"
    "\xC0\x80"
  );
  UTF8StreamDecoder dec2( t2, errors );
  
  CPPUNIT_ASSERT( 'a' == dec2.get() );
  CPPUNIT_ASSERT_EQUAL( UNICODE_REPLACEMENT_CHARACTER, dec2.get() );
  CPPUNIT_ASSERT_EQUAL( 1, errors.errorCount );
  CPPUNIT_ASSERT_EQUAL( -1, dec2.get() );
}

void TestUTF8::testUTF8Encoder ()
{
  char buf[8];
  int len;
  
  len = encodeUTF8( buf, 0 );
  CPPUNIT_ASSERT_EQUAL( 1, len );
  CPPUNIT_ASSERT( std::memcmp( buf, "\x00", 1 ) == 0 );
  
  len = encodeUTF8( buf, 'a' );
  CPPUNIT_ASSERT_EQUAL( 1, len );
  CPPUNIT_ASSERT( std::memcmp( buf, "a", 1 ) == 0 );
  
  len = encodeUTF8( buf, 0x233B4 );
  CPPUNIT_ASSERT_EQUAL( 4, len );
  CPPUNIT_ASSERT( std::memcmp( buf, "\xF0\xA3\x8E\xB4", 4 ) == 0 );
  
  len = encodeUTF8( buf, 0x65E5 );
  CPPUNIT_ASSERT_EQUAL( 3, len );
  CPPUNIT_ASSERT( std::memcmp( buf, "\xE6\x97\xA5", 3 ) == 0 );
}
