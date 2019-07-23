




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _ORKINERRORHOOK_
#include "orkinErrorHook.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




orkinHeap::orkinHeap() 
{
}


orkinHeap::~orkinHeap() 
{
}


 mdb_err
orkinHeap::Alloc(nsIMdbEnv* ev, 
  mdb_size inSize,   
  void** outBlock)  
{
  mdb_err outErr = 0;
  void* block = new char[ inSize ];
  if ( !block )
    outErr = morkEnv_kOutOfMemoryError;
    
  MORK_ASSERT(outBlock);
  if ( outBlock )
    *outBlock = block;
  return outErr;
}
  
 mdb_err
orkinHeap::Free(nsIMdbEnv* ev, 
  void* inBlock)
{
  MORK_ASSERT(inBlock);
  if ( inBlock )
    delete [] inBlock;
    
  return 0;
}

 mdb_err
orkinHeap::AddStrongRef(nsIMdbEnv* ev) 
{
  MORK_USED_1(ev);
  return 0;
}

 mdb_err
orkinHeap::CutStrongRef(nsIMdbEnv* ev) 
{
  MORK_USED_1(ev);
  return 0;
}




