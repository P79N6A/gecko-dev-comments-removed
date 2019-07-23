




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif

#ifndef _MORKFILE_
#include "morkFile.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif

#ifndef _MORKWRITER_
#include "morkWriter.h"
#endif





 void
morkFactory::CloseMorkNode(morkEnv* ev)  
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseFactory(ev);
    this->MarkShut();
  }
}


morkFactory::~morkFactory()  
{
  CloseFactory(&mFactory_Env);
  MORK_ASSERT(mFactory_Env.IsShutNode());
  MORK_ASSERT(this->IsShutNode());
}


morkFactory::morkFactory() 
: morkObject(morkUsage::kGlobal, (nsIMdbHeap*) 0, morkColor_kNone)
, mFactory_Env(morkUsage::kMember, (nsIMdbHeap*) 0, this,
  new orkinHeap())
, mFactory_Heap()
{
  if ( mFactory_Env.Good() )
  {
    mNode_Derived = morkDerived_kFactory;
    mNode_Refs += morkFactory_kWeakRefCountBonus;
  }
}


morkFactory::morkFactory(nsIMdbHeap* ioHeap)
: morkObject(morkUsage::kHeap, ioHeap, morkColor_kNone)
, mFactory_Env(morkUsage::kMember, (nsIMdbHeap*) 0, this, ioHeap)
, mFactory_Heap()
{
  if ( mFactory_Env.Good() )
  {
    mNode_Derived = morkDerived_kFactory;
    mNode_Refs += morkFactory_kWeakRefCountBonus;
  }
}


morkFactory::morkFactory(morkEnv* ev, 
  const morkUsage& inUsage, nsIMdbHeap* ioHeap)
: morkObject(ev, inUsage, ioHeap, morkColor_kNone, (morkHandle*) 0)
, mFactory_Env(morkUsage::kMember, (nsIMdbHeap*) 0, this, ioHeap)
, mFactory_Heap()
{
  if ( ev->Good() )
  {
    mNode_Derived = morkDerived_kFactory;
    mNode_Refs += morkFactory_kWeakRefCountBonus;
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkFactory, morkObject, nsIMdbFactory)

extern "C" nsIMdbFactory* MakeMdbFactory() 
{
  return new morkFactory(new orkinHeap());
}


 void
morkFactory::CloseFactory(morkEnv* ev)  
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mFactory_Env.CloseMorkNode(ev);
      this->CloseObject(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




morkEnv* morkFactory::GetInternalFactoryEnv(mdb_err* outErr)
{
  morkEnv* outEnv = 0;
  if (IsNode() && IsOpenNode() && IsFactory() )
  {
    morkEnv* fenv = &mFactory_Env;
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


void
morkFactory::NonFactoryTypeError(morkEnv* ev)
{
  ev->NewError("non morkFactory");
}


NS_IMETHODIMP
morkFactory::OpenOldFile(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  const char* inFilePath,
  mork_bool inFrozen, nsIMdbFile** acqFile)
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  morkFile* file = nsnull;
  if ( ev )
  {
    if ( !ioHeap )
      ioHeap = &mFactory_Heap;
      
    file = morkFile::OpenOldFile(ev, ioHeap, inFilePath, inFrozen);
    NS_IF_ADDREF( file );
      
    outErr = ev->AsErr();
  }
  if ( acqFile )
    *acqFile = file;
    
  return outErr;
}

NS_IMETHODIMP
morkFactory::CreateNewFile(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  const char* inFilePath, nsIMdbFile** acqFile)
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  morkFile* file = nsnull;
  if ( ev )
  {
    if ( !ioHeap )
      ioHeap = &mFactory_Heap;
      
    file = morkFile::CreateNewFile(ev, ioHeap, inFilePath);
    if ( file )
      NS_ADDREF(file);
      
    outErr = ev->AsErr();
  }
  if ( acqFile )
    *acqFile = file;
    
  return outErr;
}



NS_IMETHODIMP
morkFactory::MakeEnv(nsIMdbHeap* ioHeap, nsIMdbEnv** acqEnv)

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
      morkEnv* newEnv = new(*ioHeap, fenv)
        morkEnv(morkUsage::kHeap, ioHeap, this, ioHeap);

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



NS_IMETHODIMP
morkFactory::MakeHeap(nsIMdbEnv* mev, nsIMdbHeap** acqHeap)
{
  mdb_err outErr = 0;
  nsIMdbHeap* outHeap = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
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



NS_IMETHODIMP
morkFactory::MakeCompare(nsIMdbEnv* mev, nsIMdbCompare** acqCompare)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
morkFactory::MakeRow(nsIMdbEnv* mev, nsIMdbHeap* ioHeap,
  nsIMdbRow** acqRow)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP
morkFactory::CanOpenFilePort(
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
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
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
  
NS_IMETHODIMP
morkFactory::OpenFilePort(
  nsIMdbEnv* mev, 
  nsIMdbHeap* ioHeap, 
  
  nsIMdbFile* ioFile, 
  const mdbOpenPolicy* inOpenPolicy, 
  nsIMdbThumb** acqThumb)
{
  NS_ASSERTION(PR_FALSE, "this doesn't look implemented");
  MORK_USED_1(ioHeap);
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
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



NS_IMETHODIMP
morkFactory::ThumbToOpenPort( 
  nsIMdbEnv* mev, 
  nsIMdbThumb* ioThumb, 
  nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
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
morkFactory::CanOpenMorkTextFile(morkEnv* ev,
  
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


NS_IMETHODIMP
morkFactory::CanOpenFileStore(
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
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
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
  
NS_IMETHODIMP
morkFactory::OpenFileStore( 
  nsIMdbEnv* mev, 
  nsIMdbHeap* ioHeap, 
  
  nsIMdbFile* ioFile, 
  const mdbOpenPolicy* inOpenPolicy, 
  nsIMdbThumb** acqThumb)
{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( !ioHeap ) 
      ioHeap = ev->mEnv_Heap;
    
    if ( ioFile && inOpenPolicy && acqThumb )
    {
      morkStore* store = new(*ioHeap, ev)
        morkStore(ev, morkUsage::kHeap, ioHeap, this, ioHeap);
        
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


  
NS_IMETHODIMP
morkFactory::ThumbToOpenStore( 
  nsIMdbEnv* mev, 
  nsIMdbThumb* ioThumb, 
  nsIMdbStore** acqStore)
{
  mdb_err outErr = 0;
  nsIMdbStore* outStore = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
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

NS_IMETHODIMP
morkFactory::CreateNewFileStore( 
  nsIMdbEnv* mev, 
  nsIMdbHeap* ioHeap, 
  
  nsIMdbFile* ioFile, 
  const mdbOpenPolicy* inOpenPolicy, 
  nsIMdbStore** acqStore)
{
  mdb_err outErr = 0;
  nsIMdbStore* outStore = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( !ioHeap ) 
      ioHeap = ev->mEnv_Heap;
    
    if ( ioFile && inOpenPolicy && acqStore && ioHeap )
    {
      morkStore* store = new(*ioHeap, ev)
        morkStore(ev, morkUsage::kHeap, ioHeap, this, ioHeap);
        
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






