#define sysfunc0_pcount 2
#define sysfunc0_plist 0
static reg_t sysfunc0 ( void )
{
  return (reg_t)((intptr_t)g_param1 + (intptr_t)g_param2);
}

#define sysfunc1_pcount 2
#define sysfunc1_plist 0
static reg_t sysfunc1 ( void )
{
  return (reg_t)((intptr_t)g_param1 - (intptr_t)g_param2);
}

#define sysfunc2_pcount 2
#define sysfunc2_plist 0
static reg_t sysfunc2 ( void )
{
  return (reg_t)((intptr_t)g_param1 * (intptr_t)g_param2);
}

#define sysfunc3_pcount 2
#define sysfunc3_plist 0
static reg_t sysfunc3 ( void )
{
  return (reg_t)((intptr_t)g_param1 / (intptr_t)g_param2);
}

#define sysfunc4_pcount 2
#define sysfunc4_plist 0
static reg_t sysfunc4 ( void )
{
  return (reg_t)((intptr_t)g_param1 < (intptr_t)g_param2);
}

#define sysfunc5_pcount 2
#define sysfunc5_plist 0
static reg_t sysfunc5 ( void )
{
  return (reg_t)((intptr_t)g_param1 > (intptr_t)g_param2);
}

#define sysfunc6_pcount 2
#define sysfunc6_plist 0
static reg_t sysfunc6 ( void )
{
  return (reg_t)((intptr_t)g_param1 == (intptr_t)g_param2);
}

#define sysfunc7_pcount 2
#define sysfunc7_plist 0
static reg_t sysfunc7 ( void )
{
  return (reg_t)((intptr_t)g_param1 != (intptr_t)g_param2);
}

#include <stdio.h>

#define sysfunc8_pcount 2
#define sysfunc8_plist 0
static reg_t sysfunc8 ( void )
{
  printf( "%ld\n",(long)g_param1 );
  return 0;
}

