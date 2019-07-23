




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




 void
morkBuf::NilBufBodyError(morkEnv* ev)
{
  ev->NewError("nil mBuf_Body");
}



 void
morkBlob::BlobFillOverSizeError(morkEnv* ev)
{
  ev->NewError("mBuf_Fill > mBlob_Size");
}



mork_bool
morkBlob::GrowBlob(morkEnv* ev, nsIMdbHeap* ioHeap, mork_size inNewSize)
{
  if ( ioHeap )
  {
    if ( !mBuf_Body ) 
      mBlob_Size = 0;
      
    if ( mBuf_Fill > mBlob_Size ) 
    {
      ev->NewWarning("mBuf_Fill > mBlob_Size");
      mBuf_Fill = mBlob_Size;
    }
      
    if ( inNewSize > mBlob_Size ) 
    {
      mork_u1* body = 0;
      ioHeap->Alloc(ev->AsMdbEnv(), inNewSize, (void**) &body);
      if ( body && ev->Good() )
      {
        void* oldBody = mBuf_Body;
        if ( mBlob_Size ) 
          MORK_MEMCPY(body, oldBody, mBlob_Size);
        
        mBlob_Size = inNewSize; 
        mBuf_Body = body; 
        
        if ( oldBody ) 
          ioHeap->Free(ev->AsMdbEnv(), oldBody);
      }
    }
  }
  else
    ev->NilPointerError();
    
  if ( ev->Good() && mBlob_Size < inNewSize )
    ev->NewError("mBlob_Size < inNewSize");
    
  return ev->Good();
}



morkCoil::morkCoil(morkEnv* ev, nsIMdbHeap* ioHeap)
{
  mBuf_Body = 0;
  mBuf_Fill = 0;
  mBlob_Size = 0;
  mText_Form = 0;
  mCoil_Heap = ioHeap;
  if ( !ioHeap )
    ev->NilPointerError();
}

void
morkCoil::CloseCoil(morkEnv* ev)
{
  void* body = mBuf_Body;
  nsIMdbHeap* heap = mCoil_Heap;

  mBuf_Body = 0;
  mCoil_Heap = 0;
  
  if ( body && heap )
  {
    heap->Free(ev->AsMdbEnv(), body);
  }
}


