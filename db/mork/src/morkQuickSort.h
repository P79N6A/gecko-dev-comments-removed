




































#ifndef _MORKQUICKSORT_
#define _MORKQUICKSORT_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif



extern void
morkQuickSort(mork_u1* ioVec, mork_u4 inCount, mork_u4 inSize,
  mdbAny_Order inOrder, void* ioClosure);
 


#endif 
