




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif

#ifndef _MORKCELLOBJECT_
#include "morkCellObject.h"
#endif

#ifndef _MORKCELL_
#include "morkCell.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKROWCELLCURSOR_
#include "morkRowCellCursor.h"
#endif






void morkRow::NoteRowAddCol(morkEnv* ev, mork_column inColumn)
{
  if ( !this->IsRowRewrite() )
  {
    mork_delta newDelta;
    morkDelta_Init(newDelta, inColumn, morkChange_kAdd);
    
    if ( newDelta != mRow_Delta ) 
    {
      if ( this->HasRowDelta() ) 
        this->SetRowRewrite(); 
      else
        this->SetRowDelta(inColumn, morkChange_kAdd);
    }
  }
  else
    this->ClearRowDelta();
}

void morkRow::NoteRowCutCol(morkEnv* ev, mork_column inColumn)
{
  if ( !this->IsRowRewrite() )
  {
    mork_delta newDelta;
    morkDelta_Init(newDelta, inColumn, morkChange_kCut);
    
    if ( newDelta != mRow_Delta ) 
    {
      if ( this->HasRowDelta() ) 
        this->SetRowRewrite(); 
      else
        this->SetRowDelta(inColumn, morkChange_kCut);
    }
  }
  else
    this->ClearRowDelta();
}

void morkRow::NoteRowSetCol(morkEnv* ev, mork_column inColumn)
{
  if ( !this->IsRowRewrite() )
  {
    if ( this->HasRowDelta() ) 
      this->SetRowRewrite(); 
    else
      this->SetRowDelta(inColumn, morkChange_kSet);
  }
  else
    this->ClearRowDelta();
}

void morkRow::NoteRowSetAll(morkEnv* ev)
{
  this->SetRowRewrite(); 
  this->ClearRowDelta();
}

mork_u2
morkRow::AddRowGcUse(morkEnv* ev)
{
  if ( this->IsRow() )
  {
    if ( mRow_GcUses < morkRow_kMaxGcUses ) 
      ++mRow_GcUses;
  }
  else
    this->NonRowTypeError(ev);
    
  return mRow_GcUses;
}

mork_u2
morkRow::CutRowGcUse(morkEnv* ev)
{
  if ( this->IsRow() )
  {
    if ( mRow_GcUses ) 
    {
      if ( mRow_GcUses < morkRow_kMaxGcUses ) 
        --mRow_GcUses;
    }
    else
      this->GcUsesUnderflowWarning(ev);
  }
  else
    this->NonRowTypeError(ev);
    
  return mRow_GcUses;
}

 void
morkRow::GcUsesUnderflowWarning(morkEnv* ev)
{
  ev->NewWarning("mRow_GcUses underflow");
}


 void
morkRow::NonRowTypeError(morkEnv* ev)
{
  ev->NewError("non morkRow");
}

 void
morkRow::NonRowTypeWarning(morkEnv* ev)
{
  ev->NewWarning("non morkRow");
}

 void
morkRow::LengthBeyondMaxError(morkEnv* ev)
{
  ev->NewError("mRow_Length over max");
}

 void
morkRow::ZeroColumnError(morkEnv* ev)
{
  ev->NewError(" zero mork_column");
}

 void
morkRow::NilCellsError(morkEnv* ev)
{
  ev->NewError("nil mRow_Cells");
}

void
morkRow::InitRow(morkEnv* ev, const mdbOid* inOid, morkRowSpace* ioSpace,
  mork_size inLength, morkPool* ioPool)
  
{
  if ( ioSpace && ioPool && inOid )
  {
    if ( inLength <= morkRow_kMaxLength )
    {
      if ( inOid->mOid_Id != morkRow_kMinusOneRid )
      {
        mRow_Space = ioSpace;
        mRow_Object = 0;
        mRow_Cells = 0;
        mRow_Oid = *inOid;

        mRow_Length = (mork_u2) inLength;
        mRow_Seed = (mork_u2) (mork_ip) this; 

        mRow_GcUses = 0;
        mRow_Pad = 0;
        mRow_Flags = 0;
        mRow_Tag = morkRow_kTag;
        
        morkZone* zone = &ioSpace->mSpace_Store->mStore_Zone;

        if ( inLength )
          mRow_Cells = ioPool->NewCells(ev, inLength, zone);

        if ( this->MaybeDirtySpaceStoreAndRow() ) 
        {
          this->SetRowRewrite();
          this->NoteRowSetAll(ev);
        }
      }
      else
        ioSpace->MinusOneRidError(ev);
    }
    else
      this->LengthBeyondMaxError(ev);
  }
  else
    ev->NilPointerError();
}

morkRowObject*
morkRow::AcquireRowObject(morkEnv* ev, morkStore* ioStore)
{
  morkRowObject* ro = mRow_Object;
  if ( ro ) 
    ro->AddRef();
  else
  {
    nsIMdbHeap* heap = ioStore->mPort_Heap;
    ro = new(*heap, ev)
      morkRowObject(ev, morkUsage::kHeap, heap, this, ioStore);

    morkRowObject::SlotWeakRowObject(ro, ev, &mRow_Object);
    ro->AddRef();
  }
  return ro;
}

nsIMdbRow*
morkRow::AcquireRowHandle(morkEnv* ev, morkStore* ioStore)
{
  return AcquireRowObject(ev, ioStore);
}

nsIMdbCell*
morkRow::AcquireCellHandle(morkEnv* ev, morkCell* ioCell,
  mdb_column inCol, mork_pos inPos)
{
  nsIMdbHeap* heap = ev->mEnv_Heap;
  morkCellObject* cellObj = new(*heap, ev)
    morkCellObject(ev, morkUsage::kHeap, heap, this, ioCell, inCol, inPos);
  if ( cellObj )
  {
    nsIMdbCell* cellHandle = cellObj->AcquireCellHandle(ev);

    return cellHandle;
  }
  return (nsIMdbCell*) 0;
}

mork_count
morkRow::CountOverlap(morkEnv* ev, morkCell* ioVector, mork_fill inFill)
  
  
  
  
  
  
  
  
  
  
{
  mork_count outCount = 0;
  mork_pos pos = 0; 
  morkCell* cells = ioVector;
  morkCell* end = cells + inFill;
  --cells; 
  while ( ++cells < end && ev->Good() )
  {
    mork_column col = cells->GetColumn();
    
    morkCell* old = this->GetCell(ev, col, &pos);
    if ( old ) 
    {
      mork_change newChg = cells->GetChange();
      mork_change oldChg = old->GetChange();
      if ( newChg != morkChange_kCut || oldChg != newChg ) 
      {
        if ( cells->mCell_Atom != old->mCell_Atom ) 
          ++outCount; 
      }
      else
        cells->SetColumnAndChange(col, morkChange_kDup); 
    }
  }
  return outCount;
}

void
morkRow::MergeCells(morkEnv* ev, morkCell* ioVector,
  mork_fill inVecLength, mork_fill inOldRowFill, mork_fill inOverlap)
  
  
  
{
  morkCell* newCells = mRow_Cells + inOldRowFill; 
  morkCell* newEnd = newCells + mRow_Length; 

  morkCell* srcCells = ioVector;
  morkCell* srcEnd = srcCells + inVecLength;
  
  --srcCells; 
  while ( ++srcCells < srcEnd && ev->Good() )
  {
    mork_change srcChg = srcCells->GetChange();
    if ( srcChg != morkChange_kDup ) 
    {
      morkCell* dstCell = 0;
      if ( inOverlap )
      {
        mork_pos pos = 0; 
        dstCell = this->GetCell(ev, srcCells->GetColumn(), &pos);
      }
      if ( dstCell )
      {
        --inOverlap; 
        
        morkAtom* dstAtom = dstCell->mCell_Atom;
        *dstCell = *srcCells; 
        srcCells->mCell_Atom = dstAtom; 
      }
      else if ( newCells < newEnd ) 
      {
        dstCell = newCells++; 
        
        *dstCell = *srcCells; 
        srcCells->mCell_Atom = 0; 
      }
      else 
        ev->NewError("out of new cells");
    }
  }
}

void
morkRow::TakeCells(morkEnv* ev, morkCell* ioVector, mork_fill inVecLength,
  morkStore* ioStore)
{
  if ( ioVector && inVecLength && ev->Good() )
  {
    ++mRow_Seed; 
    mork_size length = (mork_size) mRow_Length;
    
    mork_count overlap = this->CountOverlap(ev, ioVector, inVecLength);

    mork_size growth = inVecLength - overlap; 
    mork_size newLength = length + growth;
    
    if ( growth && ev->Good() ) 
    {
      morkZone* zone = &ioStore->mStore_Zone;
      morkPool* pool = ioStore->StorePool();
      if ( !pool->AddRowCells(ev, this, length + growth, zone) )
        ev->NewError("cannot take cells");
    }
    if ( ev->Good() )
    {
      if ( mRow_Length >= newLength )
        this->MergeCells(ev, ioVector, inVecLength, length, overlap);
      else
        ev->NewError("not enough new cells");
    }
  }
}

mork_bool morkRow::MaybeDirtySpaceStoreAndRow()
{
  morkRowSpace* rowSpace = mRow_Space;
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
      this->SetRowDirty();
      rowSpace->SetRowSpaceDirty();
      return morkBool_kTrue;
    }
  }
  return morkBool_kFalse;
}

morkCell*
morkRow::NewCell(morkEnv* ev, mdb_column inColumn,
  mork_pos* outPos, morkStore* ioStore)
{
  ++mRow_Seed; 
  mork_size length = (mork_size) mRow_Length;
  *outPos = (mork_pos) length;
  morkPool* pool = ioStore->StorePool();
  morkZone* zone = &ioStore->mStore_Zone;
  
  mork_bool canDirty = this->MaybeDirtySpaceStoreAndRow();
  
  if ( pool->AddRowCells(ev, this, length + 1, zone) )
  {
    morkCell* cell = mRow_Cells + length;
    
    if ( canDirty )
      cell->SetCellColumnDirty(inColumn);
    else
      cell->SetCellColumnClean(inColumn);
      
    if ( canDirty && !this->IsRowRewrite() )
      this->NoteRowAddCol(ev, inColumn);
      
    return cell;
  }
    
  return (morkCell*) 0;
}



void morkRow::SeekColumn(morkEnv* ev, mdb_pos inPos, 
  mdb_column* outColumn, mdbYarn* outYarn)
{
  morkCell* cells = mRow_Cells;
  if ( cells && inPos < mRow_Length && inPos >= 0 )
  {
    morkCell* c = cells + inPos;
    if ( outColumn )
    	*outColumn = c->GetColumn();
    if ( outYarn )
    	c->mCell_Atom->GetYarn(outYarn); 
  }
  else
  {
    if ( outColumn )
    	*outColumn = 0;
    if ( outYarn )
    	((morkAtom*) 0)->GetYarn(outYarn); 
  }
}

void
morkRow::NextColumn(morkEnv* ev, mdb_column* ioColumn, mdbYarn* outYarn)
{
  morkCell* cells = mRow_Cells;
  if ( cells )
  {
  	mork_column last = 0;
  	mork_column inCol = *ioColumn;
    morkCell* end = cells + mRow_Length;
    while ( cells < end )
    {
      if ( inCol == last ) 
      {
		    if ( outYarn )
		    	cells->mCell_Atom->GetYarn(outYarn); 
        *ioColumn = cells->GetColumn();
        return;  
      }
      else
      {
        last = cells->GetColumn();
        ++cells;
      }
    }
  }
	*ioColumn = 0;
  if ( outYarn )
  	((morkAtom*) 0)->GetYarn(outYarn); 
}

morkCell*
morkRow::CellAt(morkEnv* ev, mork_pos inPos) const
{
  MORK_USED_1(ev);
  morkCell* cells = mRow_Cells;
  if ( cells && inPos < mRow_Length && inPos >= 0 )
  {
    return cells + inPos;
  }
  return (morkCell*) 0;
}

morkCell*
morkRow::GetCell(morkEnv* ev, mdb_column inColumn, mork_pos* outPos) const
{
  MORK_USED_1(ev);
  morkCell* cells = mRow_Cells;
  if ( cells )
  {
    morkCell* end = cells + mRow_Length;
    while ( cells < end )
    {
      mork_column col = cells->GetColumn();
      if ( col == inColumn ) 
      {
        *outPos = cells - mRow_Cells;
        return cells;
      }
      else
        ++cells;
    }
  }
  *outPos = -1;
  return (morkCell*) 0;
}

mork_aid
morkRow::GetCellAtomAid(morkEnv* ev, mdb_column inColumn) const
  
  
  
  
  
{
  if ( this && this->IsRow() )
  {
    morkCell* cells = mRow_Cells;
    if ( cells )
    {
      morkCell* end = cells + mRow_Length;
      while ( cells < end )
      {
        mork_column col = cells->GetColumn();
        if ( col == inColumn ) 
        {
          morkAtom* atom = cells->mCell_Atom;
          if ( atom && atom->IsBook() )
            return ((morkBookAtom*) atom)->mBookAtom_Id;
          else
            return 0;
        }
        else
          ++cells;
      }
    }
  }
  else
    this->NonRowTypeError(ev);

  return 0;
}

void
morkRow::EmptyAllCells(morkEnv* ev)
{
  morkCell* cells = mRow_Cells;
  if ( cells )
  {
    morkStore* store = this->GetRowSpaceStore(ev);
    if ( store )
    {
      if ( this->MaybeDirtySpaceStoreAndRow() )
      {
        this->SetRowRewrite();
        this->NoteRowSetAll(ev);
      }
      morkPool* pool = store->StorePool();
      morkCell* end = cells + mRow_Length;
      --cells; 
      while ( ++cells < end )
      {
        if ( cells->mCell_Atom )
          cells->SetAtom(ev, (morkAtom*) 0, pool);
      }
    }
  }
}

void 
morkRow::cut_all_index_entries(morkEnv* ev)
{
  morkRowSpace* rowSpace = mRow_Space;
  if ( rowSpace->mRowSpace_IndexCount ) 
  {
    morkCell* cells = mRow_Cells;
    if ( cells )
    {
      morkCell* end = cells + mRow_Length;
      --cells; 
      while ( ++cells < end )
      {
        morkAtom* atom = cells->mCell_Atom;
        if ( atom )
        {
          mork_aid atomAid = atom->GetBookAtomAid();
          if ( atomAid )
          {
            mork_column col = cells->GetColumn();
            morkAtomRowMap* map = rowSpace->FindMap(ev, col);
            if ( map ) 
              map->CutAid(ev, atomAid);
          }
        }
      }
    }
  }
}

void
morkRow::CutAllColumns(morkEnv* ev)
{
  morkStore* store = this->GetRowSpaceStore(ev);
  if ( store )
  {
    if ( this->MaybeDirtySpaceStoreAndRow() )
    {
      this->SetRowRewrite();
      this->NoteRowSetAll(ev);
    }
    morkRowSpace* rowSpace = mRow_Space;
    if ( rowSpace->mRowSpace_IndexCount ) 
      this->cut_all_index_entries(ev);
  
    morkPool* pool = store->StorePool();
    pool->CutRowCells(ev, this,  0, &store->mStore_Zone);
  }
}

void
morkRow::SetRow(morkEnv* ev, const morkRow* inSourceRow)
{  
  
  morkStore* store = this->GetRowSpaceStore(ev);
  morkStore* srcStore = inSourceRow->GetRowSpaceStore(ev);
  if ( store && srcStore )
  {
    if ( this->MaybeDirtySpaceStoreAndRow() )
    {
      this->SetRowRewrite();
      this->NoteRowSetAll(ev);
    }
    morkRowSpace* rowSpace = mRow_Space;
    mork_count indexes = rowSpace->mRowSpace_IndexCount; 
    
    mork_bool sameStore = ( store == srcStore ); 
    morkPool* pool = store->StorePool();
    if ( pool->CutRowCells(ev, this,  0, &store->mStore_Zone) )
    {
      mork_fill fill = inSourceRow->mRow_Length;
      if ( pool->AddRowCells(ev, this, fill, &store->mStore_Zone) )
      {
        morkCell* dst = mRow_Cells;
        morkCell* dstEnd = dst + mRow_Length;
        
        const morkCell* src = inSourceRow->mRow_Cells;
        const morkCell* srcEnd = src + fill;
        --dst; --src; 
        
        while ( ++dst < dstEnd && ++src < srcEnd && ev->Good() )
        {
          morkAtom* atom = src->mCell_Atom;
          mork_column dstCol = src->GetColumn();
          
          
          
          if ( sameStore ) 
          {
            
            dst->SetCellColumnDirty(dstCol);
            dst->mCell_Atom = atom;
            if ( atom ) 
              atom->AddCellUse(ev);
          }
          else 
          {
            dstCol = store->CopyToken(ev, dstCol, srcStore);
            if ( dstCol )
            {
              
              dst->SetCellColumnDirty(dstCol);
              atom = store->CopyAtom(ev, atom);
              dst->mCell_Atom = atom;
              if ( atom ) 
                atom->AddCellUse(ev);
            }
          }
          if ( indexes && atom )
          {
            mork_aid atomAid = atom->GetBookAtomAid();
            if ( atomAid )
            {
              morkAtomRowMap* map = rowSpace->FindMap(ev, dstCol);
              if ( map )
                map->AddAid(ev, atomAid, this);
            }
          }
        }
      }
    }
  }
}

void
morkRow::AddRow(morkEnv* ev, const morkRow* inSourceRow)
{
  if ( mRow_Length ) 
  {
    ev->StubMethodOnlyError();
  }
  else
    this->SetRow(ev, inSourceRow); 
}

void
morkRow::OnZeroRowGcUse(morkEnv* ev)

{
  MORK_USED_1(ev);
  
}

void
morkRow::DirtyAllRowContent(morkEnv* ev)
{
  MORK_USED_1(ev);

  if ( this->MaybeDirtySpaceStoreAndRow() )
  {
    this->SetRowRewrite();
    this->NoteRowSetAll(ev);
  }
  morkCell* cells = mRow_Cells;
  if ( cells )
  {
    morkCell* end = cells + mRow_Length;
    --cells; 
    while ( ++cells < end )
    {
      cells->SetCellDirty();
    }
  }
}

morkStore*
morkRow::GetRowSpaceStore(morkEnv* ev) const
{
  morkRowSpace* rowSpace = mRow_Space;
  if ( rowSpace )
  {
    morkStore* store = rowSpace->mSpace_Store;
    if ( store )
    {
      if ( store->IsStore() )
      {
        return store;
      }
      else
        store->NonStoreTypeError(ev);
    }
    else
      ev->NilPointerError();
  }
  else
    ev->NilPointerError();
    
  return (morkStore*) 0;
}

void morkRow::CutColumn(morkEnv* ev, mdb_column inColumn)
{
  mork_pos pos = -1;
  morkCell* cell = this->GetCell(ev, inColumn, &pos);
  if ( cell ) 
  {
    morkStore* store = this->GetRowSpaceStore(ev);
    if ( store )
    {
      if ( this->MaybeDirtySpaceStoreAndRow() && !this->IsRowRewrite() )
        this->NoteRowCutCol(ev, inColumn);
        
      morkRowSpace* rowSpace = mRow_Space;
      morkAtomRowMap* map = ( rowSpace->mRowSpace_IndexCount )?
        rowSpace->FindMap(ev, inColumn) : (morkAtomRowMap*) 0;
      if ( map ) 
      {
        morkAtom* oldAtom = cell->mCell_Atom;
        if ( oldAtom ) 
        {
          mork_aid oldAid = oldAtom->GetBookAtomAid();
          if ( oldAid ) 
            map->CutAid(ev, oldAid);
        }
      }
      
      morkPool* pool = store->StorePool();
      cell->SetAtom(ev, (morkAtom*) 0, pool);
      
      mork_fill fill = mRow_Length; 
      MORK_ASSERT(fill);
      if ( fill ) 
      {
        mork_fill last = fill - 1; 
        
        if ( pos < (mork_pos)last ) 
        {
          morkCell* lastCell = mRow_Cells + last;
          mork_count after = last - pos; 
          morkCell* next = cell + 1; 
          MORK_MEMMOVE(cell, next, after * sizeof(morkCell));
          lastCell->SetColumnAndChange(0, 0);
          lastCell->mCell_Atom = 0;
        }
        
        if ( ev->Good() )
          pool->CutRowCells(ev, this, fill - 1, &store->mStore_Zone);
      }
    }
  }
}

morkAtom* morkRow::GetColumnAtom(morkEnv* ev, mdb_column inColumn)
{
  if ( ev->Good() )
  {
    mork_pos pos = -1;
    morkCell* cell = this->GetCell(ev, inColumn, &pos);
    if ( cell )
    	return cell->mCell_Atom;
  }
  return (morkAtom*) 0;
}

void morkRow::AddColumn(morkEnv* ev, mdb_column inColumn,
  const mdbYarn* inYarn, morkStore* ioStore)
{
  if ( ev->Good() )
  {
    mork_pos pos = -1;
    morkCell* cell = this->GetCell(ev, inColumn, &pos);
    morkCell* oldCell = cell; 
    if ( !cell ) 
      cell = this->NewCell(ev, inColumn, &pos, ioStore);
    
    if ( cell )
    {
      morkAtom* oldAtom = cell->mCell_Atom;

      morkAtom* atom = ioStore->YarnToAtom(ev, inYarn, PR_TRUE );
      if ( atom && atom != oldAtom )
      {
        morkRowSpace* rowSpace = mRow_Space;
        morkAtomRowMap* map = ( rowSpace->mRowSpace_IndexCount )?
          rowSpace->FindMap(ev, inColumn) : (morkAtomRowMap*) 0;
        
        if ( map ) 
        {
          if ( oldAtom && oldAtom != atom ) 
          {
            mork_aid oldAid = oldAtom->GetBookAtomAid();
            if ( oldAid ) 
              map->CutAid(ev, oldAid);
          }
        }
        
        cell->SetAtom(ev, atom, ioStore->StorePool()); 

        if ( oldCell ) 
        {
          ++mRow_Seed;
          if ( this->MaybeDirtySpaceStoreAndRow() && !this->IsRowRewrite() )
            this->NoteRowAddCol(ev, inColumn);
        }

        if ( map ) 
        {
          mork_aid newAid = atom->GetBookAtomAid();
          if ( newAid ) 
            map->AddAid(ev, newAid, this);
        }
      }
    }
  }
}

morkRowCellCursor*
morkRow::NewRowCellCursor(morkEnv* ev, mdb_pos inPos)
{
  morkRowCellCursor* outCursor = 0;
  if ( ev->Good() )
  {
    morkStore* store = this->GetRowSpaceStore(ev);
    if ( store )
    {
      morkRowObject* rowObj = this->AcquireRowObject(ev, store);
      if ( rowObj )
      {
        nsIMdbHeap* heap = store->mPort_Heap;
        morkRowCellCursor* cursor = new(*heap, ev)
          morkRowCellCursor(ev, morkUsage::kHeap, heap, rowObj);
         
        if ( cursor )
        {
          if ( ev->Good() )
          {
            cursor->mCursor_Pos = inPos;
            outCursor = cursor;
          }
          else
            cursor->CutStrongRef(ev->mEnv_SelfAsMdbEnv);
        }
        rowObj->Release(); 
      }
    }
  }
  return outCursor;
}




