




































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

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif






 void
morkCursor::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseCursor(ev);
    this->MarkShut();
  }
}


morkCursor::~morkCursor() 
{
}


morkCursor::morkCursor(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap)
: morkObject(ev, inUsage, ioHeap, morkColor_kNone, (morkHandle*) 0)
, mCursor_Seed( 0 )
, mCursor_Pos( -1 )
, mCursor_DoFailOnSeedOutOfSync( morkBool_kFalse )
{
  if ( ev->Good() )
    mNode_Derived = morkDerived_kCursor;
}

NS_IMPL_ISUPPORTS_INHERITED1(morkCursor, morkObject, nsIMdbCursor)

 void
morkCursor::CloseCursor(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mCursor_Seed = 0;
      mCursor_Pos = -1;
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}


NS_IMETHODIMP
morkCursor::GetWeakRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  *outCount = WeakRefsOnly();
  return NS_OK;
}  
NS_IMETHODIMP
morkCursor::GetStrongRefCount(nsIMdbEnv* mev, 
  mdb_count* outCount)
{
  *outCount = StrongRefsOnly();
  return NS_OK;
}

NS_IMETHODIMP
morkCursor::AddWeakRef(nsIMdbEnv* mev)
{
  return morkNode::AddWeakRef((morkEnv *) mev);
}
NS_IMETHODIMP
morkCursor::AddStrongRef(nsIMdbEnv* mev)
{
  return morkNode::AddStrongRef((morkEnv *) mev);
}

NS_IMETHODIMP
morkCursor::CutWeakRef(nsIMdbEnv* mev)
{
  return morkNode::CutWeakRef((morkEnv *) mev);
}
NS_IMETHODIMP
morkCursor::CutStrongRef(nsIMdbEnv* mev)
{
  return morkNode::CutStrongRef((morkEnv *) mev);
}

  
NS_IMETHODIMP
morkCursor::CloseMdbObject(nsIMdbEnv* mev)
{
  return morkNode::CloseMdbObject((morkEnv *) mev);
}

NS_IMETHODIMP
morkCursor::IsOpenMdbObject(nsIMdbEnv* mev, mdb_bool* outOpen)
{
  *outOpen = IsOpenNode();
  return NS_OK;
}
NS_IMETHODIMP
morkCursor::IsFrozenMdbObject(nsIMdbEnv* mev, mdb_bool* outIsReadonly)
{
  *outIsReadonly = IsFrozen();
  return NS_OK;
}



NS_IMETHODIMP
morkCursor::GetCount(nsIMdbEnv* mev, mdb_count* outCount)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkCursor::GetSeed(nsIMdbEnv* mev, mdb_seed* outSeed)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkCursor::SetPos(nsIMdbEnv* mev, mdb_pos inPos)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkCursor::GetPos(nsIMdbEnv* mev, mdb_pos* outPos)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkCursor::SetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool inFail)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
morkCursor::GetDoFailOnSeedOutOfSync(nsIMdbEnv* mev, mdb_bool* outFail)
{
  NS_ASSERTION(PR_FALSE, "not implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}



