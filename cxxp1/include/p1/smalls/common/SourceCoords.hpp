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


#ifndef P1_SMALLS_COMMON_SOURCECOORDS_HPP
#define P1_SMALLS_COMMON_SOURCECOORDS_HPP

#include "p1/util/gc-support.hpp"

namespace p1 {
namespace smalls {

class SourceCoords
{
public:
  const gc_char * fileName;
  unsigned short line, column;

  SourceCoords () : fileName( NULL ), line(0), column(0) {};

  SourceCoords ( const gc_char * fileName_, unsigned line_, unsigned column_ )
    : fileName( fileName_ ), line( line_ ), column( column_ )
  {};

  bool operator == ( const SourceCoords & x ) const
  {
    return fileName == fileName && line == x.line && column == x.column;
  }

  bool full () const
  {
    return this->fileName && this->line && this->column;
  }

  std::string toString () const;
};

std::ostream & operator<< ( std::ostream & os, const SourceCoords & sc );

}} // namespaces

#endif /* P1_SMALLS_COMMON_SOURCECOORDS_HPP */

