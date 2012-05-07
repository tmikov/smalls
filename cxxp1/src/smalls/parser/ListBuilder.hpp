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


#ifndef P1_SMALLS_PARSER_LISTBUILDER_HPP
#define	P1_SMALLS_PARSER_LISTBUILDER_HPP

#include "Syntax.hpp"

namespace p1 {
namespace smalls {
namespace detail {

using namespace p1::smalls;

class ListBuilder
{
  SyntaxPair * m_first, * m_last;
  SourceCoords m_coords;
  bool m_haveCoords;
public:
  ListBuilder ();

  ListBuilder & operator<< ( Syntax * d );

  ListBuilder & operator<< ( const SourceCoords & coords )
  {
    if (!m_haveCoords)
    {
      m_coords = coords;
      m_haveCoords = true;
    }
    return * this;
  }

  SyntaxPair * toList ();

  SyntaxPair * toList ( Syntax * cdr );

  operator SyntaxPair * () { return toList(); };
};

}}} // namespaces

#endif	/* P1_SMALLS_PARSER_LISTBUILDER_HPP */
