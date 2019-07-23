




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKSINK_
#include "morkSink.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKBLOB_
#include "morkBlob.h"
#endif



 morkSink::~morkSink()
{
  mSink_At = 0;
  mSink_End = 0;
}

 void
morkSpool::FlushSink(morkEnv* ev) 
{
  morkCoil* coil = mSpool_Coil;
  if ( coil )
  {
    mork_u1* body = (mork_u1*) coil->mBuf_Body;
    if ( body )
    {
      mork_u1* at = mSink_At;
      mork_u1* end = mSink_End;
      if ( at >= body && at <= end ) 
      {
        mork_fill fill = (mork_fill) (at - body); 
        if ( fill <= coil->mBlob_Size )
          coil->mBuf_Fill = fill;
        else
        {
          coil->BlobFillOverSizeError(ev);
          coil->mBuf_Fill = coil->mBlob_Size; 
        }
      }
      else
        this->BadSpoolCursorOrderError(ev);
    }
    else
      coil->NilBufBodyError(ev);
  }
  else
    this->NilSpoolCoilError(ev);
}

 void
morkSpool::SpillPutc(morkEnv* ev, int c) 
{
  morkCoil* coil = mSpool_Coil;
  if ( coil )
  {
    mork_u1* body = (mork_u1*) coil->mBuf_Body;
    if ( body )
    {
      mork_u1* at = mSink_At;
      mork_u1* end = mSink_End;
      if ( at >= body && at <= end ) 
      {
        mork_size size = coil->mBlob_Size;
        mork_fill fill = (mork_fill) (at - body); 
        if ( fill <= size ) 
        {
          coil->mBuf_Fill = fill;
          if ( at >= end ) 
          {
            if ( size > 2048 ) 
              size += 512;
            else
            {
              mork_size growth = ( size * 4 ) / 3; 
              if ( growth < 64 ) 
                growth = 64;
              size += growth;
            }
            if ( coil->GrowCoil(ev, size) ) 
            {
              body = (mork_u1*) coil->mBuf_Body;
              if ( body ) 
              {
                mSink_At = at = body + fill;
                mSink_End = end = body + coil->mBlob_Size;
              }
              else
                coil->NilBufBodyError(ev);
            }
          }
          if ( ev->Good() ) 
          {
            if ( at < end ) 
            {
              *at++ = (mork_u1) c;
              mSink_At = at;
              coil->mBuf_Fill = fill + 1;
            }
            else
              this->BadSpoolCursorOrderError(ev);
          }
        }
        else 
        {
          coil->BlobFillOverSizeError(ev);
          coil->mBuf_Fill = coil->mBlob_Size; 
        }
      }
      else
        this->BadSpoolCursorOrderError(ev);
    }
    else
      coil->NilBufBodyError(ev);
  }
  else
    this->NilSpoolCoilError(ev);
}





morkSpool::~morkSpool()



{
  mSink_At = 0;
  mSink_End = 0;
  mSpool_Coil = 0;
}

morkSpool::morkSpool(morkEnv* ev, morkCoil* ioCoil)

: morkSink()
, mSpool_Coil( 0 )
{
  mSink_At = 0; 
  mSink_End = 0; 
  
  if ( ev->Good() )
  {
    if ( ioCoil )
    {
      mSpool_Coil = ioCoil;
      this->Seek(ev,  0);
    }
    else
      ev->NilPointerError();
  }
}



 void
morkSpool::BadSpoolCursorOrderError(morkEnv* ev)
{
  ev->NewError("bad morkSpool cursor order");
}

 void
morkSpool::NilSpoolCoilError(morkEnv* ev)
{
  ev->NewError("nil mSpool_Coil");
}

mork_bool
morkSpool::Seek(morkEnv* ev, mork_pos inPos)


{
  morkCoil* coil = mSpool_Coil;
  if ( coil )
  {
    mork_size minSize = (mork_size) (inPos + 64);
    
    if ( coil->mBlob_Size < minSize )
      coil->GrowCoil(ev, minSize);
      
    if ( ev->Good() )
    {
      coil->mBuf_Fill = (mork_fill) inPos;
      mork_u1* body = (mork_u1*) coil->mBuf_Body;
      if ( body )
      {
        mSink_At = body + inPos;
        mSink_End = body + coil->mBlob_Size;
      }
      else
        coil->NilBufBodyError(ev);
    }
  }
  else
    this->NilSpoolCoilError(ev);
    
  return ev->Good();
}

mork_bool
morkSpool::Write(morkEnv* ev, const void* inBuf, mork_size inSize)

{
  
  
 
  morkCoil* coil = mSpool_Coil;
  if ( coil )
  {
    mork_u1* body = (mork_u1*) coil->mBuf_Body;
    if ( body )
    {
      if ( inBuf && inSize ) 
      {
        mork_u1* at = mSink_At;
        mork_u1* end = mSink_End;
        if ( at >= body && at <= end ) 
        {
          
          mork_pos fill = at - body; 
          mork_num space = (mork_num) (end - at); 
          if ( space < inSize ) 
          {
            mork_size minGrowth = space + 16;
            mork_size minSize = coil->mBlob_Size + minGrowth;
            if ( coil->GrowCoil(ev, minSize) )
            {
              body = (mork_u1*) coil->mBuf_Body;
              if ( body )
              {
                mSink_At = at = body + fill;
                mSink_End = end = body + coil->mBlob_Size;
                space = (mork_num) (end - at); 
              }
              else
                coil->NilBufBodyError(ev);
            }
          }
          if ( ev->Good() )
          {
            if ( space >= inSize ) 
            {
              MORK_MEMCPY(at, inBuf, inSize); 
              mSink_At = at + inSize; 
              coil->mBuf_Fill = fill + inSize; 
            }
            else
              ev->NewError("insufficient morkSpool space");
          }
        }
        else
          this->BadSpoolCursorOrderError(ev);
      }
    }
    else
      coil->NilBufBodyError(ev);
  }
  else
    this->NilSpoolCoilError(ev);
  
  return ev->Good();
}

mork_bool
morkSpool::PutString(morkEnv* ev, const char* inString)


{
  if ( inString )
  {
    mork_size size = MORK_STRLEN(inString);
    this->Write(ev, inString, size);
  }
  return ev->Good();
}


