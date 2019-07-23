




































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

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _ORKINTABLE_
#include "orkinTable.h"
#endif

#ifndef _ORKINROW_
#include "orkinRow.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

#ifndef _ORKINTABLEROWCURSOR_
#include "orkinTableRowCursor.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _ORKINSTORE_
#include "orkinStore.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif




orkinTable:: ~orkinTable() 
{
}


orkinTable::orkinTable(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkTable* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kTable)
{
  
}


 orkinTable*
orkinTable::MakeTable(morkEnv* ev, morkTable* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinTable));
    if ( face )
      return new(face) orkinTable(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinTable*) 0;
}

morkEnv*
orkinTable::CanUseTable(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkTable* self = (morkTable*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kTable,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsTable() )
        outEnv = ev;
      else
        self->NonTableTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}



NS_IMPL_QUERY_INTERFACE0(orkinTable)

 nsrefcnt
orkinTable::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinTable::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinTable::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinTable::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinTable::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinTable::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinTable::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinTable::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinTable::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinTable::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinTable::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinTable::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}







 mdb_err
orkinTable::GetSeed(nsIMdbEnv* mev,
  mdb_seed* outSeed)    
{
  mdb_err outErr = 0;
  mdb_seed seed = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    seed = table->mTable_RowArray.mArray_Seed;
    outErr = ev->AsErr();
  }
  if ( outSeed )
    *outSeed = seed;
  return outErr;
}
  
 mdb_err
orkinTable::GetCount(nsIMdbEnv* mev,
  mdb_count* outCount) 
{
  mdb_err outErr = 0;
  mdb_count count = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    count = table->mTable_RowArray.mArray_Fill;
    outErr = ev->AsErr();
  }
  if ( outCount )
    *outCount = count;
  return outErr;
}

 mdb_err
orkinTable::GetPort(nsIMdbEnv* mev,
  nsIMdbPort** acqPort) 
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkStore* store = table->mTable_Store;
    if ( store )
      outPort = store->AcquireStoreHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqPort )
    *acqPort = outPort;
  return outErr;
}



 mdb_err
orkinTable::GetCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inMemberPos, 
  nsIMdbCursor** acqCursor) 
{
  return this->GetTableRowCursor(mev, inMemberPos,
    (nsIMdbTableRowCursor**) acqCursor);
}



 mdb_err
orkinTable::GetOid(nsIMdbEnv* mev,
  mdbOid* outOid) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    table->GetTableOid(ev, outOid);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::BecomeContent(nsIMdbEnv* mev,
  const mdbOid* inOid) 
{
  MORK_USED_1(inOid);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    
    morkTable* table = (morkTable*) mHandle_Object;
    MORK_USED_1(table);

    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinTable::DropActivity( 
  nsIMdbEnv* mev)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table;
    table = (morkTable*) mHandle_Object;
    
    outErr = ev->AsErr();
  }
  return outErr;
}








 mdb_err
orkinTable::SetTablePriority(nsIMdbEnv* mev, mdb_priority inPrio)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    if ( inPrio > morkPriority_kMax )
      inPrio = morkPriority_kMax;
      
    table->mTable_Priority = inPrio;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::GetTablePriority(nsIMdbEnv* mev, mdb_priority* outPrio)
{
  mdb_err outErr = 0;
  mork_priority prio = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    prio = table->mTable_Priority;
    if ( prio > morkPriority_kMax )
    {
      prio = morkPriority_kMax;
      table->mTable_Priority = prio;
    }
    outErr = ev->AsErr();
  }
  if ( outPrio )
    *outPrio = prio;
  return outErr;
}


 mdb_err
orkinTable:: GetTableBeVerbose(nsIMdbEnv* mev, mdb_bool* outBeVerbose)
{
  mdb_err outErr = 0;
  mdb_bool beVerbose = morkBool_kFalse;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    beVerbose = table->IsTableVerbose();
    outErr = ev->AsErr();
  }
  if ( outBeVerbose )
    *outBeVerbose = beVerbose;
  return outErr;
}

 mdb_err
orkinTable::SetTableBeVerbose(nsIMdbEnv* mev, mdb_bool inBeVerbose)
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    if ( inBeVerbose )
      table->SetTableVerbose();
    else
       table->ClearTableVerbose();
   
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::GetTableIsUnique(nsIMdbEnv* mev, mdb_bool* outIsUnique)
{
  mdb_err outErr = 0;
  mdb_bool isUnique = morkBool_kFalse;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    isUnique = table->IsTableUnique();
    outErr = ev->AsErr();
  }
  if ( outIsUnique )
    *outIsUnique = isUnique;
  return outErr;
}

 mdb_err
orkinTable::GetTableKind(nsIMdbEnv* mev, mdb_kind* outTableKind)
{
  mdb_err outErr = 0;
  mdb_kind tableKind = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    tableKind = table->mTable_Kind;
    outErr = ev->AsErr();
  }
  if ( outTableKind )
    *outTableKind = tableKind;
  return outErr;
}

 mdb_err
orkinTable::GetRowScope(nsIMdbEnv* mev, mdb_scope* outRowScope)
{
  mdb_err outErr = 0;
  mdb_scope rowScope = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkRowSpace* space = table->mTable_RowSpace;
    if ( space )
      rowScope = space->SpaceScope();
    else
      table->NilRowSpaceError(ev);

    outErr = ev->AsErr();
  }
  if ( outRowScope )
    *outRowScope = rowScope;
  return outErr;
}

 mdb_err
orkinTable::GetMetaRow( nsIMdbEnv* mev,
  const mdbOid* inOptionalMetaRowOid, 
  mdbOid* outOid, 
  nsIMdbRow** acqRow) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkRow* row = table->GetMetaRow(ev, inOptionalMetaRowOid);
    if ( row && ev->Good() )
    {
      if ( outOid )
        *outOid = row->mRow_Oid;
        
      outRow = row->AcquireRowHandle(ev, table->mTable_Store);
    }
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
    
  if ( ev->Bad() && outOid )
  {
    outOid->mOid_Scope = 0;
    outOid->mOid_Id = morkRow_kMinusOneRid;
  }
  return outErr;
}




 mdb_err
orkinTable::GetTableRowCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  nsIMdbTableRowCursor** acqCursor) 
{
  mdb_err outErr = 0;
  nsIMdbTableRowCursor* outCursor = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTableRowCursor* cursor =
      ((morkTable*) mHandle_Object)->NewTableRowCursor(ev, inRowPos);
    if ( cursor )
    {
      if ( ev->Good() )
      {
        
        outCursor = cursor->AcquireTableRowCursorHandle(ev);
      }
      cursor->CutStrongRef(mev);
    }
      
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}



 mdb_err
orkinTable::PosToOid( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  mdbOid* outOid) 
{
  mdb_err outErr = 0;
  mdbOid roid;
  roid.mOid_Scope = 0;
  roid.mOid_Id = (mork_id) -1;
  
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkRow* row = table->SafeRowAt(ev, inRowPos);
    if ( row )
      roid = row->mRow_Oid;
    
    outErr = ev->AsErr();
  }
  if ( outOid )
    *outOid = roid;
  return outErr;
}

 mdb_err
orkinTable::OidToPos( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid, 
  mdb_pos* outPos) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    mork_pos pos = ((morkTable*) mHandle_Object)->ArrayHasOid(ev, inOid);
    if ( outPos )
      *outPos = pos;
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::PosToRow( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkStore* store = table->mTable_Store;
    morkRow* row = table->SafeRowAt(ev, inRowPos);
    if ( row && store )
      outRow = row->AcquireRowHandle(ev, store);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

 mdb_err
orkinTable::RowToPos( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow, 
  mdb_pos* outPos) 
{
  mdb_err outErr = 0;
  mork_pos pos = -1;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRow* row = (morkRow*) ioRow;
    morkTable* table = (morkTable*) mHandle_Object;
    pos = table->ArrayHasOid(ev, &row->mRow_Oid);
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
  return outErr;
}
  




 mdb_err
orkinTable::AddOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid) 
{
  MORK_USED_1(inOid);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::HasOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid, 
  mdb_bool* outHasOid) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( outHasOid )
      *outHasOid = ((morkTable*) mHandle_Object)->MapHasOid(ev, inOid);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::CutOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkStore* store = table->mTable_Store;
    if ( inOid && store )
    {
      morkRow* row = store->GetRow(ev, inOid);
      if ( row )
        table->CutRow(ev, row);
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinTable::NewRow( 
  nsIMdbEnv* mev, 
  mdbOid* ioOid, 
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkStore* store = table->mTable_Store;
    if ( ioOid && store )
    {
      morkRow* row = 0;
      if ( ioOid->mOid_Id == morkRow_kMinusOneRid )
        row = store->NewRow(ev, ioOid->mOid_Scope);
      else
        row = store->NewRowWithOid(ev, ioOid);
        
      if ( row && table->AddRow(ev, row) )
        outRow = row->AcquireRowHandle(ev, store);
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

 mdb_err
orkinTable::AddRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    ((morkTable*) mHandle_Object)->AddRow(ev, row);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::HasRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow, 
  mdb_bool* outBool) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    morkTable* table = (morkTable*) mHandle_Object;
    if ( outBool )
      *outBool = table->MapHasOid(ev, &row->mRow_Oid);
    outErr = ev->AsErr();
  }
  return outErr;
}


 mdb_err
orkinTable::CutRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    ((morkTable*) mHandle_Object)->CutRow(ev, row);
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::CutAllRows( 
  nsIMdbEnv* mev) 
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ((morkTable*) mHandle_Object)->CutAllRows(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}



 mdb_err
orkinTable::FindRowMatches( 
  nsIMdbEnv* mev, 
  const mdbYarn* inPrefix, 
  nsIMdbTableRowCursor** acqCursor) 
{
  MORK_USED_1(inPrefix);
  nsIMdbTableRowCursor* outCursor = 0;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}
  
 mdb_err
orkinTable::GetSearchColumns( 
  nsIMdbEnv* mev, 
  mdb_count* outCount, 
  mdbColumnSet* outColSet) 
  
  
  
  
  
  
  
  
  
  
  
  
{
  MORK_USED_1(outColSet);
  mdb_count count = 0;
  mdb_err outErr = 0;

  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( outCount )
    *outCount = count;
  return outErr;
}



 mdb_err
orkinTable::SearchColumnsHint( 
  nsIMdbEnv* mev, 
  const mdbColumnSet* inColumnSet) 
{
  MORK_USED_1(inColumnSet);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}
  
 mdb_err
orkinTable::SortColumnsHint( 
  nsIMdbEnv* mev, 
  const mdbColumnSet* inColumnSet) 
{
  MORK_USED_1(inColumnSet);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::StartBatchChangeHint( 
  nsIMdbEnv* mev, 
  const void* inLabel) 
  
  
  
{
  MORK_USED_1(inLabel);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::EndBatchChangeHint( 
  nsIMdbEnv* mev, 
  const void* inLabel) 
  
  
  
  
  
  
  
  
  
{
  MORK_USED_1(inLabel);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}






 mdb_err
orkinTable::CanSortColumn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdb_bool* outCanSort) 
{
  MORK_USED_1(inColumn);
  mdb_bool canSort = mdbBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  if ( outCanSort )
    *outCanSort = canSort;
  return outErr;
}

 mdb_err
orkinTable::GetSorting( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbSorting** acqSorting) 
{
  MORK_USED_1(inColumn);
  nsIMdbSorting* outSorting = 0;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    ev->StubMethodOnlyError();
    outErr = ev->AsErr();
  }
  if ( acqSorting )
    *acqSorting = outSorting;
  return outErr;
}

 mdb_err
orkinTable::SetSearchSorting( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbSorting* ioSorting) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  MORK_USED_1(inColumn);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioSorting )
    {
      ev->StubMethodOnlyError();
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }
  return outErr;
}






 mdb_err
orkinTable::MoveOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_pos inHintFromPos, 
  mdb_pos inToPos,       
  mdb_pos* outActualPos) 
{
  mdb_err outErr = 0;
  mdb_pos actualPos = -1; 
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = (morkTable*) mHandle_Object;
    morkStore* store = table->mTable_Store;
    if ( inOid && store )
    {
      morkRow* row = store->GetRow(ev, inOid);
      if ( row )
        actualPos = table->MoveRow(ev, row, inHintFromPos, inToPos);
    }
    else
      ev->NilPointerError();

    outErr = ev->AsErr();
  }
  if ( outActualPos )
    *outActualPos = actualPos;
  return outErr;
}

 mdb_err
orkinTable::MoveRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow,  
  mdb_pos inHintFromPos, 
  mdb_pos inToPos,       
  mdb_pos* outActualPos) 
{
  mdb_pos actualPos = -1; 
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    morkTable* table = (morkTable*) mHandle_Object;
    actualPos = table->MoveRow(ev, row, inHintFromPos, inToPos);
    outErr = ev->AsErr();
  }
  if ( outActualPos )
    *outActualPos = actualPos;
  return outErr;
}



 mdb_err
orkinTable::AddIndex( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbThumb** acqThumb) 


{
  MORK_USED_1(inColumn);
  nsIMdbThumb* outThumb = 0;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
     
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;

  return outErr;
}

 mdb_err
orkinTable::CutIndex( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbThumb** acqThumb) 


{
  MORK_USED_1(inColumn);
  mdb_err outErr = 0;
  nsIMdbThumb* outThumb = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    
    outErr = ev->AsErr();
  }
  if ( acqThumb )
    *acqThumb = outThumb;
    
  return outErr;
}

 mdb_err
orkinTable::HasIndex( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdb_bool* outHasIndex) 
{
  MORK_USED_1(inColumn);
  mdb_bool hasIndex = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
      
    outErr = ev->AsErr();
  }
  if ( outHasIndex )
    *outHasIndex = hasIndex;
  return outErr;
}

 mdb_err
orkinTable::EnableIndexOnSort( 
  nsIMdbEnv* mev, 
  mdb_column inColumn) 
{
  MORK_USED_1(inColumn);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinTable::QueryIndexOnSort( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdb_bool* outIndexOnSort) 
{
  MORK_USED_1(inColumn);
  mdb_bool indexOnSort = morkBool_kFalse;
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
      
    outErr = ev->AsErr();
  }
  if ( outIndexOnSort )
    *outIndexOnSort = indexOnSort;
  return outErr;
}

 mdb_err
orkinTable::DisableIndexOnSort( 
  nsIMdbEnv* mev, 
  mdb_column inColumn) 
{
  MORK_USED_1(inColumn);
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseTable(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    
    outErr = ev->AsErr();
  }
  return outErr;
}






