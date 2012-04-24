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
#include "FastStdioInput.hpp"
#include "FastFileInput.hpp"
#include "FastMMapInput.hpp"
#include <sys/times.h>

using namespace std;

class ErrorReporter : public IStreamDecoderErrorReporter
{
public:
  int errorCount;

  ErrorReporter () { errorCount = 0; };

  virtual void error ( off_t offset, off_t outOffset, const gc_char * message )
  {
    std::cerr << "**error at input offsset " << offset << " and output offset " << outOffset << ":" << message << std::endl;
    ++this->errorCount;
  }
};

static unsigned rnd ( unsigned l, unsigned h )
{
  return rand() % (h+1-l) + l;
}

static void generate ( unsigned gensize )
{
  for ( unsigned sampSize = 0; sampSize < gensize; )
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
    char buf[8];
    unsigned len = encodeUTF8( buf, v );
    buf[len] = 0;
    sampSize += len;

    fprintf( stdout, "%s", buf );
  }
}

ErrorReporter g_errors;

class Timer
{
  struct tms t1, t2;
  long clk_tck;
public:
  Timer ()
  {
    clk_tck = sysconf( _SC_CLK_TCK );
    times( &t1 );
  }

  void stamp ()
  {
    times( &t2 );
  }

  void print ( unsigned len, unsigned iters = 1 )
  {
    long elaps = (t2.tms_utime + t2.tms_stime) - (t1.tms_utime + t1.tms_stime);
    printf( "Elapsed time %.3f seconds, %.1f ns per char\n",
            (double)elaps/clk_tck,
            ((double)elaps*(1e9/iters)/(clk_tck * (double)len))
    );
  }
};

static void memtest ( const char * fileName, unsigned iters )
{
  FILE * f;
  if (!(f = fopen( fileName, "rb" )))
  {
    perror( fileName );
    exit( EXIT_FAILURE );
  }
  if (fseek( f, 0, SEEK_END ))
  {
    perror( "fseek" );
    exit( EXIT_FAILURE );
  }
  long int len;
  if ( (len = ftell(f)) == -1)
  {
    perror( "ftell" );
    exit( EXIT_FAILURE );
  }
  rewind( f );

  gc_char * samp = new (GC) gc_char[len];
  if ((long)fread( samp, 1, len, f ) != len)
  {
    perror( "fread" );
    exit( EXIT_FAILURE );
  }
  fclose( f );

  Timer tim;
  long sum = 0;
  for ( unsigned iter = 0; iter != iters; ++iter )
  {
    CharBufInput istr(samp,len);
    UTF8StreamDecoder dec(istr,g_errors);

    int32_t ch;
    while ((ch = dec.get()) >= 0)
      sum += ch;
  }

  tim.stamp();
  tim.print( len, iters );
  printf( "Sum of all codepoints=%ld\n", sum);
}

static void streamtest ( FastCharInput & fi )
{
  Timer tim;
  long sum = 0;
  {
    UTF8StreamDecoder dec(fi,g_errors);

    int32_t ch;
    while ((ch = dec.get()) >= 0)
      sum += ch;
  }
  tim.stamp();
  tim.print( fi.offset() );
  printf( "Sum of all codepoints=%ld\n", sum);
}


static void stdiotest ( const char * fileName, unsigned bufSize )
{
  FastStdioInput fi(fileName,"rb",bufSize);
  streamtest( fi );
}

static void filetest ( const char * fileName, unsigned bufSize )
{
  FastFileInput fi(fileName,O_RDONLY,bufSize);
  streamtest( fi );
}

static void mmaptest ( const char * fileName )
{
  FastMMapInput fi(fileName);
  streamtest( fi );
}

static void errorExit ( const char * msg, ... )
{
  va_list ap;
  va_start( ap, msg );
  fprintf( stderr, "**Error:" );
  vfprintf( stderr, msg, ap );
  fprintf( stderr, "\n" );
  va_end( ap );
  std::exit( EXIT_FAILURE );
}

static unsigned cvtSize ( const char * str )
{
  char * endPtr;
  errno = 0;
  long val = strtol( str, &endPtr, 10 );
  if (errno != 0)
    errorExit( "Invalid size" );
  switch(*endPtr)
  {
  case 0: break;
  case 'K': val *= 1024; break;
  case 'M': val *= 1024*1024; break;
  default:
    errorExit( "Invalid size" );
    break;
  }
  if (val > numeric_limits<unsigned>::max())
    errorExit( "Invalid size" );
  return (unsigned)val;
}

/*
 *
 */
int main ( int argc, char** argv )
{
  GC_INIT();

  if (argc > 1)
  {
    if (std::strcmp( argv[1], "generate" ) == 0)
      generate( cvtSize(argv[2]) );
    else if (std::strcmp( argv[1], "mem") == 0)
      memtest( argv[2], atoi(argv[3]) );
    else if (std::strcmp( argv[1], "stdio") == 0)
      stdiotest( argv[2], cvtSize(argv[3]) );
    else if (std::strcmp( argv[1], "file") == 0)
      filetest( argv[2], cvtSize(argv[3]) );
    else if (std::strcmp( argv[1], "mmap") == 0)
      mmaptest( argv[2] );
    else
      errorExit( "Unknown command");
  }
  else
    errorExit("Must specify a command\n");

  return 0;
}

