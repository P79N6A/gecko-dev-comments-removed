




































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

#ifndef _MORKSORTING_
#include "morkSorting.h"
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

#ifndef _ORKINSORTING_
#include "orkinSorting.h"
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




orkinSorting:: ~orkinSorting() 
{
}


orkinSorting::orkinSorting(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkSorting* ioObject)  
: morkHandle(ev, ioFace, ioObject, morkMagic_kSorting)
{
  
}


 orkinSorting*
orkinSorting::MakeSorting(morkEnv* ev, morkSorting* ioObject)
{
  mork_bool isEnv = ev->IsEnv();
  MORK_ASSERT(isEnv);
  if ( isEnv )
  {
    morkHandleFace* face = ev->NewHandle(sizeof(orkinSorting));
    if ( face )
      return new(face) orkinSorting(ev, face, ioObject);
    else
      ev->OutOfMemoryError();
  }
    
  return (orkinSorting*) 0;
}

morkEnv*
orkinSorting::CanUseSorting(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const
{
  morkEnv* outEnv = 0;
  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    morkSorting* self = (morkSorting*)
      this->GetGoodHandleObject(ev, inMutable, morkMagic_kSorting,
         morkBool_kFalse);
    if ( self )
    {
      if ( self->IsSorting() )
        outEnv = ev;
      else
        self->NonSortingTypeError(ev);
    }
    *outErr = ev->AsErr();
  }
  MORK_ASSERT(outEnv);
  return outEnv;
}



NS_IMPL_QUERY_INTERFACE0(orkinSorting)

 nsrefcnt
orkinSorting::AddRef() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_AddStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}

 nsrefcnt
orkinSorting::Release() 
{
  morkEnv* ev = mHandle_Env;
  if ( ev && ev->IsEnv() )
    return this->Handle_CutStrongRef(ev->AsMdbEnv());
  else
    return morkEnv_kNonEnvTypeError;
}





 mdb_err
orkinSorting::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  return this->Handle_IsFrozenMdbObject(mev, outIsReadonly);
}




 mdb_err
orkinSorting::GetMdbFactory(nsIMdbEnv* mev, nsIMdbFactory** acqFactory)
{
  return this->Handle_GetMdbFactory(mev, acqFactory);
} 



 mdb_err
orkinSorting::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetWeakRefCount(mev, outCount);
}  
 mdb_err
orkinSorting::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  return this->Handle_GetStrongRefCount(mev, outCount);
}

 mdb_err
orkinSorting::AddWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_AddWeakRef(mev);
}
 mdb_err
orkinSorting::AddStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_AddStrongRef(mev);
}

 mdb_err
orkinSorting::CutWeakRef(nsIMdbEnv* mev)
{
  return this->Handle_CutWeakRef(mev);
}
 mdb_err
orkinSorting::CutStrongRef(nsIMdbEnv* mev)
{
  return this->Handle_CutStrongRef(mev);
}

 mdb_err
orkinSorting::CloseMdbObject(nsIMdbEnv* mev)
{
  return this->Handle_CloseMdbObject(mev);
}

 mdb_err
orkinSorting::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  return this->Handle_IsOpenMdbObject(mev, outOpen);
}










 mdb_err
orkinSorting::GetTable(nsIMdbEnv* mev, nsIMdbTable** acqTable)
{
  mdb_err outErr = 0;
  nsIMdbTable* outTable = 0;
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkSorting* sorting = (morkSorting*) mHandle_Object;
    morkTable* table = sorting->mSorting_Table;
    if ( table && ev->Good() )
    {
      outTable = table->AcquireTableHandle(ev);
    }
    outErr = ev->AsErr();
  }
  if ( acqTable )
    *acqTable = outTable;

  return outErr;
}

 mdb_err
orkinSorting::GetSortColumn( 
  nsIMdbEnv* mev, 
  mdb_column* outColumn) 
{
  mdb_err outErr = 0;
  mdb_column col = 0;
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkSorting* sorting = (morkSorting*) mHandle_Object;
    col = sorting->mSorting_Col;

    outErr = ev->AsErr();
  }
  if ( outColumn )
    *outColumn = col;

  return outErr;
}

 mdb_err
orkinSorting::SetNewCompare(nsIMdbEnv* mev,
  nsIMdbCompare* ioNewCompare)
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    if ( ioNewCompare )
    {
      morkSorting* sorting = (morkSorting*) mHandle_Object;
      nsIMdbCompare_SlotStrongCompare(ioNewCompare, ev,
        &sorting->mSorting_Compare);
    }
    else
      ev->NilPointerError();
      
    outErr = ev->AsErr();
  }

  return outErr;
}

 mdb_err
orkinSorting::GetOldCompare(nsIMdbEnv* mev,
  nsIMdbCompare** acqOldCompare)
  
  
  
  
  
  
{
  mdb_err outErr = 0;
  nsIMdbCompare* outCompare = 0;
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkSorting* sorting = (morkSorting*) mHandle_Object;
    nsIMdbCompare* compare = sorting->mSorting_Compare;
    if ( compare && ev->Good() )
    {
      compare->AddStrongRef(mev);
        
      if ( ev->Good() )
        outCompare = compare;
    }
    outErr = ev->AsErr();
  }
  if ( acqOldCompare )
    *acqOldCompare = outCompare;

  return outErr;
}  





 mdb_err
orkinSorting::GetSortingRowCursor( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  nsIMdbTableRowCursor** acqCursor) 
  
{
  mdb_err outErr = 0;
  nsIMdbTableRowCursor* outCursor = 0;
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkSortingRowCursor* cursor =
      ((morkSorting*) mHandle_Object)->NewSortingRowCursor(ev, inRowPos);
    if ( cursor )
    {
      
      
      
      
      
      
      
    }
      
    outErr = ev->AsErr();
  }
  if ( acqCursor )
    *acqCursor = outCursor;
  return outErr;
}




 mdb_err
orkinSorting::PosToOid( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  mdbOid* outOid) 
{
  mdb_err outErr = 0;
  mdbOid roid;
  roid.mOid_Scope = 0;
  roid.mOid_Id = (mork_id) -1;
  
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkSorting* sorting = (morkSorting*) mHandle_Object;
    morkRow* row = sorting->SafeRowAt(ev, inRowPos);
    if ( row )
      roid = row->mRow_Oid;
    
    outErr = ev->AsErr();
  }
  if ( outOid )
    *outOid = roid;
  return outErr;
}

 mdb_err
orkinSorting::PosToRow( 
  nsIMdbEnv* mev, 
  mdb_pos inRowPos, 
  nsIMdbRow** acqRow) 
{
  mdb_err outErr = 0;
  nsIMdbRow* outRow = 0;
  morkEnv* ev = this->CanUseSorting(mev,  morkBool_kFalse, &outErr);
  if ( ev )
  {
    morkSorting* sorting = (morkSorting*) mHandle_Object;
    morkStore* store = sorting->mSorting_Table->mTable_Store;
    morkRow* row = sorting->SafeRowAt(ev, inRowPos);
    if ( row && store )
      outRow = row->AcquireRowHandle(ev, store);
      
    outErr = ev->AsErr();
  }
  if ( acqRow )
    *acqRow = outRow;
  return outErr;
}
  








