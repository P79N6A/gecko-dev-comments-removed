




































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

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _ORKINSTORE_
#include "orkinStore.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKPORTTABLECURSOR_
#include "morkPortTableCursor.h"
#endif

#ifndef _ORKINPORTTABLECURSOR_
#include "orkinPortTableCursor.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif





#ifndef _ORKINTHUMB_
#include "orkinThumb.h"
#endif




orkinStore:: ~orkinStore() 
{
}


orkinStore::orkinStore(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkStore* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kStore)
{
  
}


 orkinStore*
orkinStore::MakeStore(morkEnv* ev, morkStore* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinStore));
    if ( face )
      return new(face) orkinStore(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinStore*) 0;
}

morkEnv*
orkinStore::CanUseStore(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkStore* self = (morkStore*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kStore,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsStore() )
        outEnv = ev;
      else
        self->NonStoreTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}



NS_IMPL_QUERY_INTERFACE0(orkinStore)

 nsrefcnt
orkinStore::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinStore::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinStore::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinStore::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinStore::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinStore::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinStore::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinStore::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinStore::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinStore::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinStore::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinStore::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinStore::GetIsPortReadonly(nsIMdbEnv* mev, mdb_bool* outBool)
{
  mdb_err outErr = 0;
  mdb_bool isReadOnly = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outBool )
    *outBool = isReadOnly;
  return outErr;
}

 mdb_err
orkinStore::GetIsStore(nsIMdbEnv* mev, mdb_bool* outBool)
{
  MORK_USED_1(mev);
 if ( outBool )
    *outBool = morkBool_kTrue;
  return 0;
}

 mdb_err
orkinStore::GetIsStoreAndDirty(nsIMdbEnv* mev, mdb_bool* outBool)
{
  mdb_err outErr = 0;
  mdb_bool isStoreAndDirty = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outBool )
    *outBool = isStoreAndDirty;
  return outErr;
}

 mdb_err
orkinStore::GetUsagePolicy(nsIMdbEnv* mev, 
  mdbUsagePolicy* ioUsagePolicy)
{
  MORK_USED_1(ioUsagePolicy);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinStore::SetUsagePolicy(nsIMdbEnv* mev, 
  const mdbUsagePolicy* inUsagePolicy)
{
  MORK_USED_1(inUsagePolicy);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinStore::IdleMemoryPurge( 
  nsIMdbEnv* mev, 
  mdb_size* outEstimatedBytesFreed) 
{
  mdb_err outErr = 0;
  mdb_size estimatedBytesFreed = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outEstimatedBytesFreed )
    *outEstimatedBytesFreed = estimatedBytesFreed;
  return outErr;
}

 mdb_err
orkinStore::SessionMemoryPurge( 
  nsIMdbEnv* mev, 
  mdb_size inDesiredBytesFreed, 
  mdb_size* outEstimatedBytesFreed) 
{
  MORK_USED_1(inDesiredBytesFreed);
  mdb_err outErr = 0;
  mdb_size estimate = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outEstimatedBytesFreed )
    *outEstimatedBytesFreed = estimate;
  return outErr;
}

 mdb_err
orkinStore::PanicMemoryPurge( 
  nsIMdbEnv* mev, 
  mdb_size* outEstimatedBytesFreed) 
{
  mdb_err outErr = 0;
  mdb_size estimate = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outEstimatedBytesFreed )
    *outEstimatedBytesFreed = estimate;
  return outErr;
}



 mdb_err
orkinStore::GetPortFilePath(
  nsIMdbEnv* mev, 
  mdbYarn* outFilePath, 
  mdbYarn* outFormatVersion) 
{
  mdb_err outErr = 0;
  if ( outFormatVersion )
    outFormatVersion->mYarn_Fill = 0;
  if ( outFilePath )
    outFilePath->mYarn_Fill = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    nsIMdbFile* file = store->mStore_File;
    
    if ( file )
      file->Path(mev, outFilePath);
    else
      store->NilStoreFileError(ev);
    
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinStore::GetPortFile(
  nsIMdbEnv* mev, 
  nsIMdbFile** acqFile) 
{
  mdb_err outErr = 0;
  if ( acqFile )
    *acqFile = 0;

  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    nsIMdbFile* file = store->mStore_File;
    
    if ( file )
    {
      if ( acqFile )
      {
        file->AddStrongRef(mev);
        if ( ev->Good() )
          *acqFile = file;
      }
    }
    else
      store->NilStoreFileError(ev);
      
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinStore::BestExportFormat( 
  nsIMdbEnv* mev, 
  mdbYarn* outFormatVersion) 
{
  mdb_err outErr = 0;
  if ( outFormatVersion )
    outFormatVersion->mYarn_Fill = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinStore::CanExportToFormat( 
  nsIMdbEnv* mev, 
  const char* inFormatVersion, 
  mdb_bool* outCanExport) 
{
  MORK_USED_1(inFormatVersion);
  mdb_bool canExport = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCanExport )
    *outCanExport = canExport;
  return outErr;
}

 mdb_err
orkinStore::ExportToFormat( 
  nsIMdbEnv* mev, 
  
  nsIMdbFile* ioFile, 
  const char* inFormatVersion, 
  nsIMdbThumb** acqThumb) 


{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioFile && inFormatVersion && acqThumb )
    {
      ev->StubMethodOnlyError();
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
orkinStore::TokenToString( 
  nsIMdbEnv* mev, 
  mdb_token inToken, 
  mdbYarn* outTokenName) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ((morkStore*) mHandle_Object)->TokenToString(ev, inToken, outTokenName);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinStore::StringToToken( 
  nsIMdbEnv* mev, 
  const char* inTokenName, 
  mdb_token* outToken) 
  
  
  
  
{
  mdb_err outErr = 0;
  mdb_token token = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    token = ((morkStore*) mHandle_Object)->StringToToken(ev, inTokenName);
    outErr = ev->AsErr();
  }
  if ( outToken )
    *outToken = token;
  return outErr;
}
  

 mdb_err
orkinStore::QueryToken( 
  nsIMdbEnv* mev, 
  const char* inTokenName, 
  mdb_token* outToken) 
  
  
  
{
  mdb_err outErr = 0;
  mdb_token token = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    token = ((morkStore*) mHandle_Object)->QueryToken(ev, inTokenName);
    outErr = ev->AsErr();
  }
  if ( outToken )
    *outToken = token;
  return outErr;
}





 mdb_err
orkinStore::HasRow( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_bool* outHasRow) 
{
  mdb_err outErr = 0;
  mdb_bool hasRow = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    morkRow* row = store->GetRow(ev, inOid);
    if ( row )
      hasRow = morkBool_kTrue;
      
    outErr = ev->AsErr();
  }
  if ( outHasRow )
    *outHasRow = hasRow;
  return outErr;
}
  
 mdb_err
orkinStore::GetRow( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    morkRow* row = store->GetRow(ev, inOid);
    if ( row && ev->Good() )
      outRow = row->AcquireRowHandle(ev, store);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

 mdb_err
orkinStore::GetRowRefCount( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_count* outRefCount) 
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    morkRow* row = store->GetRow(ev, inOid);
    if ( row && ev->Good() )
      count = row->mRow_GcUses;
      
    outErr = ev->AsErr();
  }
  if ( outRefCount )
    *outRefCount = count;
  return outErr;
}

 mdb_err
orkinStore::FindRow(nsIMdbEnv* mev, 
    mdb_scope inRowScope,   
    mdb_column inColumn,   
    const mdbYarn* inTargetCellValue, 
    mdbOid* outRowOid, 
    nsIMdbRow** acqRow) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  mdbOid rowOid;
  rowOid.mOid_Scope = 0;
  rowOid.mOid_Id = (mdb_id) -1;
  
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    morkRow* row = store->FindRow(ev, inRowScope, inColumn, inTargetCellValue);
    if ( row && ev->Good() )
    {
      outRow = row->AcquireRowHandle(ev, store);
      if ( outRow )
        rowOid = row->mRow_Oid;
    }
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
    
  return outErr;
}




 mdb_err
orkinStore::HasTable( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_bool* outHasTable) 
{
  mdb_err outErr = 0;
  mork_bool hasTable = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = ((morkStore*) mHandle_Object)->GetTable(ev, inOid);
    if ( table )
      hasTable = morkBool_kTrue;
    
    outErr = ev->AsErr();
  }
  if ( outHasTable )
    *outHasTable = hasTable;
  return outErr;
}
  
 mdb_err
orkinStore::GetTable( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  nsIMdbTable** acqTable) 
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table =
      ((morkStore*) mHandle_Object)->GetTable(ev, inOid);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}

 mdb_err
orkinStore::HasTableKind( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope, 
  mdb_kind inTableKind, 
  mdb_count* outTableCount, 
  mdb_bool* outSupportsTable) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    *outSupportsTable =
      ((morkStore*) mHandle_Object)->HasTableKind(ev, inRowScope,
        inTableKind, outTableCount);
    outErr = ev->AsErr();
  }
  return outErr;
}
      
 mdb_err
orkinStore::GetTableKind( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope,      
  mdb_kind inTableKind,      
  mdb_count* outTableCount, 
  mdb_bool* outMustBeUnique, 
  nsIMdbTable** acqTable)      
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table =
      ((morkStore*) mHandle_Object)->GetTableKind(ev, inRowScope,
        inTableKind, outTableCount, outMustBeUnique);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}
  
 mdb_err
orkinStore::GetPortTableCursor( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope, 
  mdb_kind inTableKind, 
  nsIMdbPortTableCursor** acqCursor) 
{
  mdb_err outErr = 0;
  nsIMdbPortTableCursor* outCursor = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkPortTableCursor* cursor =
      ((morkStore*) mHandle_Object)->GetPortTableCursor(ev, inRowScope,
        inTableKind);
    if ( cursor && ev->Good() )
      outCursor = cursor->AcquirePortTableCursorHandle(ev);

    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}



  
 mdb_err
orkinStore::ShouldCompress( 
  nsIMdbEnv* mev, 
  mdb_percent inPercentWaste, 
  mdb_percent* outActualWaste, 
  mdb_bool* outShould) 
{
  mdb_percent actualWaste = 0;
  mdb_bool shouldCompress = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    actualWaste = ((morkStore*) mHandle_Object)->PercentOfStoreWasted(ev);
    if ( inPercentWaste > 100 )
      inPercentWaste = 100;
    shouldCompress = ( actualWaste >= inPercentWaste );
    outErr = ev->AsErr();
  }
  if ( outActualWaste )
    *outActualWaste = actualWaste;
  if ( outShould )
    *outShould = shouldCompress;
  return outErr;
}







 mdb_err
orkinStore::NewTable( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope,    
  mdb_kind inTableKind,    
  mdb_bool inMustBeUnique, 
  const mdbOid* inOptionalMetaRowOid, 
  nsIMdbTable** acqTable)     
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table =
      ((morkStore*) mHandle_Object)->NewTable(ev, inRowScope,
        inTableKind, inMustBeUnique, inOptionalMetaRowOid);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}

 mdb_err
orkinStore::NewTableWithOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,   
  mdb_kind inTableKind,    
  mdb_bool inMustBeUnique, 
  const mdbOid* inOptionalMetaRowOid, 
  nsIMdbTable** acqTable)     
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = ((morkStore*) mHandle_Object)->OidToTable(ev, inOid,
      inOptionalMetaRowOid);
    if ( table && ev->Good() )
    {
      table->mTable_Kind = inTableKind;
      if ( inMustBeUnique )
        table->SetTableUnique();
      outTable = table->AcquireTableHandle(ev);
    }
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}



 mdb_err
orkinStore::RowScopeHasAssignedIds(nsIMdbEnv* mev,
  mdb_scope inRowScope,   
  mdb_bool* outCallerAssigned, 
  mdb_bool* outStoreAssigned) 
{
  MORK_USED_1(inRowScope);
  mdb_bool storeAssigned = morkBool_kFalse;
  mdb_bool callerAssigned = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCallerAssigned )
    *outCallerAssigned = callerAssigned;
  if ( outStoreAssigned )
    *outStoreAssigned = storeAssigned;
  return outErr;
}

 mdb_err
orkinStore::SetCallerAssignedIds(nsIMdbEnv* mev,
  mdb_scope inRowScope,   
  mdb_bool* outCallerAssigned, 
  mdb_bool* outStoreAssigned) 
{
  MORK_USED_1(inRowScope);
  mdb_bool storeAssigned = morkBool_kFalse;
  mdb_bool callerAssigned = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCallerAssigned )
    *outCallerAssigned = callerAssigned;
  if ( outStoreAssigned )
    *outStoreAssigned = storeAssigned;
  return outErr;
}

 mdb_err
orkinStore::SetStoreAssignedIds(nsIMdbEnv* mev,
  mdb_scope inRowScope,   
  mdb_bool* outCallerAssigned, 
  mdb_bool* outStoreAssigned) 
{
  MORK_USED_1(inRowScope);
  mdb_err outErr = 0;
  mdb_bool storeAssigned = morkBool_kFalse;
  mdb_bool callerAssigned = morkBool_kFalse;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCallerAssigned )
    *outCallerAssigned = callerAssigned;
  if ( outStoreAssigned )
    *outStoreAssigned = storeAssigned;
  return outErr;
}



 mdb_err
orkinStore::NewRowWithOid(nsIMdbEnv* mev, 
  const mdbOid* inOid,   
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    morkRow* row = store->NewRowWithOid(ev, inOid);
    if ( row && ev->Good() )
      outRow = row->AcquireRowHandle(ev, store);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

 mdb_err
orkinStore::NewRow(nsIMdbEnv* mev, 
  mdb_scope inRowScope,   
  nsIMdbRow** acqRow) 


{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    morkRow* row = store->NewRow(ev, inRowScope);
    if ( row && ev->Good() )
      outRow = row->AcquireRowHandle(ev, store);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}



 mdb_err
orkinStore::ImportContent( 
  nsIMdbEnv* mev, 
  mdb_scope inRowScope, 
  nsIMdbPort* ioPort, 
  nsIMdbThumb** acqThumb) 


{
  MORK_USED_2(inRowScope,ioPort);
  nsIMdbThumb* outThumb = 0;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}

 mdb_err
orkinStore::ImportFile( 
  nsIMdbEnv* mev, 
  nsIMdbFile* ioFile, 
  nsIMdbThumb** acqThumb) 


{
  nsIMdbThumb* outThumb = 0;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioFile && acqThumb )
    {
      ev->StubMethodOnlyError();
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
orkinStore::ShareAtomColumnsHint( 
  nsIMdbEnv* mev, 
  mdb_scope inScopeHint, 
  const mdbColumnSet* inColumnSet) 
{
  MORK_USED_2(inColumnSet,inScopeHint);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinStore::AvoidAtomColumnsHint( 
  nsIMdbEnv* mev, 
  const mdbColumnSet* inColumnSet) 
{
  MORK_USED_1(inColumnSet);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinStore::SmallCommit( 
  nsIMdbEnv* mev) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinStore::LargeCommit( 
  nsIMdbEnv* mev, 
  nsIMdbThumb** acqThumb) 




{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    nsIMdbHeap* heap = store->mPort_Heap;
    
    morkThumb* thumb = 0;
    
    if ( store->DoPreferLargeOverCompressCommit(ev) )
    {
      thumb = morkThumb::Make_LargeCommit(ev, heap, store);
    }
    else
    {
      mork_bool doCollect = morkBool_kFalse;
      thumb = morkThumb::Make_CompressCommit(ev, heap, store, doCollect);
    }
    
    if ( thumb )
    {
      outThumb = thumb;
      thumb->AddRef();
    }
      
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}

 mdb_err
orkinStore::SessionCommit( 
  nsIMdbEnv* mev, 
  nsIMdbThumb** acqThumb) 




{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    nsIMdbHeap* heap = store->mPort_Heap;
    
    morkThumb* thumb = 0;
    if ( store->DoPreferLargeOverCompressCommit(ev) )
    {
      thumb = morkThumb::Make_LargeCommit(ev, heap, store);
    }
    else
    {
      mork_bool doCollect = morkBool_kFalse;
      thumb = morkThumb::Make_CompressCommit(ev, heap, store, doCollect);
    }
    
    if ( thumb )
    {
      outThumb = thumb;
      thumb->AddRef();
    }
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}

 mdb_err
orkinStore::CompressCommit( 
  nsIMdbEnv* mev, 
  nsIMdbThumb** acqThumb) 




{
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseStore(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkStore* store = (morkStore*) mHandle_Object;
    nsIMdbHeap* heap = store->mPort_Heap;
    mork_bool doCollect = morkBool_kFalse;
    morkThumb* thumb = morkThumb::Make_CompressCommit(ev, heap, store, doCollect);
    if ( thumb )
    {
      outThumb = thumb;
      thumb->AddRef();
      store->mStore_CanWriteIncremental = morkBool_kTrue;
    }
      
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
  return outErr;
}







