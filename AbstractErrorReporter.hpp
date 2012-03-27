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


#ifndef ABSTRACTERRORREPORTER_HPP
#define	ABSTRACTERRORREPORTER_HPP

#include "base.hpp"

class SourceCoords
{
public:
  const gc_char * fileName;
  unsigned line, column;

  SourceCoords () : fileName( NULL ), line(0), column(0) {};

  SourceCoords ( const gc_char * fileName_, unsigned line_, unsigned column_ )
    : fileName( fileName_ ), line( line_ ), column( column_ )
  {};

  bool operator == ( const SourceCoords & x ) const
  {
    return fileName == fileName && line == x.line && column == x.column;
  }
};

class ErrorInfo : public gc
{
public:
  const SourceCoords coords;
  const gc_char * const message;

  ErrorInfo ( const SourceCoords & coords_, const gc_char * message_ )
    : coords( coords_ ), message( message_ )
  {}

  std::string formatMessage () const;
};

class AbstractErrorReporter
{
public:
  virtual void error ( const SourceCoords & coords, const gc_char * message ) = 0;

  void verrorFormat ( const SourceCoords & coords, const gc_char * message, std::va_list ap );
  void errorFormat ( const SourceCoords & coords, const gc_char * message, ... );
};

#endif	/* ABSTRACTERRORREPORTER_HPP */

