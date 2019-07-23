




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif

#ifndef _MORKPORTTABLECURSOR_
#include "morkPortTableCursor.h"
#endif

#ifndef _MORKSTORE_
#include "morkStore.h"
#endif






 void
morkPortTableCursor::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->ClosePortTableCursor(ev);
    this->MarkShut();
  }
}


morkPortTableCursor::~morkPortTableCursor() 
{
  CloseMorkNode(mMorkEnv);
}


morkPortTableCursor::morkPortTableCursor(morkEnv* ev,
  const morkUsage& inUsage,
  nsIMdbHeap* ioHeap, morkStore* ioStore, mdb_scope inRowScope,
  mdb_kind inTableKind, nsIMdbHeap* ioSlotHeap)
: morkCursor(ev, inUsage, ioHeap)
, mPortTableCursor_Store( 0 )
, mPortTableCursor_RowScope( (mdb_scope) -1 ) 
, mPortTableCursor_TableKind( (mdb_kind) -1 ) 
, mPortTableCursor_LastTable ( 0 ) 
, mPortTableCursor_RowSpace( 0 ) 
, mPortTableCursor_TablesDidEnd( morkBool_kFalse )
, mPortTableCursor_SpacesDidEnd( morkBool_kFalse )
{
  if ( ev->Good() )
  {
    if ( ioStore && ioSlotHeap )
    {
      mCursor_Pos = -1;
      mCursor_Seed = 0; 
      morkStore::SlotWeakStore(ioStore, ev, &mPortTableCursor_Store);

      if ( this->SetRowScope(ev, inRowScope) )
        this->SetTableKind(ev, inTableKind);
        
      if ( ev->Good() )
        mNode_Derived = morkDerived_kPortTableCursor;
    }
    else
      ev->NilPointerError();
  }
}

NS_IMPL_ISUPPORTS_INHERITED1(morkPortTableCursor, morkCursor, nsIMdbPortTableCursor)

morkEnv*
morkPortTableCursor::CanUsePortTableCursor(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( IsPortTableCursor() )
      outEnv = ev;
    else
      NonPortTableCursorTypeError(ev);
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}


 void
morkPortTableCursor::ClosePortTableCursor(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mCursor_Pos = -1;
      mCursor_Seed = 0;
      mPortTableCursor_LastTable = 0;
      morkStore::SlotWeakStore((morkStore*) 0, ev, &mPortTableCursor_Store);
      morkRowSpace::SlotStrongRowSpace((morkRowSpace*) 0, ev,
        &mPortTableCursor_RowSpace);
      this->CloseCursor(ev);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkPortTableCursor::NilCursorStoreError(morkEnv* ev)
{
  ev->NewError("nil mPortTableCursor_Store");
}

 void
morkPortTableCursor::NonPortTableCursorTypeError(morkEnv* ev)
{
  ev->NewError("non morkPortTableCursor");
}

mork_bool 
morkPortTableCursor::SetRowScope(morkEnv* ev, mork_scope inRowScope)
{
  mPortTableCursor_RowScope = inRowScope;
  mPortTableCursor_LastTable = 0; 
  
  mPortTableCursor_TableIter.CloseMapIter(ev);
  mPortTableCursor_TablesDidEnd = morkBool_kTrue;
  mPortTableCursor_SpacesDidEnd = morkBool_kTrue;
  
  morkStore* store = mPortTableCursor_Store;
  if ( store )
  {
    morkRowSpace* space = mPortTableCursor_RowSpace;

    if ( inRowScope ) 
    {
      space = store->LazyGetRowSpace(ev, inRowScope);
      morkRowSpace::SlotStrongRowSpace(space, ev, 
       &mPortTableCursor_RowSpace);
       
      
      
    }
    else 
    {
      morkRowSpaceMapIter* rsi = &mPortTableCursor_SpaceIter;
      rsi->InitRowSpaceMapIter(ev, &store->mStore_RowSpaces);
      
      space = 0;
      (void) rsi->FirstRowSpace(ev, (mork_scope*) 0, &space);
      morkRowSpace::SlotStrongRowSpace(space, ev,
        &mPortTableCursor_RowSpace);
        
      if ( space ) 
        mPortTableCursor_SpacesDidEnd = morkBool_kFalse;
    }

    this->init_space_tables_map(ev);
  }
  else
    this->NilCursorStoreError(ev);
    
  return ev->Good();
}

void
morkPortTableCursor::init_space_tables_map(morkEnv* ev)
{
  morkRowSpace* space = mPortTableCursor_RowSpace;
  if ( space && ev->Good() )
  {
    morkTableMapIter* ti = &mPortTableCursor_TableIter;
    ti->InitTableMapIter(ev, &space->mRowSpace_Tables);
    if ( ev->Good() )
      mPortTableCursor_TablesDidEnd = morkBool_kFalse;
  }
}


mork_bool
morkPortTableCursor::SetTableKind(morkEnv* ev, mork_kind inTableKind)
{
  mPortTableCursor_TableKind = inTableKind;
  mPortTableCursor_LastTable = 0; 

  mPortTableCursor_TablesDidEnd = morkBool_kTrue;

  morkRowSpace* space = mPortTableCursor_RowSpace;
  if ( !space && mPortTableCursor_RowScope == 0 )
  {
    this->SetRowScope(ev, 0);
    space = mPortTableCursor_RowSpace;
  }
  this->init_space_tables_map(ev);
  
  return ev->Good();
}

morkRowSpace*
morkPortTableCursor::NextSpace(morkEnv* ev)
{
  morkRowSpace* outSpace = 0;
  mPortTableCursor_LastTable = 0;
  mPortTableCursor_SpacesDidEnd = morkBool_kTrue;
  mPortTableCursor_TablesDidEnd = morkBool_kTrue;

  if ( !mPortTableCursor_RowScope ) 
  {
    morkStore* store = mPortTableCursor_Store;
    if ( store )
    {
      morkRowSpaceMapIter* rsi = &mPortTableCursor_SpaceIter;

      (void) rsi->NextRowSpace(ev, (mork_scope*) 0, &outSpace);
      morkRowSpace::SlotStrongRowSpace(outSpace, ev,
        &mPortTableCursor_RowSpace);
        
      if ( outSpace ) 
      {
        mPortTableCursor_SpacesDidEnd = morkBool_kFalse;
        
        this->init_space_tables_map(ev);

        if ( ev->Bad() )
          outSpace = 0;
      }
    }
    else
      this->NilCursorStoreError(ev);
  }
    
  return outSpace;
}

morkTable *
morkPortTableCursor::NextTable(morkEnv* ev)
{
  mork_kind kind = mPortTableCursor_TableKind;
  
  do 
  { 
    morkRowSpace* space = mPortTableCursor_RowSpace;
    if ( mPortTableCursor_TablesDidEnd ) 
      space = this->NextSpace(ev); 
      
    if ( space ) 
    {
#ifdef MORK_BEAD_OVER_NODE_MAPS
      morkTableMapIter* ti = &mPortTableCursor_TableIter;
      morkTable* table = ( mPortTableCursor_LastTable )?
        ti->NextTable(ev) : ti->FirstTable(ev);

      for ( ; table && ev->Good(); table = ti->NextTable(ev) )
      
#else 
      mork_tid* key = 0; 
      morkTable* table = 0; 
      morkTableMapIter* ti = &mPortTableCursor_TableIter;
      mork_change* c = ( mPortTableCursor_LastTable )?
        ti->NextTable(ev, key, &table) : ti->FirstTable(ev, key, &table);

      for ( ; c && ev->Good(); c = ti->NextTable(ev, key, &table) )
#endif 
      {
        if ( table && table->IsTable() )
        {
          if ( !kind || kind == table->mTable_Kind )
          {
            mPortTableCursor_LastTable = table; 
            return table;
          }
        }
        else
          table->NonTableTypeWarning(ev);
      }
      mPortTableCursor_TablesDidEnd = morkBool_kTrue; 
      mPortTableCursor_LastTable = 0; 
    }
  
  } while ( ev->Good() && !mPortTableCursor_SpacesDidEnd );

  return (morkTable*) 0;
}







NS_IMETHODIMP
morkPortTableCursor::SetPort(nsIMdbEnv* mev, nsIMdbPort* ioPort)
{
  NS_ASSERTION(PR_FALSE,"not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkPortTableCursor::GetPort(nsIMdbEnv* mev, nsIMdbPort** acqPort)
{
  mdb_err outErr = 0;
  nsIMdbPort* outPort = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( mPortTableCursor_Store )
      outPort = mPortTableCursor_Store->AcquireStoreHandle(ev);
    outErr = ev->AsErr();
  }
  if ( acqPort )
    *acqPort = outPort;
  return outErr;
}

NS_IMETHODIMP
morkPortTableCursor::SetRowScope(nsIMdbEnv* mev, 
  mdb_scope inRowScope)
{
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    mCursor_Pos = -1;
    
    SetRowScope(ev, inRowScope);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkPortTableCursor::GetRowScope(nsIMdbEnv* mev, mdb_scope* outRowScope)
{
  mdb_err outErr = 0;
  mdb_scope rowScope = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    rowScope = mPortTableCursor_RowScope;
    outErr = ev->AsErr();
  }
  *outRowScope = rowScope;
  return outErr;
}

  
NS_IMETHODIMP
morkPortTableCursor::SetTableKind(nsIMdbEnv* mev, 
  mdb_kind inTableKind)
{
  mdb_err outErr = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    mCursor_Pos = -1;
    
    SetTableKind(ev, inTableKind);
    outErr = ev->AsErr();
  }
  return outErr;
}

NS_IMETHODIMP
morkPortTableCursor::GetTableKind(nsIMdbEnv* mev, mdb_kind* outTableKind)

{
  mdb_err outErr = 0;
  mdb_kind tableKind = 0;
  morkEnv* ev =
    this->CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    tableKind = mPortTableCursor_TableKind;
    outErr = ev->AsErr();
  }
  *outTableKind = tableKind;
  return outErr;
}



NS_IMETHODIMP
morkPortTableCursor::NextTable( 
  nsIMdbEnv* mev, 
  nsIMdbTable** acqTable)
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev =
    CanUsePortTableCursor(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkTable* table = NextTable(ev);
    if ( table && ev->Good() )
      outTable = table->AcquireTableHandle(ev);
        
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;
  return outErr;
}


