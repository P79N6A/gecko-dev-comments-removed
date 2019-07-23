




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCONFIG_
#include "morkConfig.h"
#endif



void mork_assertion_signal(const char* inMessage)
{
#if defined(MORK_WIN) || defined(MORK_MAC)
  
  NS_ERROR(inMessage);
#endif 
}

#if defined(MORK_OS2)
#include <fcntl.h>
#include <sys/stat.h>
#include <share.h>
#include <io.h>

FILE* mork_fileopen(const char* name, const char* mode)
{
    int access = O_RDWR;
    int descriptor;
    int pmode = 0;

    
    MORK_ASSERT((mode[0] == 'w' || mode[0] == 'r') && (mode[1] == 'b') && (mode[2] == '+'));
    if (mode[0] == 'w') {
        access |= (O_TRUNC | O_CREAT);
        pmode = S_IREAD | S_IWRITE;
    }

    descriptor = sopen(name, access, SH_DENYNO, pmode);
    if (descriptor != -1) {
        return fdopen(descriptor, mode);
    }
    return NULL;
}
#endif

#ifdef MORK_PROVIDE_STDLIB

MORK_LIB_IMPL(mork_i4)
mork_memcmp(const void* inOne, const void* inTwo, mork_size inSize)
{
  register const mork_u1* t = (const mork_u1*) inTwo;
  register const mork_u1* s = (const mork_u1*) inOne;
  const mork_u1* end = s + inSize;
  register mork_i4 delta;
  
  while ( s < end )
  {
    delta = ((mork_i4) *s) - ((mork_i4) *t);
    if ( delta )
      return delta;
    else
    {
      ++t;
      ++s;
    }
  }
  return 0;
}

MORK_LIB_IMPL(void)
mork_memcpy(void* outDst, const void* inSrc, mork_size inSize)
{
  register mork_u1* d = (mork_u1*) outDst;
  mork_u1* end = d + inSize;
  register const mork_u1* s = ((const mork_u1*) inSrc);
  
  while ( inSize >= 8 )
  {
    *d++ = *s++;
    *d++ = *s++;
    *d++ = *s++;
    *d++ = *s++;
    
    *d++ = *s++;
    *d++ = *s++;
    *d++ = *s++;
    *d++ = *s++;
    
    inSize -= 8;
  }
  
  while ( d < end )
    *d++ = *s++;
}

MORK_LIB_IMPL(void)
mork_memmove(void* outDst, const void* inSrc, mork_size inSize)
{
  register mork_u1* d = (mork_u1*) outDst;
  register const mork_u1* s = (const mork_u1*) inSrc;
  if ( d != s && inSize ) 
  {
    const mork_u1* srcEnd = s + inSize; 
    
    if ( d > s && d < srcEnd ) 
    {
      s = srcEnd; 
      d += inSize; 
      mork_u1* dstBegin = d; 
      while ( d - dstBegin >= 8 )
      {
        *--d = *--s;
        *--d = *--s;
        *--d = *--s;
        *--d = *--s;
        
        *--d = *--s;
        *--d = *--s;
        *--d = *--s;
        *--d = *--s;
      }
      while ( d > dstBegin )
        *--d = *--s;
    }
    else 
    {
      mork_u1* dstEnd = d + inSize;
      while ( dstEnd - d >= 8 )
      {
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
        *d++ = *s++;
      }
      while ( d < dstEnd )
        *d++ = *s++;
    }
  }
}

MORK_LIB_IMPL(void)
mork_memset(void* outDst, int inByte, mork_size inSize)
{
  register mork_u1* d = (mork_u1*) outDst;
  mork_u1* end = d + inSize;
  while ( d < end )
    *d++ = (mork_u1) inByte;
}

MORK_LIB_IMPL(void)
mork_strcpy(void* outDst, const void* inSrc)
{
  
  register mork_u1* d = ((mork_u1*) outDst) - 1;
  register const mork_u1* s = ((const mork_u1*) inSrc) - 1;
  while ( ( *++d = *++s ) != 0 )
    ;
}

MORK_LIB_IMPL(mork_i4)
mork_strcmp(const void* inOne, const void* inTwo)
{
  register const mork_u1* t = (const mork_u1*) inTwo;
  register const mork_u1* s = ((const mork_u1*) inOne);
  register mork_i4 a;
  register mork_i4 b;
  register mork_i4 delta;
  
  do
  {
    a = (mork_i4) *s++;
    b = (mork_i4) *t++;
    delta = a - b;
  }
  while ( !delta && a && b );
  
  return delta;
}

MORK_LIB_IMPL(mork_i4)
mork_strncmp(const void* inOne, const void* inTwo, mork_size inSize)
{
  register const mork_u1* t = (const mork_u1*) inTwo;
  register const mork_u1* s = (const mork_u1*) inOne;
  const mork_u1* end = s + inSize;
  register mork_i4 delta;
  register mork_i4 a;
  register mork_i4 b;
  
  while ( s < end )
  {
    a = (mork_i4) *s++;
    b = (mork_i4) *t++;
    delta = a - b;
    if ( delta || !a || !b )
      return delta;
  }
  return 0;
}

MORK_LIB_IMPL(mork_size)
mork_strlen(const void* inString)
{
  
  register const mork_u1* s = ((const mork_u1*) inString) - 1;
  while ( *++s ) 
    ;
  
  return s - ((const mork_u1*) inString); 
}

#endif 


