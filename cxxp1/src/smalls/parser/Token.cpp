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
===============================================================================
   Token encapsulates all data returned by the Lexer for every token.
*/
#include "Token.hpp"

using namespace p1;
using namespace p1::smalls;

#define _MK_ENUM(name,repr)  #name,
const char * TokenKind::s_names[] =
{
  _DEF_TOKENS
};
#undef _MK_ENUM

#define _MK_ENUM(name,repr)  repr,
const char * TokenKind::s_reprs[] =
{
  _DEF_TOKENS
};
#undef _MK_ENUM

