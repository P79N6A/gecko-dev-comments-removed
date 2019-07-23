




































#include "nscore.h"

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

#ifndef _MORKARRAY_
#include "morkArray.h"
#endif






 void
morkArray::CloseMorkNode(morkEnv* ev) 
{
  if ( this->IsOpenNode() )
  {
    this->MarkClosing();
    this->CloseArray(ev);
    this->MarkShut();
  }
}


morkArray::~morkArray() 
{
  MORK_ASSERT(this->IsShutNode());
  MORK_ASSERT(mArray_Slots==0);
}


morkArray::morkArray(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, mork_size inSize, nsIMdbHeap* ioSlotHeap)
: morkNode(ev, inUsage, ioHeap)
, mArray_Slots( 0 )
, mArray_Heap( 0 )
, mArray_Fill( 0 )
, mArray_Size( 0 )
, mArray_Seed( (mork_u4)NS_PTR_TO_INT32(this) ) 
{
  if ( ev->Good() )
  {
    if ( ioSlotHeap )
    {
      nsIMdbHeap_SlotStrongHeap(ioSlotHeap, ev, &mArray_Heap);
      if ( ev->Good() )
      {
        if ( inSize < 3 )
          inSize = 3;
        mdb_size byteSize = inSize * sizeof(void*);
        void** block = 0;
        ioSlotHeap->Alloc(ev->AsMdbEnv(), byteSize, (void**) &block);
        if ( block && ev->Good() )
        {
          mArray_Slots = block;
          mArray_Size = inSize;
          MORK_MEMSET(mArray_Slots, 0, byteSize);
          if ( ev->Good() )
            mNode_Derived = morkDerived_kArray;
        }
      }
    }
    else
      ev->NilPointerError();
  }
}

 void
morkArray::CloseArray(morkEnv* ev) 
{
  if ( this )
  {
    if ( this->IsNode() )
    {
      if ( mArray_Heap && mArray_Slots )
        mArray_Heap->Free(ev->AsMdbEnv(), mArray_Slots);
        
      mArray_Slots = 0;
      mArray_Size = 0;
      mArray_Fill = 0;
      ++mArray_Seed;
      nsIMdbHeap_SlotStrongHeap((nsIMdbHeap*) 0, ev, &mArray_Heap);
      this->MarkShut();
    }
    else
      this->NonNodeError(ev);
  }
  else
    ev->NilPointerError();
}




 void
morkArray::NonArrayTypeError(morkEnv* ev)
{
  ev->NewError("non morkArray");
}

 void
morkArray::IndexBeyondEndError(morkEnv* ev)
{
  ev->NewError("array index beyond end");
}

 void
morkArray::NilSlotsAddressError(morkEnv* ev)
{
  ev->NewError("nil mArray_Slots");
}

 void
morkArray::FillBeyondSizeError(morkEnv* ev)
{
  ev->NewError("mArray_Fill > mArray_Size");
}

mork_bool
morkArray::Grow(morkEnv* ev, mork_size inNewSize)

{
  if ( ev->Good() && inNewSize > mArray_Size ) 
  {
    if ( mArray_Fill <= mArray_Size ) 
    {
      if (mArray_Size <= 3)
        inNewSize = mArray_Size + 3;
      else
        inNewSize = mArray_Size  * 2;
        
      mdb_size newByteSize = inNewSize * sizeof(void*);
      void** newBlock = 0;
      mArray_Heap->Alloc(ev->AsMdbEnv(), newByteSize, (void**) &newBlock);
      if ( newBlock && ev->Good() ) 
      {
        void** oldSlots = mArray_Slots;
        void** oldEnd = oldSlots + mArray_Fill;
        
        void** newSlots = newBlock;
        void** newEnd = newBlock + inNewSize;
        
        while ( oldSlots < oldEnd )
          *newSlots++ = *oldSlots++;
          
        while ( newSlots < newEnd )
          *newSlots++ = (void*) 0;

        oldSlots = mArray_Slots;
        mArray_Size = inNewSize;
        mArray_Slots = newBlock;
        mArray_Heap->Free(ev->AsMdbEnv(), oldSlots);
      }
    }
    else
      this->FillBeyondSizeError(ev);
  }
  ++mArray_Seed; 
  return ( ev->Good() && mArray_Size >= inNewSize );
}

void*
morkArray::SafeAt(morkEnv* ev, mork_pos inPos)
{
  if ( mArray_Slots )
  {
    if ( inPos >= 0 && inPos < (mork_pos) mArray_Fill )
      return mArray_Slots[ inPos ];
    else
      this->IndexBeyondEndError(ev);
  }
  else
    this->NilSlotsAddressError(ev);
    
  return (void*) 0;
}

void
morkArray::SafeAtPut(morkEnv* ev, mork_pos inPos, void* ioSlot)
{
  if ( mArray_Slots )
  {
    if ( inPos >= 0 && inPos < (mork_pos) mArray_Fill )
    {
      mArray_Slots[ inPos ] = ioSlot;
      ++mArray_Seed;
    }
    else
      this->IndexBeyondEndError(ev);
  }
  else
    this->NilSlotsAddressError(ev);
}

mork_pos
morkArray::AppendSlot(morkEnv* ev, void* ioSlot)
{
  mork_pos outPos = -1;
  if ( mArray_Slots )
  {
    mork_fill fill = mArray_Fill;
    if ( this->Grow(ev, fill+1) )
    {
      outPos = (mork_pos) fill;
      mArray_Slots[ fill ] = ioSlot;
      mArray_Fill = fill + 1;
      
    }
  }
  else
    this->NilSlotsAddressError(ev);
    
  return outPos;
}

void
morkArray::AddSlot(morkEnv* ev, mork_pos inPos, void* ioSlot)
{
  if ( mArray_Slots )
  {
    mork_fill fill = mArray_Fill;
    if ( this->Grow(ev, fill+1) )
    {
      void** slot = mArray_Slots; 
      void** end = slot + fill; 
      slot += inPos; 

      while ( --end >= slot ) 
        end[ 1 ] = *end;

      *slot = ioSlot;
      mArray_Fill = fill + 1;
      
    }
  }
  else
    this->NilSlotsAddressError(ev);
}

void
morkArray::CutSlot(morkEnv* ev, mork_pos inPos)
{
  MORK_USED_1(ev);
  mork_fill fill = mArray_Fill;
  if ( inPos >= 0 && inPos < (mork_pos) fill ) 
  {
    void** slot = mArray_Slots; 
    void** end = slot + fill; 
    slot += inPos; 
    
    while ( ++slot < end ) 
      slot[ -1 ] = *slot;
      
    slot[ -1 ] = 0; 
    
    
    mArray_Fill = fill - 1;
    ++mArray_Seed;
  }
}

void
morkArray::CutAllSlots(morkEnv* ev)
{
  if ( mArray_Slots )
  {
    if ( mArray_Fill <= mArray_Size )
    {
      mdb_size oldByteSize = mArray_Fill * sizeof(void*);
      MORK_MEMSET(mArray_Slots, 0, oldByteSize);
    }
    else
      this->FillBeyondSizeError(ev);
  }
  else
    this->NilSlotsAddressError(ev);

  ++mArray_Seed;
  mArray_Fill = 0;
}


