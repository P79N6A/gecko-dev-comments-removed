




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _ORKINFILE_
#include "orkinFile.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif




orkinFile:: ~orkinFile() 
{
}


orkinFile::orkinFile(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkFile* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kFile)
{
  
}


 orkinFile*
orkinFile::MakeFile(morkEnv* ev, morkFile* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinFile));
    if ( face )
      return new(face) orkinFile(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinFile*) 0;
}

morkEnv*
orkinFile::CanUseFile(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {  
    morkFile* self = (morkFile*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kFile,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsFile() )
        outEnv = ev;
      else
        self->NonFileTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}



NS_IMPL_QUERY_INTERFACE1(orkinFile, nsIMdbFile)

 nsrefcnt
orkinFile::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinFile::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinFile::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinFile::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinFile::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinFile::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinFile::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinFile::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinFile::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinFile::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinFile::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinFile::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinFile::Tell(nsIMdbEnv* mev, mdb_pos* outPos)
{
  mdb_err outErr = 0;
  mdb_pos pos = -1;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    pos = file->Tell(ev);
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
  return outErr;
}

 mdb_err
orkinFile::Seek(nsIMdbEnv* mev, mdb_pos inPos)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    file->Seek(ev, inPos);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinFile::Eof(nsIMdbEnv* mev, mdb_pos* outPos)
{
  mdb_err outErr = 0;
  mdb_pos pos = -1;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    pos = file->Length(ev);
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
  return outErr;
}




 mdb_err
orkinFile::Read(nsIMdbEnv* mev, void* outBuf, mdb_size inSize,
  mdb_size* outActualSize)
{
  mdb_err outErr = 0;
  mdb_size actualSize = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    actualSize = file->Read(ev, outBuf, inSize);
    outErr = ev->AsErr();
  }
  if ( outActualSize )
    *outActualSize = actualSize;
  return outErr;
}

 mdb_err
orkinFile::Get(nsIMdbEnv* mev, void* outBuf, mdb_size inSize,
  mdb_pos inPos, mdb_size* outActualSize)
{
  mdb_err outErr = 0;
  mdb_size actualSize = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    file->Seek(ev, inPos);
    if ( ev->Good() )
      actualSize = file->Read(ev, outBuf, inSize);
    outErr = ev->AsErr();
  }
  if ( outActualSize )
    *outActualSize = actualSize;
  return outErr;
}


  

 mdb_err
orkinFile::Write(nsIMdbEnv* mev, const void* inBuf, mdb_size inSize,
  mdb_size* outActualSize)
{
  mdb_err outErr = 0;
  mdb_size actualSize = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    actualSize = file->Write(ev, inBuf, inSize);
    outErr = ev->AsErr();
  }
  if ( outActualSize )
    *outActualSize = actualSize;
  return outErr;
}

 mdb_err
orkinFile::Put(nsIMdbEnv* mev, const void* inBuf, mdb_size inSize,
  mdb_pos inPos, mdb_size* outActualSize)
{
  mdb_err outErr = 0;
  mdb_size actualSize = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    file->Seek(ev, inPos);
    if ( ev->Good() )
      actualSize = file->Write(ev, inBuf, inSize);
    outErr = ev->AsErr();
  }
  if ( outActualSize )
    *outActualSize = actualSize;
  return outErr;
}

 mdb_err
orkinFile::Flush(nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    file->Flush(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}


  

 mdb_err
orkinFile::Path(nsIMdbEnv* mev, mdbYarn* outFilePath)
{
  mdb_err outErr = 0;
  if ( outFilePath )
    outFilePath->mYarn_Fill = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    ev->StringToYarn(file->GetFileNameString(), outFilePath);
    outErr = ev->AsErr();
  }
  return outErr;
}


  


 mdb_err
orkinFile::Steal(nsIMdbEnv* mev, nsIMdbFile* ioThief)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioThief )
    {
      
      
      
      morkFile* file = (morkFile*) mHandle_Object;
      file->Steal(ev, ioThief);
      outErr = ev->AsErr();
    }
    else
      ev->NilPointerError();
  }
  return outErr;
}

 mdb_err
orkinFile::Thief(nsIMdbEnv* mev, nsIMdbFile** acqThief)
{
  mdb_err outErr = 0;
  nsIMdbFile* outThief = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    outThief = file->GetThief();
    if ( outThief )
      outThief->AddStrongRef(ev->AsMdbEnv());
    outErr = ev->AsErr();
  }
  if ( acqThief )
    *acqThief = outThief;
  return outErr;
}





 mdb_err
orkinFile::BecomeTrunk(nsIMdbEnv* mev)







{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    file->BecomeTrunk(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinFile::AcquireBud(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  nsIMdbFile** acqBud) 














{
  mdb_err outErr = 0;
  nsIMdbFile* outBud = 0;
  morkEnv* ev = this->CanUseFile(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkFile* file = (morkFile*) mHandle_Object;
    morkFile* bud = file->AcquireBud(ev, ioHeap);
    if ( bud )
    {
      outBud = bud->AcquireFileHandle(ev);
      bud->CutStrongRef(mev);
    }
    outErr = ev->AsErr();
  }
  if ( acqBud )
    *acqBud = outBud;
  return outErr;
}






