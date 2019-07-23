




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif






 void
morkTable::CloseMorkNode(morkEnv* ev)  
{
  if ( this->IsOpenNode() )
  {
    morkObject::CloseMorkNode(ev); 
    this->MarkClosing();
    this->CloseTable(ev);
    this->MarkShut();
  }
}


morkTable::~morkTable()  
{
  CloseMorkNode(mMorkEnv);
  MORK_ASSERT(this->IsShutNode());
  MORK_ASSERT(mTable_Store==0);
  MORK_ASSERT(mTable_RowSpace==0);
}


morkTable::morkTable(morkEnv* ev, 
  const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
  morkStore* ioStore, nsIMdbHeap* ioSlotHeap, morkRowSpace* ioRowSpace,
  const mdbOid* inOptionalMetaRowOid, 
  mork_tid inTid, mork_kind inKind, mork_bool inMustBeUnique)
: morkObject(ev, inUsage, ioHeap, (mork_color) inTid, (morkHandle*) 0)
, mTable_Store( 0 )
, mTable_RowSpace( 0 )
, mTable_MetaRow( 0 )

, mTable_RowMap( 0 )


, mTable_RowArray(ev, morkUsage::kMember, (nsIMdbHeap*) 0,
  morkTable_kStartRowArraySize, ioSlotHeap)
  
, mTable_ChangeList()
, mTable_ChangesCount( 0 )
, mTable_ChangesMax( 3 ) 

, mTable_Kind( inKind )

, mTable_Flags( 0 )
, mTable_Priority( morkPriority_kLo ) 
, mTable_GcUses( 0 )
, mTable_Pad( 0 )
{
  this->mLink_Next = 0;
  this->mLink_Prev = 0;
  
  if ( ev->Good() )
  {
    if ( ioStore && ioSlotHeap && ioRowSpace )
    {
      if ( inKind )
      {
        if ( inMustBeUnique )
          this->SetTableUnique();
        mTable_Store = ioStore;
        mTable_RowSpace = ioRowSpace;
        if ( inOptionalMetaRowOid )
          mTable_MetaRowOid = *inOptionalMetaRowOid;
        else
        {
          mTable_MetaRowOid.mOid_Scope = 0;
          mTable_MetaRowOid.mOid_Id = morkRow_kMinusOneRid;
        }
        if ( ev->Good() )
        {
          if ( this->MaybeDirtySpaceStoreAndTable() )
            this->SetTableRewrite(); 
            
          mNode_Derived = morkDerived_kTable;
        }
        this->MaybeDirtySpaceStoreAndTable(); 
      }
      else
        ioRowSpace->ZeroKindError(ev);
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkTable, morkObject, nsIMdbTable)

 void
morkTable::CloseTable(morkEnv* ev)  
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkRowMap::SlotStrongRowMap((morkRowMap*) 0, ev, &mTable_RowMap);
      
      mTable_RowArray.CloseMorkNode(ev);
      mTable_Store = 0;
      mTable_RowSpace = 0;
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}








NS_IMETHODIMP
morkTable::GetSeed(nsIMdbEnv* mev,
  mdb_seed* outSeed)    
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    *outSeed = mTable_RowArray.mArray_Seed;
    outErr = ev->AsErr();
  }
  return outErr;
}
  
NS_IMETHODIMP
morkTable::GetCount(nsIMdbEnv* mev,
  mdb_count* outCount) 
{
  NS_ENSURE_ARG_POINTER(outCount);
  *outCount = mTable_RowArray.mArray_Fill;
  return NS_OK;
}

NS_IMETHODIMP
morkTable::GetPort(nsIMdbEnv* mev,
  nsIMdbPort** acqPort) 
{
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  NS_ENSURE_ARG_POINTER(acqPort);    
  *acqPort = mTable_Store;
  return NS_OK;
}



NS_IMETHODIMP
morkTable::GetCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inMemberPos, 
  nsIMdbCursor** acqCursor) 
{
  return this->GetTableRowCursor(mev, inMemberPos,
    (nsIMdbTableRowCursor**) acqCursor);
}



NS_IMETHODIMP
morkTable::GetOid(nsIMdbEnv* mev,
  mdbOid* outOid) 
{
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  GetTableOid(ev, outOid);
  return NS_OK;
}

NS_IMETHODIMP
morkTable::BecomeContent(nsIMdbEnv* mev,
  const mdbOid* inOid) 
{
  NS_ASSERTION(PR_FALSE, "not implemented"); 
  return NS_ERROR_NOT_IMPLEMENTED;
  
}




NS_IMETHODIMP
morkTable::DropActivity( 
  nsIMdbEnv* mev)
{
  NS_ASSERTION(PR_FALSE, "not implemented"); 
  return NS_ERROR_NOT_IMPLEMENTED;
}









NS_IMETHODIMP
morkTable::SetTablePriority(nsIMdbEnv* mev, mdb_priority inPrio)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( inPrio > morkPriority_kMax )
      inPrio = morkPriority_kMax;
      
    mTable_Priority = inPrio;
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkTable::GetTablePriority(nsIMdbEnv* mev, mdb_priority* outPrio)
{
  mdb_err outErr = 0;
  mork_priority prio = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    prio = mTable_Priority;
    if ( prio > morkPriority_kMax )
    {
      prio = morkPriority_kMax;
      mTable_Priority = prio;
    }
    outErr = ev->AsErr();
  }
  if ( outPrio )
    *outPrio = prio;
  return outErr;
}


NS_IMETHODIMP
morkTable:: GetTableBeVerbose(nsIMdbEnv* mev, mdb_bool* outBeVerbose)
{
  NS_ENSURE_ARG_POINTER(outBeVerbose);
  *outBeVerbose = IsTableVerbose();
  return NS_OK;
}

NS_IMETHODIMP
morkTable::SetTableBeVerbose(nsIMdbEnv* mev, mdb_bool inBeVerbose)
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( inBeVerbose )
      SetTableVerbose();
    else
      ClearTableVerbose();
   
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkTable::GetTableIsUnique(nsIMdbEnv* mev, mdb_bool* outIsUnique)
{
  NS_ENSURE_ARG_POINTER(outIsUnique);
  *outIsUnique = IsTableUnique();
  return NS_OK;
}

NS_IMETHODIMP
morkTable::GetTableKind(nsIMdbEnv* mev, mdb_kind* outTableKind)
{
  NS_ENSURE_ARG_POINTER(outTableKind);
  *outTableKind = mTable_Kind;
  return NS_OK;
}

NS_IMETHODIMP
morkTable::GetRowScope(nsIMdbEnv* mev, mdb_scope* outRowScope)
{
  mdb_err outErr = 0;
  mdb_scope rowScope = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( mTable_RowSpace )
      rowScope = mTable_RowSpace->SpaceScope();
    else
      NilRowSpaceError(ev);

    outErr = ev->AsErr();
  }
  if ( outRowScope )
    *outRowScope = rowScope;
  return outErr;
}

NS_IMETHODIMP
morkTable::GetMetaRow( nsIMdbEnv* mev,
  const mdbOid* inOptionalMetaRowOid, 
  mdbOid* outOid, 
  nsIMdbRow** acqRow) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRow* row = GetMetaRow(ev, inOptionalMetaRowOid);
    if ( row && ev->Good() )
    {
      if ( outOid )
        *outOid = row->mRow_Oid;
        
      outRow = row->AcquireRowHandle(ev, mTable_Store);
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




NS_IMETHODIMP
morkTable::GetTableRowCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  nsIMdbTableRowCursor** acqCursor) 
{
  mdb_err outErr = 0;
  nsIMdbTableRowCursor* outCursor = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkTableRowCursor* cursor = NewTableRowCursor(ev, inRowPos);
    if ( cursor )
    {
      if ( ev->Good() )
      {
        
        outCursor = cursor;
        outCursor->AddRef();
      }
    }
      
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}



NS_IMETHODIMP
morkTable::PosToOid( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  mdbOid* outOid) 
{
  mdb_err outErr = 0;
  mdbOid roid;
  roid.mOid_Scope = 0;
  roid.mOid_Id = (mork_id) -1;
  
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRow* row = SafeRowAt(ev, inRowPos);
    if ( row )
      roid = row->mRow_Oid;
    
    outErr = ev->AsErr();
  }
  if ( outOid )
    *outOid = roid;
  return outErr;
}

NS_IMETHODIMP
morkTable::OidToPos( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid, 
  mdb_pos* outPos) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    mork_pos pos = ArrayHasOid(ev, inOid);
    if ( outPos )
      *outPos = pos;
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkTable::PosToRow( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRow* row = SafeRowAt(ev, inRowPos);
    if ( row && mTable_Store )
      outRow = row->AcquireRowHandle(ev, mTable_Store);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

NS_IMETHODIMP
morkTable::RowToPos( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow, 
  mdb_pos* outPos) 
{
  mdb_err outErr = 0;
  mork_pos pos = -1;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject* row = (morkRowObject*) ioRow;
    pos = ArrayHasOid(ev, &row->mRowObject_Row->mRow_Oid);
    outErr = ev->AsErr();
  }
  if ( outPos )
    *outPos = pos;
  return outErr;
}
  




NS_IMETHODIMP
morkTable::AddOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::HasOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid, 
  mdb_bool* outHasOid) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( outHasOid )
      *outHasOid = MapHasOid(ev, inOid);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkTable::CutOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( inOid && mTable_Store )
    {
      morkRow* row = mTable_Store->GetRow(ev, inOid);
      if ( row )
        CutRow(ev, row);
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkTable::NewRow( 
  nsIMdbEnv* mev, 
  mdbOid* ioOid, 
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( ioOid && mTable_Store )
    {
      morkRow* row = 0;
      if ( ioOid->mOid_Id == morkRow_kMinusOneRid )
        row = mTable_Store->NewRow(ev, ioOid->mOid_Scope);
      else
        row = mTable_Store->NewRowWithOid(ev, ioOid);
        
      if ( row && AddRow(ev, row) )
        outRow = row->AcquireRowHandle(ev, mTable_Store);
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}

NS_IMETHODIMP
morkTable::AddRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    AddRow(ev, row);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkTable::HasRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow, 
  mdb_bool* outBool) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    if ( outBool )
      *outBool = MapHasOid(ev, &row->mRow_Oid);
    outErr = ev->AsErr();
  }
  return outErr;
}


NS_IMETHODIMP
morkTable::CutRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    CutRow(ev, row);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkTable::CutAllRows( 
  nsIMdbEnv* mev) 
{
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    CutAllRows(ev);
    outErr = ev->AsErr();
  }
  return outErr;
}



NS_IMETHODIMP
morkTable::FindRowMatches( 
  nsIMdbEnv* mev, 
  const mdbYarn* inPrefix, 
  nsIMdbTableRowCursor** acqCursor) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}
  
NS_IMETHODIMP
morkTable::GetSearchColumns( 
  nsIMdbEnv* mev, 
  mdb_count* outCount, 
  mdbColumnSet* outColSet) 
  
  
  
  
  
  
  
  
  
  
  
  
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
morkTable::SearchColumnsHint( 
  nsIMdbEnv* mev, 
  const mdbColumnSet* inColumnSet) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}
  
NS_IMETHODIMP
morkTable::SortColumnsHint( 
  nsIMdbEnv* mev, 
  const mdbColumnSet* inColumnSet) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::StartBatchChangeHint( 
  nsIMdbEnv* mev, 
  const void* inLabel) 
  
  
  
{
  
  return NS_OK;
}

NS_IMETHODIMP
morkTable::EndBatchChangeHint( 
  nsIMdbEnv* mev, 
  const void* inLabel) 
  
  
  
  
  
  
  
  
  
{
  
  return NS_OK;
}






NS_IMETHODIMP
morkTable::CanSortColumn( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdb_bool* outCanSort) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::GetSorting( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbSorting** acqSorting) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::SetSearchSorting( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbSorting* ioSorting) 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
morkTable::MoveOid( 
  nsIMdbEnv* mev, 
  const mdbOid* inOid,  
  mdb_pos inHintFromPos, 
  mdb_pos inToPos,       
  mdb_pos* outActualPos) 
{
  mdb_err outErr = 0;
  mdb_pos actualPos = -1; 
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( inOid && mTable_Store )
    {
      morkRow* row = mTable_Store->GetRow(ev, inOid);
      if ( row )
        actualPos = MoveRow(ev, row, inHintFromPos, inToPos);
    }
    else
      ev->NilPointerError();

    outErr = ev->AsErr();
  }
  if ( outActualPos )
    *outActualPos = actualPos;
  return outErr;
}

NS_IMETHODIMP
morkTable::MoveRow( 
  nsIMdbEnv* mev, 
  nsIMdbRow* ioRow,  
  mdb_pos inHintFromPos, 
  mdb_pos inToPos,       
  mdb_pos* outActualPos) 
{
  mdb_pos actualPos = -1; 
  mdb_err outErr = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkRowObject *rowObj = (morkRowObject *) ioRow;
    morkRow* row = rowObj->mRowObject_Row;
    actualPos = MoveRow(ev, row, inHintFromPos, inToPos);
    outErr = ev->AsErr();
  }
  if ( outActualPos )
    *outActualPos = actualPos;
  return outErr;
}



NS_IMETHODIMP
morkTable::AddIndex( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbThumb** acqThumb) 


{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::CutIndex( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  nsIMdbThumb** acqThumb) 


{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::HasIndex( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdb_bool* outHasIndex) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::EnableIndexOnSort( 
  nsIMdbEnv* mev, 
  mdb_column inColumn) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::QueryIndexOnSort( 
  nsIMdbEnv* mev, 
  mdb_column inColumn, 
  mdb_bool* outIndexOnSort) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkTable::DisableIndexOnSort( 
  nsIMdbEnv* mev, 
  mdb_column inColumn) 
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}





mork_refs
morkTable::AddStrongRef(morkEnv *ev)
{
  return (mork_refs) AddRef();
}

mork_refs
morkTable::CutStrongRef(morkEnv *ev)
{
  return (mork_refs) Release();
}

mork_u2
morkTable::AddTableGcUse(morkEnv* ev)
{
  MORK_USED_1(ev); 
  if ( mTable_GcUses < morkTable_kMaxTableGcUses ) 
    ++mTable_GcUses;
    
  return mTable_GcUses;
}

mork_u2
morkTable::CutTableGcUse(morkEnv* ev)
{
  if ( mTable_GcUses ) 
  {
    if ( mTable_GcUses < morkTable_kMaxTableGcUses ) 
      --mTable_GcUses;
  }
  else
    this->TableGcUsesUnderflowWarning(ev);
    
  return mTable_GcUses;
}



void morkTable::SetTableClean(morkEnv* ev)
{
  if ( mTable_ChangeList.HasListMembers() )
  {
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    mTable_ChangeList.CutAndZapAllListMembers(ev, heap); 
  }
  mTable_ChangesCount = 0;
  
  mTable_Flags = 0;
  this->SetNodeClean();
}



void morkTable::NoteTableMoveRow(morkEnv* ev, morkRow* ioRow, mork_pos inPos)
{
  nsIMdbHeap* heap = mTable_Store->mPort_Heap;
  if ( this->IsTableRewrite() || this->HasChangeOverflow() )
    this->NoteTableSetAll(ev);
  else
  {
    morkTableChange* tableChange = new(*heap, ev)
      morkTableChange(ev, ioRow, inPos);
    if ( tableChange )
    {
      if ( ev->Good() )
      {
        mTable_ChangeList.PushTail(tableChange);
        ++mTable_ChangesCount;
      }
      else
      {
        tableChange->ZapOldNext(ev, heap);
        this->SetTableRewrite(); 
      }
    }
  }
}

void morkTable::note_row_move(morkEnv* ev, morkRow* ioRow, mork_pos inNewPos)
{
  if ( this->IsTableRewrite() || this->HasChangeOverflow() )
    this->NoteTableSetAll(ev);
  else
  {
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    morkTableChange* tableChange = new(*heap, ev)
      morkTableChange(ev, ioRow, inNewPos);
    if ( tableChange )
    {
      if ( ev->Good() )
      {
        mTable_ChangeList.PushTail(tableChange);
        ++mTable_ChangesCount;
      }
      else
      {
        tableChange->ZapOldNext(ev, heap);
        this->NoteTableSetAll(ev);
      }
    }
  }
}

void morkTable::note_row_change(morkEnv* ev, mork_change inChange,
  morkRow* ioRow)
{
  if ( this->IsTableRewrite() || this->HasChangeOverflow() )
    this->NoteTableSetAll(ev);
  else
  {
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    morkTableChange* tableChange = new(*heap, ev)
      morkTableChange(ev, inChange, ioRow);
    if ( tableChange )
    {
      if ( ev->Good() )
      {
        mTable_ChangeList.PushTail(tableChange);
        ++mTable_ChangesCount;
      }
      else
      {
        tableChange->ZapOldNext(ev, heap);
        this->NoteTableSetAll(ev);
      }
    }
  }
}

void morkTable::NoteTableSetAll(morkEnv* ev)
{
  if ( mTable_ChangeList.HasListMembers() )
  {
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    mTable_ChangeList.CutAndZapAllListMembers(ev, heap); 
  }
  mTable_ChangesCount = 0;
  this->SetTableRewrite();
}

 void
morkTable::TableGcUsesUnderflowWarning(morkEnv* ev)
{
  ev->NewWarning("mTable_GcUses underflow");
}

 void
morkTable::NonTableTypeError(morkEnv* ev)
{
  ev->NewError("non morkTable");
}

 void
morkTable::NonTableTypeWarning(morkEnv* ev)
{
  ev->NewWarning("non morkTable");
}

 void
morkTable::NilRowSpaceError(morkEnv* ev)
{
  ev->NewError("nil mTable_RowSpace");
}

mork_bool morkTable::MaybeDirtySpaceStoreAndTable()
{
  morkRowSpace* rowSpace = mTable_RowSpace;
  if ( rowSpace )
  {
    morkStore* store = rowSpace->mSpace_Store;
    if ( store && store->mStore_CanDirty )
    {
      store->SetStoreDirty();
      rowSpace->mSpace_CanDirty = morkBool_kTrue;
    }
    
    if ( rowSpace->mSpace_CanDirty ) 
    {
      if ( this->IsTableClean() )
      {
        mork_count rowCount = this->GetRowCount();
        mork_count oneThird = rowCount / 4; 
        if ( oneThird > 0x07FFF ) 
          oneThird = 0x07FFF;
          
        mTable_ChangesMax = (mork_u2) oneThird;
      }
      this->SetTableDirty();
      rowSpace->SetRowSpaceDirty();
      
      return morkBool_kTrue;
    }
  }
  return morkBool_kFalse;
}

morkRow*
morkTable::GetMetaRow(morkEnv* ev, const mdbOid* inOptionalMetaRowOid)
{
  morkRow* outRow = mTable_MetaRow;
  if ( !outRow )
  {
    morkStore* store = mTable_Store;
    mdbOid* oid = &mTable_MetaRowOid;
    if ( inOptionalMetaRowOid && !oid->mOid_Scope )
      *oid = *inOptionalMetaRowOid;
      
    if ( oid->mOid_Scope ) 
      outRow = store->OidToRow(ev, oid);
    else
    {
      outRow = store->NewRow(ev, morkStore_kMetaScope);
      if ( outRow ) 
        *oid = outRow->mRow_Oid;
    }
    mTable_MetaRow = outRow;
    if ( outRow ) 
    {
      outRow->AddRowGcUse(ev);

      this->SetTableNewMeta();
      if ( this->IsTableClean() ) 
        this->MaybeDirtySpaceStoreAndTable();
    }
  }
  
  return outRow;
}

void
morkTable::GetTableOid(morkEnv* ev, mdbOid* outOid)
{
  morkRowSpace* space = mTable_RowSpace;
  if ( space )
  {
    outOid->mOid_Scope = space->SpaceScope();
    outOid->mOid_Id = this->TableId();
  }
  else
    this->NilRowSpaceError(ev);
}

nsIMdbTable*
morkTable::AcquireTableHandle(morkEnv* ev)
{
  AddRef();
  return this;
}

mork_pos
morkTable::ArrayHasOid(morkEnv* ev, const mdbOid* inOid)
{
  MORK_USED_1(ev); 
  mork_count count = mTable_RowArray.mArray_Fill;
  mork_pos pos = -1;
  while ( ++pos < (mork_pos)count )
  {
    morkRow* row = (morkRow*) mTable_RowArray.At(pos);
    MORK_ASSERT(row);
    if ( row && row->EqualOid(inOid) )
    {
      return pos;
    }
  }
  return -1;
}

mork_bool
morkTable::MapHasOid(morkEnv* ev, const mdbOid* inOid)
{
  if ( mTable_RowMap )
    return ( mTable_RowMap->GetOid(ev, inOid) != 0 );
  else
    return ( ArrayHasOid(ev, inOid) >= 0 );
}

void morkTable::build_row_map(morkEnv* ev)
{
  morkRowMap* map = mTable_RowMap;
  if ( !map )
  {
    mork_count count = mTable_RowArray.mArray_Fill + 3;
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    map = new(*heap, ev) morkRowMap(ev, morkUsage::kHeap, heap, heap, count);
    if ( map )
    {
      if ( ev->Good() )
      {
        mTable_RowMap = map; 
        count = mTable_RowArray.mArray_Fill;
        mork_pos pos = -1;
        while ( ++pos < (mork_pos)count )
        {
          morkRow* row = (morkRow*) mTable_RowArray.At(pos);
          if ( row && row->IsRow() )
            map->AddRow(ev, row);
          else
            row->NonRowTypeError(ev);
        }
      }
      else
        map->CutStrongRef(ev);
    }
  }
}

morkRow* morkTable::find_member_row(morkEnv* ev, morkRow* ioRow)
{
  if ( mTable_RowMap )
    return mTable_RowMap->GetRow(ev, ioRow);
  else
  {
    mork_count count = mTable_RowArray.mArray_Fill;
    mork_pos pos = -1;
    while ( ++pos < (mork_pos)count )
    {
      morkRow* row = (morkRow*) mTable_RowArray.At(pos);
      if ( row == ioRow )
        return row;
    }
  }
  return (morkRow*) 0;
}

mork_pos
morkTable::MoveRow(morkEnv* ev, morkRow* ioRow, 
  mork_pos inHintFromPos, 
  mork_pos inToPos) 
  
  
{
  mork_pos outPos = -1; 
  mork_bool canDirty = ( this->IsTableClean() )?
    this->MaybeDirtySpaceStoreAndTable() : morkBool_kTrue;
  
  morkRow** rows = (morkRow**) mTable_RowArray.mArray_Slots;
  mork_count count = mTable_RowArray.mArray_Fill;
  if ( count && rows && ev->Good() ) 
  {
    mork_pos lastPos = count - 1; 
      
    if ( inToPos > lastPos ) 
      inToPos = lastPos; 
    else if ( inToPos < 0 ) 
      inToPos = 0; 
      
    if ( inHintFromPos > lastPos ) 
      inHintFromPos = lastPos; 
    else if ( inHintFromPos < 0 ) 
      inHintFromPos = 0; 

    morkRow** fromSlot = 0; 
    morkRow** rowsEnd = rows + count; 
    
    if ( inHintFromPos <= 0 ) 
    {
      morkRow** cursor = rows - 1; 
      while ( ++cursor < rowsEnd )
      {
        if ( *cursor == ioRow )
        {
          fromSlot = cursor;
          break; 
        }
      }
    }
    else 
    {
      morkRow** lo = rows + inHintFromPos; 
      morkRow** hi = lo; 
      
      
      
      
      
      while ( lo >= rows || hi < rowsEnd ) 
      {
        if ( lo >= rows ) 
        {
          if ( *lo == ioRow ) 
          {
            fromSlot = lo;
            break; 
          }
          --lo; 
        }
        if ( hi < rowsEnd ) 
        {
          if ( *hi == ioRow ) 
          {
            fromSlot = hi;
            break; 
          }
          ++hi; 
        }
      }
    }
    
    if ( fromSlot ) 
    {
      outPos = fromSlot - rows; 
      if ( outPos != inToPos ) 
      {
        morkRow** toSlot = rows + inToPos; 
        
        ++mTable_RowArray.mArray_Seed; 
        
        if ( fromSlot < toSlot ) 
        {
          morkRow** up = fromSlot; 
          while ( ++up <= toSlot ) 
          {
            *fromSlot = *up; 
            fromSlot = up; 
          }
        }
        else 
        {
          morkRow** down = fromSlot; 
          while ( --down >= toSlot ) 
          {
            *fromSlot = *down; 
            fromSlot = down; 
          }
        }
        *toSlot = ioRow;
        outPos = inToPos; 

        if ( canDirty )
          this->note_row_move(ev, ioRow, inToPos);
      }
    }
  }
  return outPos;
}

mork_bool
morkTable::AddRow(morkEnv* ev, morkRow* ioRow)
{
  morkRow* row = this->find_member_row(ev, ioRow);
  if ( !row && ev->Good() )
  {
    mork_bool canDirty = ( this->IsTableClean() )?
      this->MaybeDirtySpaceStoreAndTable() : morkBool_kTrue;
      
    mork_pos pos = mTable_RowArray.AppendSlot(ev, ioRow);
    if ( ev->Good() && pos >= 0 )
    {
      ioRow->AddRowGcUse(ev);
      if ( mTable_RowMap )
      {
        if ( mTable_RowMap->AddRow(ev, ioRow) )
        {
          
        }
        else
          mTable_RowArray.CutSlot(ev, pos);
      }
      else if ( mTable_RowArray.mArray_Fill >= morkTable_kMakeRowMapThreshold )
        this->build_row_map(ev);

      if ( canDirty && ev->Good() )
        this->NoteTableAddRow(ev, ioRow);
    }
  }
  return ev->Good();
}

mork_bool
morkTable::CutRow(morkEnv* ev, morkRow* ioRow)
{
  morkRow* row = this->find_member_row(ev, ioRow);
  if ( row )
  {
    mork_bool canDirty = ( this->IsTableClean() )?
      this->MaybeDirtySpaceStoreAndTable() : morkBool_kTrue;
      
    mork_count count = mTable_RowArray.mArray_Fill;
    morkRow** rowSlots = (morkRow**) mTable_RowArray.mArray_Slots;
    if ( rowSlots ) 
    {
      mork_pos pos = -1;
      morkRow** end = rowSlots + count;
      morkRow** slot = rowSlots - 1; 
      while ( ++slot < end ) 
      {
        if ( *slot == row ) 
        {
          pos = slot - rowSlots; 
          break; 
        }
      }
      if ( pos >= 0 ) 
        mTable_RowArray.CutSlot(ev, pos);
      else
        ev->NewWarning("row not found in array");
    }
    else
      mTable_RowArray.NilSlotsAddressError(ev);
      
    if ( mTable_RowMap )
      mTable_RowMap->CutRow(ev, ioRow);

    if ( canDirty )
      this->NoteTableCutRow(ev, ioRow);

    if ( ioRow->CutRowGcUse(ev) == 0 )
      ioRow->OnZeroRowGcUse(ev);
  }
  return ev->Good();
}


mork_bool
morkTable::CutAllRows(morkEnv* ev)
{
  if ( this->MaybeDirtySpaceStoreAndTable() )
  {
    this->SetTableRewrite(); 
    this->NoteTableSetAll(ev);
  }
    
  if ( ev->Good() )
  {
    mTable_RowArray.CutAllSlots(ev);
    if ( mTable_RowMap )
    {
      morkRowMapIter i(ev, mTable_RowMap);
      mork_change* c = 0;
      morkRow* r = 0;
      
      for ( c = i.FirstRow(ev, &r); c;  c = i.NextRow(ev, &r) )
      {
        if ( r )
        {
          if ( r->CutRowGcUse(ev) == 0 )
            r->OnZeroRowGcUse(ev);
            
          i.CutHereRow(ev, (morkRow**) 0);
        }
        else
          ev->NewWarning("nil row in table map");
      }
    }
  }
  return ev->Good();
}

morkTableRowCursor*
morkTable::NewTableRowCursor(morkEnv* ev, mork_pos inRowPos)
{
  morkTableRowCursor* outCursor = 0;
  if ( ev->Good() )
  {
    nsIMdbHeap* heap = mTable_Store->mPort_Heap;
    morkTableRowCursor* cursor = new(*heap, ev)
      morkTableRowCursor(ev, morkUsage::kHeap, heap, this, inRowPos);
    if ( cursor )
    {
      if ( ev->Good() )
        outCursor = cursor;
      else
        cursor->CutStrongRef((nsIMdbEnv *) ev);
    }
  }
  return outCursor;
}



morkTableChange::morkTableChange(morkEnv* ev, mork_change inChange,
  morkRow* ioRow)

: morkNext()
, mTableChange_Row( ioRow )
, mTableChange_Pos( morkTableChange_kNone )
{
  if ( ioRow )
  {
    if ( ioRow->IsRow() )
    {
      if ( inChange == morkChange_kAdd )
        mTableChange_Pos = morkTableChange_kAdd;
      else if ( inChange == morkChange_kCut )
        mTableChange_Pos = morkTableChange_kCut;
      else
        this->UnknownChangeError(ev);
    }
    else
      ioRow->NonRowTypeError(ev);
  }
  else
    ev->NilPointerError();
}

morkTableChange::morkTableChange(morkEnv* ev, morkRow* ioRow, mork_pos inPos)

: morkNext()
, mTableChange_Row( ioRow )
, mTableChange_Pos( inPos )
{
  if ( ioRow )
  {
    if ( ioRow->IsRow() )
    {
      if ( inPos < 0 )
        this->NegativeMovePosError(ev);
    }
    else
      ioRow->NonRowTypeError(ev);
  }
  else
    ev->NilPointerError();
}

void morkTableChange::UnknownChangeError(morkEnv* ev) const

{
  ev->NewError("mTableChange_Pos neither kAdd nor kCut");
}

void morkTableChange::NegativeMovePosError(morkEnv* ev) const

{
  ev->NewError("negative mTableChange_Pos for row move");
}




morkTableMap::~morkTableMap()
{
}

morkTableMap::morkTableMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
#ifdef MORK_BEAD_OVER_NODE_MAPS
  : morkBeadMap(ev, inUsage, ioHeap, ioSlotHeap)
#else 
  : morkNodeMap(ev, inUsage, ioHeap, ioSlotHeap)
#endif 
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kTableMap;
}



