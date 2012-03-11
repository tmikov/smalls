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


#include "utf-8.hpp"
#include <sys/times.h>

using namespace std;

#define SAMPSIZE (1024*1024*16)
#define ITERS    8

class ErrorReporter : public IStreamErrorReporter
{
public:
  int errorCount;
  
  ErrorReporter () { errorCount = 0; };
  
  virtual void error ( off_t offset, const gc_char * message )
  {
    std::cerr << "**error at offset " << offset << ":" << message << std::endl;
    ++this->errorCount;
  }
};

static unsigned rnd ( unsigned l, unsigned h )
{
  return rand() % (h+1-l) + l;
}

/*
 * 
 */
int main ( int argc, char** argv )
{
  GC_INIT();
  
  gc_char * samp = new (GC) gc_char[SAMPSIZE+6];
  unsigned sampSize;
  for ( sampSize = 0; sampSize < SAMPSIZE; )
  {
    int32_t v;
    unsigned r = rand();
    if (r < (unsigned)(RAND_MAX * 0.60))
      v = rand() & 0x7F;
    else
    {
      unsigned l, h;
      if (r < (unsigned)(RAND_MAX * 0.85))
      {
        l = 0x80; h = 0x7FF;
      }
      else if (r < (unsigned)(RAND_MAX * 0.95))
      {
        l = 0x800; h = 0xFFFF;
      }
      else
      {
        l = 0x10000; h = 0x10FFFF;
      }
      do
        v = rnd(l, h);
      while (!isValidCodePoint(v));
    }
    sampSize += encodeUTF8( samp + sampSize, v );
  }
  
  struct tms t1, t2;
  unsigned long sum = 0;
  ErrorReporter errors;
  long clk_tck = sysconf( _SC_CLK_TCK );
  
  times( &t1 );

  for ( unsigned iter = 0; iter != ITERS; ++iter )
  {
    CharBufInput istr(samp,sampSize);
    UTF8StreamDecoder dec(istr,errors);
    
    int32_t ch;
    while ((ch = dec.get()) >= 0)
      sum += ch;
  }
  times( &t2 );
  
  long elaps = (t2.tms_utime + t2.tms_stime) - (t1.tms_utime + t1.tms_stime);
  printf( "Sum of all codepoints=%ld\n", sum);
  printf( "Elapsed time %.3f seconds, %.1f ns per char\n", 
          (double)elaps/clk_tck,
          ((double)elaps*(1e9/ITERS)/(clk_tck * (double)sampSize))
  );  
  
  return 0;
}

