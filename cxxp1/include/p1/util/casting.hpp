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
#ifndef P1_UTIL_CASTING_HPP
#define	P1_UTIL_CASTING_HPP

#include "llvm/Support/Casting.h"

namespace p1 {
  using llvm::isa;
  using llvm::cast;
  using llvm::cast_or_null;
  using llvm::dyn_cast;
}

#endif	/* P1_UTIL_CASTING_HPP */
