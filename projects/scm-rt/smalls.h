#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <gc/gc.h>

typedef uintptr_t reg_t;

typedef struct
{
  reg_t car, cdr;
} pair_t;

typedef struct
{
  reg_t (*fp)(void);
  reg_t * env;
} closure_t;

typedef struct
{
  unsigned len; // without the terminating 0
  char s[]; // zero-terminated
} string_t;

typedef struct
{
  unsigned len;
  reg_t data[];
} vector_t;

__attribute__((noreturn)) static void error ( const char * msg )
{
  fprintf( stderr, "**Runtime error: %s\n", msg );
  abort();
}

static void * ALLOC ( size_t len )
{
  void * res;
  if ( (res = GC_MALLOC(len)) != NULL)
    return res;
  error( "Out of memory" );
  return NULL;
}

#define TP_PAIR 0
#define TP_SMALLINT 1
#define TP_CLOSURE 2
#define TP_DOUBLE 3
#define TP_STRING 4
#define TP_VECTOR 5
#define TP__MASK 7
#define TP__SHIFT 3

#define TP(v)  ((v) & TP__MASK)
#define NTP(v) ((v) & ~TP__MASK)

#define IS_PAIR(v)     (TP(v) == TP_PAIR)
#define IS_SMALLINT(v) (TP(v) == TP_SMALLINT)
#define IS_CLOSURE(v)  (TP(v) == TP_CLOSURE)
#define IS_DOUBLE(v)   (TP(v) == TP_DOUBLE)
#define IS_STRING(v)   (TP(v) == TP_STRING)
#define IS_VECTOR(v)   (TP(v) == TP_VECTOR)

#define MK_PAIR(x)      ((uintptr_t)x)
#define MK_SMALLINT(x)  (((uintptr_t)(x) << TP__SHIFT) | TP_SMALLINT)
#define MK_CLOSURE(x)   ((uintptr_t)(x) | TP_CLOSURE)
#define MK_DOUBLE(x)    ((uintptr_t)(x) | TP_DOUBLE)
#define MK_STRING(x)    ((uintptr_t)(x) | TP_STRING)
#define MK_VECTOR(x)    ((uintptr_t)(x) | TP_VECTOR)

#define X_PAIR(v)       ((pair_t *)(v))
#define X_SMALLINT(v)   ((uintptr_t)(v) >> TP__SHIFT)
#define X_CLOSURE(v)    ((closure_t *)NTP(v))
#define X_DOUBLE(v)     ((double *)NTP(v))
#define X_STRING(v)     ((string_t *)NTP(v))
#define X_VECTOR(v)     ((vector_t *)NTP(v))

#define CHK_PAIR(v)       (IS_PAIR(v) ? X_PAIR(v) : (pair_t*)(error("Not a pair"),0))
#define CHK_SMALLINT(v)   (IS_SMALLINT(v) ? X_SMALLINT(v) : (error("Not an integer"),0))
#define CHK_CLOSURE(v)    (IS_CLOSURE(v) ? X_CLOSURE(v) : (closure_t*)(error("Not a closure"),0))
#define CHK_DOUBLE(v)     (IS_DOUBLE(v) ? X_DOUBLE(v) : (double*)(error("Not a double"),0))
#define CHK_STRING(v)     (IS_STRING(v) ? X_STRING(v) : (string_t*)(error("Not a string"),0))
#define CHK_VECTOR(v)     (IS_VECTOR(v) ? X_VECTOR(v) : (vector_t*)(error("Not a vector"),0))

static pair_t * make_pair ( reg_t car, reg_t cdr )
{
  pair_t * res = (pair_t *)ALLOC( sizeof(pair_t) );
  res->car = car;
  res->cdr = cdr;
  return res;
}

#define GPARAM_COUNT 30
static int g_pcount;
static reg_t * g_penv;
static reg_t g_param[GPARAM_COUNT];
