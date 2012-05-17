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

#ifndef P1_SMALLS_PARSER_MATCHER_HPP
#define	P1_SMALLS_PARSER_MATCHER_HPP

#include "p1/util/gc-support.hpp"
#include <list>

namespace p1 {
namespace smalls {
  class Symbol;
  class Syntax;
  class SyntaxValue;
}}


namespace p1 {
namespace smalls {

typedef void * PatternVar;
typedef void * PatternRep;

class PatternBuilder
{
public:
  PatternVar literal ( Symbol * sym );
  PatternVar literal ( SyntaxValue * val );
  // TODO: vector support
  PatternVar anything ();
  void openList ();
  void closeList ();
  void closeListWithRest ();
  void repeatNext ();
private:
};

class Pattern
{
public:
  Pattern ( PatternBuilder & pb );
};

struct MatchedKind
{
  enum Enum
  {
    SYNTAX,
    REPEAT
  };
};

class Matched : public gc
{
public:
  const MatchedKind::Enum kind;
  unsigned const level;

  Matched ( MatchedKind::Enum kind_, unsigned level_ ) : kind(kind_), level(level_) {}

  static bool classof ( const Matched * ) { return true; }
};

class MatchedSyntax : public Matched
{
public:
  Syntax * const syntax;

  MatchedSyntax ( Syntax * syntax_, unsigned level_ )
    : Matched( MatchedKind::SYNTAX, level_ ), syntax(syntax_)
  {}

  static bool classof ( const MatchedSyntax * ) { return true; }
  static bool classof ( const Matched * t ) { return t->kind == MatchedKind::SYNTAX; }
};

class MatchedRepeat : public Matched
{
public:
  MatchedRepeat ( unsigned level_ )
    : Matched( MatchedKind::REPEAT, level_ )
  {}

  static bool classof ( const MatchedRepeat * ) { return true; }
  static bool classof ( const Matched * t ) { return t->kind == MatchedKind::REPEAT; }

  typedef std::list<Matched *, gc_allocator<Matched *> > MatchedList;
  const MatchedList & list () const;
};

class Matcher
{
public:
  Matcher ( const Pattern & pat );
  bool match ( Syntax * s );

  Matched * matched ( PatternVar v );
};

#if 0
/*
 (define-syntax or
   (syntax-rules ()
     ( (_ ((a b)... c)... )   ((c b)... ...)
 */

/*
 (define-syntax or
   (syntax-rules ()
     ((_) #t)
     ((_ a) a)
     ((_ a b c ...) (let ((tmp a)) (if tmp tmp (or b c ...))))))
 */
Syntax * myor ( Syntax * s )
{
  PatternBuilder b1, b2, b3;
  b1.openList();
  b1.anything();
  b1.closeList();
  Pattern p1( b1 );

  b2.openList();
  b2.anything();
  PatternVar b2_a = b2.anything();
  b2.closeList();
  Pattern p2( b2 );

  b3.openList();
  b3.anything();
  PatternVar b3_a = b3.anything();
  PatternVar b3_b = b3.anything();
  b3.repeatNext();
  PatternVar b3_c = b3.anything();
  b3.closeList();
  Pattern p3( b3 );


  Matcher m1( p1 );
  Matcher m2( p2 );
  Matcher m3( p3 );

  if (m1.match( s ))
  {
    return new SyntaxValue( SyntaxKind::BOOL, s->coords, true );
  }
  else if (m2.match( s ))
  {
    return m2.getPatternVar( b2_a );
  }
  else if (m3.match( s ))
  {

  }
}
#endif

}} // namespace

#endif	/* P1_SMALLS_PARSER_MATCHER_HPP */
