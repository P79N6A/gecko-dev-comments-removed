




































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

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif

#ifndef _ORKINFACTORY_
#include "orkinFactory.h"
#endif

#ifndef _ORKINENV_
#include "orkinEnv.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _ORKINROW_
#include "orkinRow.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif

#ifndef _MORKWRITER_
#include "morkWriter.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _ORKINSTORE_
#include "orkinStore.h"
#endif

#ifndef _ORKINTHUMB_
#include "orkinThumb.h"
#endif

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif

#ifndef _ORKINCOMPARE_
#include "orkinCompare.h"
#endif




orkinFactory:: ~orkinFactory() 
{
}


orkinFactory::orkinFactory(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkFactory* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kFactory)
{
  
}

extern "C" nsIMdbFactory* MakeMdbFactory() 
{
  return orkinFactory::MakeGlobalFactory();
}

 orkinFactory*
orkinFactory::MakeGlobalFactory()

{
  morkFactory* factory = new morkFactory(new orkinHeap());
  MORK_ASSERT(factory);
  if ( factory )
    return orkinFactory::MakeFactory(&factory->mFactory_Env, factory);
  else
    return (orkinFactory*) 0;
}

 orkinFactory*
orkinFactory::MakeFactory(morkEnv* ev,  morkFactory* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinFactory));
    if ( face )
    {
      orkinFactory* f = new(face) orkinFactory(ev, face, ioObject);
      if ( f )
        f->mNode_Refs += morkFactory_kWeakRefCountBonus;
      return f;
    }
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinFactory*) 0;
}

morkEnv*
orkinFactory::CanUseFactory(nsIMdbEnv* mev, mork_bool inMutable,
  mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkFactory* factory = (morkFactory*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kFactory,
         morkBool_kFalse);
    if ( factory )
    {
      if ( factory->IsFactory() )
        outEnv = ev;
      else
        factory->NonFactoryTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}

morkEnv* orkinFactory::GetInternalFactoryEnv(mdb_err* outErr)
{
  morkEnv* outEnv = 0;
  morkFactory* f = (morkFactory*) this->mHandle_Object;
  if ( f && f->IsNode() && f->IsOpenNode() && f->IsFactory() )
  {
    morkEnv* fenv = &f->mFactory_Env;
    if ( fenv && fenv->IsNode() && fenv->IsOpenNode() && fenv->IsEnv() )
    {
      fenv->ClearMorkErrorsAndWarnings(); 
      outEnv = fenv;
    }
    else
      *outErr = morkEnv_kBadFactoryEnvError;
  }
  else
    *outErr = morkEnv_kBadFactoryError;
    
  return outEnv;
}


NS_IMPL_QUERY_INTERFACE0(orkinFactory)

 nsrefcnt
orkinFactory::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinFactory::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}






 mdb_err
orkinFactory::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinFactory::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinFactory::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinFactory::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinFactory::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinFactory::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinFactory::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinFactory::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinFactory::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinFactory::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinFactory::OpenOldFile(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  const char* inFilePath,
  mork_bool inFrozen, nsIMdbFile** acqFile)
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbFile* outFile = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  morkFile* file = nsnull;
  if ( ev )
  {
    morkFactory* factory = (morkFactory*) this->mHandle_Object;
    if ( !ioHeap )
      ioHeap = &factory->mFactory_Heap;
      
    file = morkFile::OpenOldFile(ev, ioHeap, inFilePath, inFrozen);
    NS_IF_ADDREF( file );
      
    outErr = ev->AsErr();
  }
  if ( acqFile )
    *acqFile = file;
    
  return outErr;
}

 mdb_err
orkinFactory::CreateNewFile(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  const char* inFilePath, nsIMdbFile** acqFile)
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbFile* outFile = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  morkFile* file = nsnull;
  if ( ev )
  {
    morkFactory* factory = (morkFactory*) this->mHandle_Object;
    if ( !ioHeap )
      ioHeap = &factory->mFactory_Heap;
      
    file = morkFile::CreateNewFile(ev, ioHeap, inFilePath);
    if ( file )
      NS_ADDREF(file);
      
    outErr = ev->AsErr();
  }
  if ( acqFile )
    *acqFile = file;
    
  return outErr;
}



 mdb_err
orkinFactory::MakeEnv(nsIMdbHeap* ioHeap, nsIMdbEnv** acqEnv)

{
  mdb_err outErr = 0;
  nsIMdbEnv* outEnv = 0;
  mork_bool ownsHeap = (ioHeap == 0);
  if ( !ioHeap )
    ioHeap = new orkinHeap();

  if ( acqEnv && ioHeap )
  {
    morkEnv* fenv = this->GetInternalFactoryEnv(&outErr);
    if ( fenv )
    {
      morkFactory* factory = (morkFactory*) this->mHandle_Object;
      morkEnv* newEnv = new(*ioHeap, fenv)
        morkEnv(morkUsage::kHeap, ioHeap, factory, ioHeap);

      if ( newEnv )
      {
        newEnv->mEnv_OwnsHeap = ownsHeap;
        newEnv->mNode_Refs += morkEnv_kWeakRefCountEnvBonus;
        NS_ADDREF(newEnv);
        newEnv->mEnv_SelfAsMdbEnv = newEnv;
        outEnv = newEnv;
      }
      else
        outErr = morkEnv_kOutOfMemoryError;
    }
    
    *acqEnv = outEnv;
  }
  else
    outErr = morkEnv_kNilPointerError;
    
  return outErr;
}



 mdb_err
orkinFactory::MakeHeap(nsIMdbEnv* mev, nsIMdbHeap** acqHeap)
{
  mdb_err outErr = 0;
  nsIMdbHeap* outHeap = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    outHeap = new orkinHeap();
    if ( !outHeap )
      ev->OutOfMemoryError();
  }
  MORK_ASSERT(acqHeap);
  if ( acqHeap )
    *acqHeap = outHeap;
  return outErr;
}



 mdb_err
orkinFactory::MakeCompare(nsIMdbEnv* mev, nsIMdbCompare** acqCompare)
{
  mdb_err outErr = 0;
  nsIMdbCompare* outCompare = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    NS_ASSERTION(PR_FALSE, "not implemented");
    outErr = NS_ERROR_NOT_IMPLEMENTED;

    if ( !outCompare )
      ev->OutOfMemoryError();
  }
  if ( acqCompare )
    *acqCompare = outCompare;
  return outErr;
}



 mdb_err
orkinFactory::MakeRow(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  nsIMdbRow** acqRow)
{
  MORK_USED_1(ioHeap);
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
    
  return outErr;
}




 mdb_err
orkinFactory::CanOpenFilePort(
  nsIMdbEnv* mev, 
  
  
  nsIMdbFile* ioFile, 
  mdb_bool* outCanOpen, 
  mdbYarn* outFormatVersion)
{
  mdb_err outErr = 0;
  if ( outFormatVersion )
  {
    outFormatVersion->mYarn_Fill = 0;
  }
  mdb_bool canOpenAsPort = morkBool_kFalse;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioFile && outCanOpen )
    {
      canOpenAsPort = this->CanOpenMorkTextFile(ev, ioFile);
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
    
  if ( outCanOpen )
    *outCanOpen = canOpenAsPort;
    
  return outErr;
}
  
 mdb_err
orkinFactory::OpenFilePort(
  nsIMdbEnv* mev, 
  nsIMdbHeap* ioHeap, 
  
  nsIMdbFile* ioFile, 
  const mdbOpenPolicy* inOpenPolicy, 
  nsIMdbThumb** acqThumb)
{
  MORK_USED_1(ioHeap);
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioFile && inOpenPolicy && acqThumb )
    {
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}



 mdb_err
orkinFactory::ThumbToOpenPort( 
  nsIMdbEnv* mev, 
  nsIMdbThumb* ioThumb, 
  nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioThumb && acqPort )
    {
      morkThumb* thumb = (morkThumb*) ioThumb;
      morkStore* store = thumb->ThumbToOpenStore(ev);
      if ( store )
      {
        store->mStore_CanAutoAssignAtomIdentity = morkBool_kTrue;
        store->mStore_CanDirty = morkBool_kTrue;
        store->SetStoreAndAllSpacesCanDirty(ev, morkBool_kTrue);
        
        NS_ADDREF(store);
        outPort = store;
      }
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( acqPort )
    *acqPort = outPort;
  return outErr;
}


mork_bool
orkinFactory::CanOpenMorkTextFile(morkEnv* ev,
  
  nsIMdbFile* ioFile)
{
  MORK_USED_1(ev);
  mork_bool outBool = morkBool_kFalse;
  mork_size headSize = MORK_STRLEN(morkWriter_kFileHeader);
  
  char localBuf[ 256 + 4 ]; 
  mdbYarn localYarn;
  mdbYarn* y = &localYarn;
  y->mYarn_Buf = localBuf; 
  y->mYarn_Fill = 0;       
  y->mYarn_Size = 256;     
  y->mYarn_More = 0;
  y->mYarn_Form = 0;
  y->mYarn_Grow = 0;
  
  if ( ioFile )
  {
    nsIMdbEnv* menv = ev->AsMdbEnv();
    mdb_size actualSize = 0;
    ioFile->Get(menv, y->mYarn_Buf, y->mYarn_Size,  0, &actualSize);
    y->mYarn_Fill = actualSize;
    
    if ( y->mYarn_Buf && actualSize >= headSize && ev->Good() )
    {
      mork_u1* buf = (mork_u1*) y->mYarn_Buf;
      outBool = ( MORK_MEMCMP(morkWriter_kFileHeader, buf, headSize) == 0 );
    }
  }
  else
    ev->NilPointerError();

  return outBool;
}


 mdb_err
orkinFactory::CanOpenFileStore(
  nsIMdbEnv* mev, 
  
  
  nsIMdbFile* ioFile, 
  mdb_bool* outCanOpenAsStore, 
  mdb_bool* outCanOpenAsPort, 
  mdbYarn* outFormatVersion)
{
  mdb_bool canOpenAsStore = morkBool_kFalse;
  mdb_bool canOpenAsPort = morkBool_kFalse;
  if ( outFormatVersion )
  {
    outFormatVersion->mYarn_Fill = 0;
  }
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioFile && outCanOpenAsStore )
    {
      
      canOpenAsStore = this->CanOpenMorkTextFile(ev, ioFile);
      canOpenAsPort = canOpenAsStore;
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( outCanOpenAsStore )
    *outCanOpenAsStore = canOpenAsStore;
    
  if ( outCanOpenAsPort )
    *outCanOpenAsPort = canOpenAsPort;
    
  return outErr;
}
  
 mdb_err
orkinFactory::OpenFileStore( 
  nsIMdbEnv* mev, 
  nsIMdbHeap* ioHeap, 
  
  nsIMdbFile* ioFile, 
  const mdbOpenPolicy* inOpenPolicy, 
  nsIMdbThumb** acqThumb)
{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( !ioHeap ) 
      ioHeap = ev->mEnv_Heap;
    
    if ( ioFile && inOpenPolicy && acqThumb )
    {
      morkFactory* factory = (morkFactory*) this->mHandle_Object;
      morkStore* store = new(*ioHeap, ev)
        morkStore(ev, morkUsage::kHeap, ioHeap, factory, ioHeap);
        
      if ( store )
      {
        mork_bool frozen = morkBool_kFalse; 
        if ( store->OpenStoreFile(ev, frozen, ioFile, inOpenPolicy) )
        {
          morkThumb* thumb = morkThumb::Make_OpenFileStore(ev, ioHeap, store);
          if ( thumb )
          {
            outThumb = thumb;
            thumb->AddRef();
          }
        }

      }
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}


  
 mdb_err
orkinFactory::ThumbToOpenStore( 
  nsIMdbEnv* mev, 
  nsIMdbThumb* ioThumb, 
  nsIMdbStore** acqStore)
{
  mdb_err outErr = 0;
  nsIMdbStore* outStore = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioThumb && acqStore )
    {
      morkThumb* thumb = (morkThumb*) ioThumb;
      morkStore* store = thumb->ThumbToOpenStore(ev);
      if ( store )
      {
        store->mStore_CanAutoAssignAtomIdentity = morkBool_kTrue;
        store->mStore_CanDirty = morkBool_kTrue;
        store->SetStoreAndAllSpacesCanDirty(ev, morkBool_kTrue);
        
        outStore = store;
        NS_ADDREF(store);
      }
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( acqStore )
    *acqStore = outStore;
  return outErr;
}

 mdb_err
orkinFactory::CreateNewFileStore( 
  nsIMdbEnv* mev, 
  nsIMdbHeap* ioHeap, 
  
  nsIMdbFile* ioFile, 
  const mdbOpenPolicy* inOpenPolicy, 
  nsIMdbStore** acqStore)
{
  mdb_err outErr = 0;
  nsIMdbStore* outStore = 0;
  morkEnv* ev = this->CanUseFactory(mev,
     morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( !ioHeap ) 
      ioHeap = ev->mEnv_Heap;
    
    if ( ioFile && inOpenPolicy && acqStore && ioHeap )
    {
      morkFactory* factory = (morkFactory*) this->mHandle_Object;
      morkStore* store = new(*ioHeap, ev)
        morkStore(ev, morkUsage::kHeap, ioHeap, factory, ioHeap);
        
      if ( store )
      {
        store->mStore_CanAutoAssignAtomIdentity = morkBool_kTrue;
        store->mStore_CanDirty = morkBool_kTrue;
        store->SetStoreAndAllSpacesCanDirty(ev, morkBool_kTrue);

        if ( store->CreateStoreFile(ev, ioFile, inOpenPolicy) )
          outStore = store;
        NS_ADDREF(store);          
      }
    }
    else
      ev->NilPointerError();
    
    outErr = ev->AsErr();
  }
  if ( acqStore )
    *acqStore = outStore;
  return outErr;
}






