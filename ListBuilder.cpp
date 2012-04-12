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


#include "ListBuilder.hpp"

ListBuilder::ListBuilder ()
{
  m_first = m_last = NULL;
  m_haveCoords = false;
}

ListBuilder & ListBuilder::operator<< ( Syntax * value )
{
  assert( value != NULL );
  SyntaxPair * d = new SyntaxPair( m_haveCoords ? (m_haveCoords=false, m_coords) : value->coords, value, NULL );

  if (m_first == NULL)
    m_first = d;
  else
    m_last->cdr = d;
  m_last = d;

  return *this;
}

SyntaxPair * ListBuilder::toList ()
{
  return toList(
    new SyntaxNil( m_haveCoords ? m_coords : (m_last ? m_last->coords : SourceCoords()) )
  );
}

SyntaxPair * ListBuilder::toList ( Syntax * cdr )
{
  assert( cdr != NULL );
  SyntaxPair * res;
  if (m_last != NULL)
  {
    m_last->cdr = cdr;
    res = m_first;
    m_first = m_last = NULL;
  }
  else
  {
    assert( cdr->sclass == SyntaxClass::PAIR || cdr->sclass == SyntaxClass::NIL );
    res = static_cast<SyntaxPair *>(cdr);
  }
  m_haveCoords = false;
  return res;
}
