




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif





static morkUsage morkUsage_gHeap; 
const morkUsage& morkUsage::kHeap = morkUsage_gHeap;

static morkUsage morkUsage_gStack; 
const morkUsage& morkUsage::kStack = morkUsage_gStack;

static morkUsage morkUsage_gMember; 
const morkUsage& morkUsage::kMember = morkUsage_gMember;

static morkUsage morkUsage_gGlobal; 
const morkUsage& morkUsage::kGlobal = morkUsage_gGlobal;

static morkUsage morkUsage_gPool; 
const morkUsage& morkUsage::kPool = morkUsage_gPool;

static morkUsage morkUsage_gNone; 
const morkUsage& morkUsage::kNone = morkUsage_gNone;







static mork_u4 morkUsage_g_static_init_target; 
static mork_u4* morkUsage_g_static_init_done; 

#define morkUsage_do_static_init() \
  ( morkUsage_g_static_init_done = &morkUsage_g_static_init_target )

#define morkUsage_need_static_init() \
  ( morkUsage_g_static_init_done != &morkUsage_g_static_init_target )


void morkUsage::EnsureReadyStaticUsage()
{
  if ( morkUsage_need_static_init() )
  {
    morkUsage_do_static_init();

    morkUsage_gHeap.InitUsage(morkUsage_kHeap);
    morkUsage_gStack.InitUsage(morkUsage_kStack);
    morkUsage_gMember.InitUsage(morkUsage_kMember);
    morkUsage_gGlobal.InitUsage(morkUsage_kGlobal);
    morkUsage_gPool.InitUsage(morkUsage_kPool);
    morkUsage_gNone.InitUsage(morkUsage_kNone);
  }
}


const morkUsage& morkUsage::GetHeap()   
{
  EnsureReadyStaticUsage();
  return morkUsage_gHeap;
}


const morkUsage& morkUsage::GetStack()  
{
  EnsureReadyStaticUsage();
  return morkUsage_gStack;
}


const morkUsage& morkUsage::GetMember() 
{
  EnsureReadyStaticUsage();
  return morkUsage_gMember;
}


const morkUsage& morkUsage::GetGlobal() 
{
  EnsureReadyStaticUsage();
  return morkUsage_gGlobal;
}


const morkUsage& morkUsage::GetPool()  
{
  EnsureReadyStaticUsage();
  return morkUsage_gPool;
}


const morkUsage& morkUsage::GetNone()  
{
  EnsureReadyStaticUsage();
  return morkUsage_gNone;
}

morkUsage::morkUsage()
{
  if ( morkUsage_need_static_init() )
  {
    morkUsage::EnsureReadyStaticUsage();
  }
}

morkUsage::morkUsage(mork_usage code)
  : mUsage_Code(code)
{
  if ( morkUsage_need_static_init() )
  {
    morkUsage::EnsureReadyStaticUsage();
  }
}



 void*
morkNode::MakeNew(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev)
{
  void* node = 0;
  if ( &ioHeap )
  {
    ioHeap.Alloc(ev->AsMdbEnv(), inSize, (void **) &node);
    if ( !node )
      ev->OutOfMemoryError();
  }
  else
    ev->NilPointerError();
  
  return node;
}

 void
morkNode::ZapOld(morkEnv* ev, nsIMdbHeap* ioHeap)
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      mork_usage usage = mNode_Usage; 
      this->morkNode::~morkNode(); 
      if ( ioHeap ) 
        ioHeap->Free(ev->AsMdbEnv(), this);
      else if ( usage == morkUsage_kPool ) 
      {
        morkHandle* h = (morkHandle*) this;
        if ( h->IsHandle() && h->GoodHandleTag() )
        {
          if ( h->mHandle_Face )
          {
            if (ev->mEnv_HandlePool)
              ev->mEnv_HandlePool->ZapHandle(ev, h->mHandle_Face);
            else if (h->mHandle_Env && h->mHandle_Env->mEnv_HandlePool)
              h->mHandle_Env->mEnv_HandlePool->ZapHandle(ev, h->mHandle_Face);
          }
          else
            ev->NilPointerError();
        }
      }
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}

 void
morkNode::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseNode(ev);
    this->MarkShut();
  }
}
NS_IMETHODIMP
morkNode::CloseMdbObject(nsIMdbEnv* mev)
{
  return morkNode::CloseMdbObject((morkEnv *) mev);
}

mdb_err morkNode::CloseMdbObject(morkEnv *ev)
{
  
  if (mNode_Uses == 1)
    return CutStrongRef(ev);

  mdb_err outErr = 0;
  
  if ( IsNode() && IsOpenNode() )
  {
    if ( ev )
    {
      CloseMorkNode(ev);
      outErr = ev->AsErr();
    }
  }
  return outErr;
}


morkNode::~morkNode() 
{
  MORK_ASSERT(this->IsShutNode() || IsDeadNode()); 
  mNode_Access = morkAccess_kDead;
  mNode_Usage = morkUsage_kNone;
}



  
  
  
  
  
  
  


morkNode::morkNode( mork_usage inCode )
: mNode_Heap( 0 )
, mNode_Base( morkBase_kNode )
, mNode_Derived ( 0 ) 
, mNode_Access( morkAccess_kOpen )
, mNode_Usage( inCode )
, mNode_Mutable( morkAble_kEnabled )
, mNode_Load( morkLoad_kClean )
, mNode_Uses( 1 )
, mNode_Refs( 1 )
{
}


morkNode::morkNode(const morkUsage& inUsage, nsIMdbHeap* ioHeap)
: mNode_Heap( ioHeap )
, mNode_Base( morkBase_kNode )
, mNode_Derived ( 0 ) 
, mNode_Access( morkAccess_kOpen )
, mNode_Usage( inUsage.Code() )
, mNode_Mutable( morkAble_kEnabled )
, mNode_Load( morkLoad_kClean )
, mNode_Uses( 1 )
, mNode_Refs( 1 )
{
  if ( !ioHeap && mNode_Usage == morkUsage_kHeap )
    MORK_ASSERT(ioHeap);
}


morkNode::morkNode(morkEnv* ev,
  const morkUsage& inUsage, nsIMdbHeap* ioHeap)
: mNode_Heap( ioHeap )
, mNode_Base( morkBase_kNode )
, mNode_Derived ( 0 ) 
, mNode_Access( morkAccess_kOpen )
, mNode_Usage( inUsage.Code() )
, mNode_Mutable( morkAble_kEnabled )
, mNode_Load( morkLoad_kClean )
, mNode_Uses( 1 )
, mNode_Refs( 1 )
{
  if ( !ioHeap && mNode_Usage == morkUsage_kHeap )
  {
    this->NilHeapError(ev);
  }
}

 void
morkNode::RefsUnderUsesWarning(morkEnv* ev) const 
{
  ev->NewError("mNode_Refs < mNode_Uses");
}

 void
morkNode::NonNodeError(morkEnv* ev) const 
{
  ev->NewError("non-morkNode");
}

 void
morkNode::NonOpenNodeError(morkEnv* ev) const 
{
  ev->NewError("non-open-morkNode");
}

 void
morkNode::NonMutableNodeError(morkEnv* ev) const 
{
  ev->NewError("non-mutable-morkNode");
}

 void
morkNode::NilHeapError(morkEnv* ev) const 
{
  ev->NewError("nil mNode_Heap");
}

 void
morkNode::RefsOverflowWarning(morkEnv* ev) const 
{
  ev->NewWarning("mNode_Refs overflow");
}

 void
morkNode::UsesOverflowWarning(morkEnv* ev) const 
{
  ev->NewWarning("mNode_Uses overflow");
}

 void
morkNode::RefsUnderflowWarning(morkEnv* ev) const 
{
  ev->NewWarning("mNode_Refs underflow");
}

 void
morkNode::UsesUnderflowWarning(morkEnv* ev) const 
{
  ev->NewWarning("mNode_Uses underflow");
}

 void
morkNode::CloseNode(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
      this->MarkShut();
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}


extern void 
nsIMdbCompare_SlotStrongCompare(nsIMdbCompare* self, morkEnv* ev,
  nsIMdbCompare** ioSlot)
  
  
  
  
  
{
  nsIMdbEnv* menv = ev->AsMdbEnv();
  nsIMdbCompare* compare = *ioSlot;
  if ( self != compare )
  {
    if ( compare )
    {
      *ioSlot = 0;
      compare->CutStrongRef(menv);
    }
    if ( self && ev->Good() && (self->AddStrongRef(menv)==0) && ev->Good() )
      *ioSlot = self;
  }
}


extern void 
nsIMdbFile_SlotStrongFile(nsIMdbFile* self, morkEnv* ev, nsIMdbFile** ioSlot)
  
  
  
  
  
{
  nsIMdbFile* file = *ioSlot;
  if ( self != file )
  {
    if ( file )
    {
      *ioSlot = 0;
      NS_RELEASE(file);
    }
    if ( self && ev->Good() && (NS_ADDREF(self)>=0) && ev->Good() )
      *ioSlot = self;
  }
}
  
void 
nsIMdbHeap_SlotStrongHeap(nsIMdbHeap* self, morkEnv* ev, nsIMdbHeap** ioSlot)
  
  
  
  
  
{
  nsIMdbEnv* menv = ev->AsMdbEnv();
  nsIMdbHeap* heap = *ioSlot;
  if ( self != heap )
  {
    if ( heap )
    {
      *ioSlot = 0;
      heap->HeapCutStrongRef(menv);
    }
    if ( self && ev->Good() && (self->HeapAddStrongRef(menv)==0) && ev->Good() )
      *ioSlot = self;
  }
}

 void
morkNode::SlotStrongNode(morkNode* me, morkEnv* ev, morkNode** ioSlot)
  
  
  
  
  
{
  morkNode* node = *ioSlot;
  if ( me != node )
  {
    if ( node )
    {
      
      
      *ioSlot = 0;
      node->CutStrongRef(ev);
    }
    if ( me && me->AddStrongRef(ev) )
      *ioSlot = me;
  }
}

 void
morkNode::SlotWeakNode(morkNode* me, morkEnv* ev, morkNode** ioSlot)
  
  
  
  
  
{
  morkNode* node = *ioSlot;
  if ( me != node )
  {
    if ( node )
    {
      *ioSlot = 0;
      node->CutWeakRef(ev);
    }
    if ( me && me->AddWeakRef(ev) )
      *ioSlot = me;
  }
}

 mork_uses
morkNode::AddStrongRef(morkEnv* ev)
{
  mork_uses outUses = 0;
  if ( this )
  {
    if ( this->IsNode() )
    {
      mork_uses uses = mNode_Uses;
      mork_refs refs = mNode_Refs;
      if ( refs < uses ) 
      { 
        this->RefsUnderUsesWarning(ev);
        mNode_Refs = mNode_Uses = refs = uses;
      }
      if ( refs < morkNode_kMaxRefCount ) 
      {
        mNode_Refs = ++refs;
        mNode_Uses = ++uses;
      }
      else
        this->RefsOverflowWarning(ev);

      outUses = uses;
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
  return outUses;
}

 mork_bool
morkNode::cut_use_count(morkEnv* ev) 
{
  mork_bool didCut = morkBool_kFalse;
  if ( this )
  {
    if ( this->IsNode() )
    {
      mork_uses uses = mNode_Uses;
      if ( uses ) 
        mNode_Uses = --uses;
      else
        this->UsesUnderflowWarning(ev);

      didCut = morkBool_kTrue;
      if ( !mNode_Uses ) 
      {
        if ( this->IsOpenNode() )
        {
          if ( !mNode_Refs ) 
          {
            this->RefsUnderflowWarning(ev);
            ++mNode_Refs; 
          }
          this->CloseMorkNode(ev); 
          
        }
      }
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
  return didCut;
}

 mork_uses
morkNode::CutStrongRef(morkEnv* ev)
{
  mork_refs outRefs = 0;
  if ( this )
  {
    if ( this->IsNode() )
    {
      if ( this->cut_use_count(ev) )
        outRefs = this->CutWeakRef(ev);
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
  return outRefs;
}

 mork_refs
morkNode::AddWeakRef(morkEnv* ev)
{
  mork_refs outRefs = 0;
  if ( this )
  {
    if ( this->IsNode() )
    {
      mork_refs refs = mNode_Refs;
      if ( refs < morkNode_kMaxRefCount ) 
        mNode_Refs = ++refs;
      else
        this->RefsOverflowWarning(ev);
        
      outRefs = refs;
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
  return outRefs;
}

 mork_refs
morkNode::CutWeakRef(morkEnv* ev)
{
  mork_refs outRefs = 0;
  if ( this )
  {
    if ( this->IsNode() )
    {
      mork_uses uses = mNode_Uses;
      mork_refs refs = mNode_Refs;
      if ( refs ) 
        mNode_Refs = --refs;
      else
        this->RefsUnderflowWarning(ev);

      if ( refs < uses ) 
      { 
        this->RefsUnderUsesWarning(ev);
        mNode_Refs = mNode_Uses = refs = uses;
      }
        
      outRefs = refs;
      if ( !refs ) 
        this->ZapOld(ev, mNode_Heap); 
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
  return outRefs;
}

static const char morkNode_kBroken[] = "broken";

 const char*
morkNode::GetNodeAccessAsString() const 
{
  const char* outString = morkNode_kBroken;
  switch( mNode_Access )
  {
    case morkAccess_kOpen: outString = "open"; break;
    case morkAccess_kClosing: outString = "closing"; break;
    case morkAccess_kShut: outString = "shut"; break;
    case morkAccess_kDead: outString = "dead"; break;
  }
  return outString;
}

 const char*
morkNode::GetNodeUsageAsString() const 
{
  const char* outString = morkNode_kBroken;
  switch( mNode_Usage )
  {
    case morkUsage_kHeap: outString = "heap"; break;
    case morkUsage_kStack: outString = "stack"; break;
    case morkUsage_kMember: outString = "member"; break;
    case morkUsage_kGlobal: outString = "global"; break;
    case morkUsage_kPool: outString = "pool"; break;
    case morkUsage_kNone: outString = "none"; break;
  }
  return outString;
}


