




































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

#ifndef _MORKSPACE_
#include "morkSpace.h"
#endif

#ifndef _MORKNODEMAP_
#include "morkNodeMap.h"
#endif

#ifndef _MORKROWMAP_
#include "morkRowMap.h"
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

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif

#ifndef _MORKTABLE_
#include "morkTable.h"
#endif

#ifndef _MORKROW_
#include "morkRow.h"
#endif

#ifndef _MORKATOMMAP_
#include "morkAtomMap.h"
#endif

#ifndef _MORKROWOBJECT_
#include "morkRowObject.h"
#endif






 void
morkRowSpace::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseRowSpace(ev);
    this->MarkShut();  
  }
}


morkRowSpace::~morkRowSpace() 
{
  MORK_ASSERT(this->IsShutNode());
}


morkRowSpace::morkRowSpace(morkEnv* ev, 
  const morkUsage& inUsage, mork_scope inScope, morkStore* ioStore,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
: morkSpace(ev, inUsage, inScope, ioStore, ioHeap, ioSlotHeap)
, mRowSpace_SlotHeap( ioSlotHeap )
, mRowSpace_Rows(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioSlotHeap,
  morkRowSpace_kStartRowMapSlotCount)
, mRowSpace_Tables(ev, morkUsage::kMember, (nsIMdbHeap*) 0, ioSlotHeap)
, mRowSpace_NextTableId( 1 )
, mRowSpace_NextRowId( 1 )

, mRowSpace_IndexCount( 0 )
{
  morkAtomRowMap** cache = mRowSpace_IndexCache;
  morkAtomRowMap** cacheEnd = cache + morkRowSpace_kPrimeCacheSize;
  while ( cache < cacheEnd )
    *cache++ = 0; 
    
  if ( ev->Good() )
  {
    if ( ioSlotHeap )
    {
      mNode_Derived = morkDerived_kRowSpace;
      
      
    }
    else
      ev->NilPointerError();
  }
}

 void
morkRowSpace::CloseRowSpace(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      morkAtomRowMap** cache = mRowSpace_IndexCache;
      morkAtomRowMap** cacheEnd = cache + morkRowSpace_kPrimeCacheSize;
      --cache; 
      while ( ++cache < cacheEnd )
      {
        if ( *cache )
          morkAtomRowMap::SlotStrongAtomRowMap(0, ev, cache);
      }
      
      mRowSpace_Tables.CloseMorkNode(ev);
      
      morkStore* store = mSpace_Store;
      if ( store )
        this->CutAllRows(ev, &store->mStore_Pool);
      
      mRowSpace_Rows.CloseMorkNode(ev);
      this->CloseSpace(ev);
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkRowSpace::NonRowSpaceTypeError(morkEnv* ev)
{
  ev->NewError("non morkRowSpace");
}

 void
morkRowSpace::ZeroKindError(morkEnv* ev)
{
  ev->NewError("zero table kind");
}

 void
morkRowSpace::ZeroScopeError(morkEnv* ev)
{
  ev->NewError("zero row scope");
}

 void
morkRowSpace::ZeroTidError(morkEnv* ev)
{
  ev->NewError("zero table ID");
}

 void
morkRowSpace::MinusOneRidError(morkEnv* ev)
{
  ev->NewError("row ID is -1");
}












mork_num
morkRowSpace::CutAllRows(morkEnv* ev, morkPool* ioPool)
{
  if ( this->IsRowSpaceClean() )
    this->MaybeDirtyStoreAndSpace();
  
  mork_num outSlots = mRowSpace_Rows.MapFill();

#ifdef MORK_ENABLE_ZONE_ARENAS
  MORK_USED_2(ev, ioPool);
  return 0;
#else 
  morkZone* zone = &mSpace_Store->mStore_Zone;
  morkRow* r = 0; 
  mork_change* c = 0;

#ifdef MORK_ENABLE_PROBE_MAPS
  morkRowProbeMapIter i(ev, &mRowSpace_Rows);
#else 
  morkRowMapIter i(ev, &mRowSpace_Rows);
#endif 
  
  for ( c = i.FirstRow(ev, &r); c && ev->Good();
        c = i.NextRow(ev, &r) )
  {
    if ( r )
    {
      if ( r->IsRow() )
      {
        if ( r->mRow_Object )
        {
          morkRowObject::SlotWeakRowObject((morkRowObject*) 0, ev,
            &r->mRow_Object);
        }
        ioPool->ZapRow(ev, r, zone);
      }
      else
        r->NonRowTypeWarning(ev);
    }
    else
      ev->NilPointerError();
    
#ifdef MORK_ENABLE_PROBE_MAPS
    
#else 
    i.CutHereRow(ev,  (morkRow**) 0);
#endif 
  }
#endif 
  
  
  return outSlots;
}

morkTable*
morkRowSpace::FindTableByKind(morkEnv* ev, mork_kind inTableKind)
{
  if ( inTableKind )
  {
#ifdef MORK_BEAD_OVER_NODE_MAPS

    morkTableMapIter i(ev, &mRowSpace_Tables);
    morkTable* table = i.FirstTable(ev);
    for ( ; table && ev->Good(); table = i.NextTable(ev) )
          
#else 
    mork_tid* key = 0; 
    morkTable* table = 0; 

    mork_change* c = 0;
    morkTableMapIter i(ev, &mRowSpace_Tables);
    for ( c = i.FirstTable(ev, key, &table); c && ev->Good();
          c = i.NextTable(ev, key, &table) )
#endif 
    {
      if ( table->mTable_Kind == inTableKind )
        return table;
    }
  }
  else
    this->ZeroKindError(ev);
    
  return (morkTable*) 0;
}

morkTable*
morkRowSpace::NewTableWithTid(morkEnv* ev, mork_tid inTid,
  mork_kind inTableKind,
  const mdbOid* inOptionalMetaRowOid) 
{
  morkTable* outTable = 0;
  morkStore* store = mSpace_Store;
  
  if ( inTableKind && store )
  {
    mdb_bool mustBeUnique = morkBool_kFalse;
    nsIMdbHeap* heap = store->mPort_Heap;
    morkTable* table = new(*heap, ev)
      morkTable(ev, morkUsage::kHeap, heap, store, heap, this,
        inOptionalMetaRowOid, inTid, inTableKind, mustBeUnique);
    if ( table )
    {
      if ( mRowSpace_Tables.AddTable(ev, table) )
      {
        outTable = table;
        if ( mRowSpace_NextTableId <= inTid )
          mRowSpace_NextTableId = inTid + 1;
      }
        
      if ( this->IsRowSpaceClean() && store->mStore_CanDirty )
        this->MaybeDirtyStoreAndSpace(); 

    }
  }
  else if ( store )
    this->ZeroKindError(ev);
  else
    this->NilSpaceStoreError(ev);
    
  return outTable;
}

morkTable*
morkRowSpace::NewTable(morkEnv* ev, mork_kind inTableKind,
  mdb_bool inMustBeUnique,
  const mdbOid* inOptionalMetaRowOid) 
{
  morkTable* outTable = 0;
  morkStore* store = mSpace_Store;
  
  if ( inTableKind && store )
  {
    if ( inMustBeUnique ) 
      outTable = this->FindTableByKind(ev, inTableKind);
      
    if ( !outTable && ev->Good() )
    {
      mork_tid id = this->MakeNewTableId(ev);
      if ( id )
      {
        nsIMdbHeap* heap = mSpace_Store->mPort_Heap;
        morkTable* table = new(*heap, ev)
          morkTable(ev, morkUsage::kHeap, heap, mSpace_Store, heap, this,
            inOptionalMetaRowOid, id, inTableKind, inMustBeUnique);
        if ( table )
        {
          if ( mRowSpace_Tables.AddTable(ev, table) )
            outTable = table;
          else
            table->Release();

          if ( this->IsRowSpaceClean() && store->mStore_CanDirty )
            this->MaybeDirtyStoreAndSpace(); 
        }
      }
    }
  }
  else if ( store )
    this->ZeroKindError(ev);
  else
    this->NilSpaceStoreError(ev);
    
  return outTable;
}

mork_tid
morkRowSpace::MakeNewTableId(morkEnv* ev)
{
  mork_tid outTid = 0;
  mork_tid id = mRowSpace_NextTableId;
  mork_num count = 9; 
  
  while ( !outTid && --count ) 
  {
    if ( !mRowSpace_Tables.GetTable(ev, id) )
      outTid = id;
    else
    {
      MORK_ASSERT(morkBool_kFalse); 
      ++id;
    }
  }
  
  mRowSpace_NextTableId = id + 1;
  return outTid;
}

mork_rid
morkRowSpace::MakeNewRowId(morkEnv* ev)
{
  mork_rid outRid = 0;
  mork_rid id = mRowSpace_NextRowId;
  mork_num count = 9; 
  mdbOid oid;
  oid.mOid_Scope = this->SpaceScope();
  
  while ( !outRid && --count ) 
  {
    oid.mOid_Id = id;
    if ( !mRowSpace_Rows.GetOid(ev, &oid) )
      outRid = id;
    else
    {
      MORK_ASSERT(morkBool_kFalse); 
      ++id;
    }
  }
  
  mRowSpace_NextRowId = id + 1;
  return outRid;
}

morkAtomRowMap*
morkRowSpace::make_index(morkEnv* ev, mork_column inCol)
{
  morkAtomRowMap* outMap = 0;
  nsIMdbHeap* heap = mRowSpace_SlotHeap;
  if ( heap ) 
  {
    morkAtomRowMap* map = new(*heap, ev)
      morkAtomRowMap(ev, morkUsage::kHeap, heap, heap, inCol);
    
    if ( map ) 
    {
      if ( ev->Good() ) 
      {
#ifdef MORK_ENABLE_PROBE_MAPS
        morkRowProbeMapIter i(ev, &mRowSpace_Rows);
#else 
        morkRowMapIter i(ev, &mRowSpace_Rows);
#endif 
        mork_change* c = 0;
        morkRow* row = 0;
        mork_aid aidKey = 0;
        
        for ( c = i.FirstRow(ev, &row); c && ev->Good();
              c = i.NextRow(ev, &row) ) 
        {
          aidKey = row->GetCellAtomAid(ev, inCol);
          if ( aidKey ) 
            map->AddAid(ev, aidKey, row); 
        }
      }
      if ( ev->Good() ) 
        outMap = map; 
      else
        map->CutStrongRef(ev); 
    }
  }
  else
    ev->NilPointerError();
  
  return outMap;
}

morkAtomRowMap*
morkRowSpace::ForceMap(morkEnv* ev, mork_column inCol)
{
  morkAtomRowMap* outMap = this->FindMap(ev, inCol);
  
  if ( !outMap && ev->Good() ) 
  {
    if ( mRowSpace_IndexCount < morkRowSpace_kMaxIndexCount )
    {
      morkAtomRowMap* map = this->make_index(ev, inCol);
      if ( map ) 
      {
        mork_count wrap = 0; 
        morkAtomRowMap** slot = mRowSpace_IndexCache; 
        morkAtomRowMap** end = slot + morkRowSpace_kPrimeCacheSize;
        slot += ( inCol % morkRowSpace_kPrimeCacheSize ); 
        while ( *slot ) 
        {
          if ( ++slot >= end ) 
          {
            slot = mRowSpace_IndexCache; 
            if ( ++wrap > 1 ) 
            {
              ev->NewError("no free cache slots"); 
              break; 
            }
          }
        }
        if ( ev->Good() ) 
        {
          ++mRowSpace_IndexCount; 
          *slot = map; 
          outMap = map; 
        }
        else
          map->CutStrongRef(ev); 
      }
    }
    else
      ev->NewError("too many indexes"); 
  }
  return outMap;
}

morkAtomRowMap*
morkRowSpace::FindMap(morkEnv* ev, mork_column inCol)
{
  if ( mRowSpace_IndexCount && ev->Good() )
  {
    mork_count wrap = 0; 
    morkAtomRowMap** slot = mRowSpace_IndexCache; 
    morkAtomRowMap** end = slot + morkRowSpace_kPrimeCacheSize;
    slot += ( inCol % morkRowSpace_kPrimeCacheSize ); 
    morkAtomRowMap* map = *slot;
    while ( map ) 
    {
      if ( inCol == map->mAtomRowMap_IndexColumn ) 
        return map;
      if ( ++slot >= end ) 
      {
        slot = mRowSpace_IndexCache;
        if ( ++wrap > 1 ) 
          return (morkAtomRowMap*) 0; 
      }
      map = *slot;
    }
  }
  return (morkAtomRowMap*) 0;
}

morkRow*
morkRowSpace::FindRow(morkEnv* ev, mork_column inCol, const mdbYarn* inYarn)
{
  morkRow* outRow = 0;

  
  
  morkAtom* atom = mSpace_Store->YarnToAtom(ev, inYarn, PR_FALSE);
  if ( atom ) 
  {
    mork_aid atomAid = atom->GetBookAtomAid();
    if ( atomAid ) 
    {
      morkAtomRowMap* map = this->ForceMap(ev, inCol);
      if ( map ) 
      {
        outRow = map->GetAid(ev, atomAid); 
      }
    }
  }
  
  return outRow;
}

morkRow*
morkRowSpace::NewRowWithOid(morkEnv* ev, const mdbOid* inOid)
{
  morkRow* outRow = mRowSpace_Rows.GetOid(ev, inOid);
  MORK_ASSERT(outRow==0);
  if ( !outRow && ev->Good() )
  {
    morkStore* store = mSpace_Store;
    if ( store )
    {
      morkPool* pool = this->GetSpaceStorePool();
      morkRow* row = pool->NewRow(ev, &store->mStore_Zone);
      if ( row )
      {
        row->InitRow(ev, inOid, this,  0, pool);
        
        if ( ev->Good() && mRowSpace_Rows.AddRow(ev, row) )
        {
          outRow = row;
          mork_rid rid = inOid->mOid_Id;
          if ( mRowSpace_NextRowId <= rid )
            mRowSpace_NextRowId = rid + 1;
        }
        else
          pool->ZapRow(ev, row, &store->mStore_Zone);

        if ( this->IsRowSpaceClean() && store->mStore_CanDirty )
          this->MaybeDirtyStoreAndSpace(); 
      }
    }
    else
      this->NilSpaceStoreError(ev);
  }
  return outRow;
}

morkRow*
morkRowSpace::NewRow(morkEnv* ev)
{
  morkRow* outRow = 0;
  if ( ev->Good() )
  {
    mork_rid id = this->MakeNewRowId(ev);
    if ( id )
    {
      morkStore* store = mSpace_Store;
      if ( store )
      {
        mdbOid oid;
        oid.mOid_Scope = this->SpaceScope();
        oid.mOid_Id = id;
        morkPool* pool = this->GetSpaceStorePool();
        morkRow* row = pool->NewRow(ev, &store->mStore_Zone);
        if ( row )
        {
          row->InitRow(ev, &oid, this,  0, pool);
          
          if ( ev->Good() && mRowSpace_Rows.AddRow(ev, row) )
            outRow = row;
          else
            pool->ZapRow(ev, row, &store->mStore_Zone);

          if ( this->IsRowSpaceClean() && store->mStore_CanDirty )
            this->MaybeDirtyStoreAndSpace(); 
        }
      }
      else
        this->NilSpaceStoreError(ev);
    }
  }
  return outRow;
}





morkRowSpaceMap::~morkRowSpaceMap()
{
}

morkRowSpaceMap::morkRowSpaceMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap)
  : morkNodeMap(ev, inUsage, ioHeap, ioSlotHeap)
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kRowSpaceMap;
}


