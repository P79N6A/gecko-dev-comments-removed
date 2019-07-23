


























#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




 
morkNext::morkNext() : mNext_Link( 0 )
{
}

 void*
morkNext::MakeNewNext(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev)
{
  void* next = 0;
  if ( &ioHeap )
  {
    ioHeap.Alloc(ev->AsMdbEnv(), inSize, (void**) &next);
    if ( !next )
      ev->OutOfMemoryError();
  }
  else
    ev->NilPointerError();
  
  return next;
}


void morkNext::ZapOldNext(morkEnv* ev, nsIMdbHeap* ioHeap)
{
  if ( ioHeap )
  {
    if ( this )
      ioHeap->Free(ev->AsMdbEnv(), this);
  }
  else
    ev->NilPointerError();
}





morkList::morkList() : mList_Head( 0 ), mList_Tail( 0 )
{
}

void morkList::CutAndZapAllListMembers(morkEnv* ev, nsIMdbHeap* ioHeap)

{
  if ( ioHeap )
  {
    morkNext* next = 0;
    while ( (next = this->PopHead()) != 0 )
      next->ZapOldNext(ev, ioHeap);
      
    mList_Head = 0;
    mList_Tail = 0;
  }
  else
    ev->NilPointerError();
}

void morkList::CutAllListMembers()

{
  while ( this->PopHead() )
    ;

  mList_Head = 0;
  mList_Tail = 0;
}

morkNext* morkList::PopHead() 
{
  morkNext* outHead = mList_Head;
  if ( outHead ) 
  {
    morkNext* next = outHead->mNext_Link;
    mList_Head = next;
    if ( !next ) 
      mList_Tail = 0;
      
    outHead->mNext_Link = 0; 
  }
  return outHead;
}


void morkList::PushHead(morkNext* ioLink) 
{
  morkNext* head = mList_Head; 
  morkNext* tail = mList_Tail; 
  
  MORK_ASSERT( (head && tail) || (!head && !tail));
  
  ioLink->mNext_Link = head; 
  if ( !head ) 
    mList_Tail = ioLink; 

  mList_Head = ioLink; 
}

void morkList::PushTail(morkNext* ioLink) 
{
  morkNext* head = mList_Head; 
  morkNext* tail = mList_Tail; 
  
  MORK_ASSERT( (head && tail) || (!head && !tail));
  
  ioLink->mNext_Link = 0; 
  if ( tail ) 
  {
	  tail->mNext_Link = ioLink;
	  mList_Tail = ioLink;
  }
  else 
	  mList_Head = mList_Tail = ioLink; 
}




 
morkLink::morkLink() : mLink_Next( 0 ), mLink_Prev( 0 )
{
}

 void*
morkLink::MakeNewLink(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev)
{
  void* alink = 0;
  if ( &ioHeap )
  {
    ioHeap.Alloc(ev->AsMdbEnv(), inSize, (void**) &alink);
    if ( !alink )
      ev->OutOfMemoryError();
  }
  else
    ev->NilPointerError();
  
  return alink;
}


void morkLink::ZapOldLink(morkEnv* ev, nsIMdbHeap* ioHeap)
{
  if ( ioHeap )
  {
    if ( this )
      ioHeap->Free(ev->AsMdbEnv(), this);
  }
  else
    ev->NilPointerError();
}
  




morkDeque::morkDeque()
{
  mDeque_Head.SelfRefer();
}




morkLink*
morkDeque::RemoveFirst() 
{
  morkLink* alink = mDeque_Head.mLink_Next;
  if ( alink != &mDeque_Head )
  {
    (mDeque_Head.mLink_Next = alink->mLink_Next)->mLink_Prev = 
      &mDeque_Head;
    return alink;
  }
  return (morkLink*) 0;
}



morkLink*
morkDeque::RemoveLast() 
{
  morkLink* alink = mDeque_Head.mLink_Prev;
  if ( alink != &mDeque_Head )
  {
    (mDeque_Head.mLink_Prev = alink->mLink_Prev)->mLink_Next = 
      &mDeque_Head;
    return alink;
  }
  return (morkLink*) 0;
}



morkLink*
morkDeque::At(mork_pos index) const 
  
{ 
  register mork_num count = 0;
  register morkLink* alink;
  for ( alink = this->First(); alink; alink = this->After(alink) )
  {
    if ( ++count == (mork_num) index )
      break;
  }
  return alink;
}



mork_pos
morkDeque::IndexOf(const morkLink* member) const 
  
  
{ 
  register mork_num count = 0;
  register const morkLink* alink;
  for ( alink = this->First(); alink; alink = this->After(alink) )
  {
    ++count;
    if ( member == alink )
      return (mork_pos) count;
  }
  return 0;
}



mork_num
morkDeque::Length() const 
{ 
  register mork_num count = 0;
  register morkLink* alink;
  for ( alink = this->First(); alink; alink = this->After(alink) )
    ++count;
  return count;
}



int
morkDeque::LengthCompare(mork_num c) const 
{ 
  register mork_num count = 0;
  register const morkLink* alink;
  for ( alink = this->First(); alink; alink = this->After(alink) )
  {
    if ( ++count > c )
      return 1;
  }
  return ( count == c )? 0 : -1;
}
