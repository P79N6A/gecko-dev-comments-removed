


























#ifndef _MORKDEQUE_
#define _MORKDEQUE_ 1

#ifndef _MORK_
#include "mork.h"
#endif





class morkNext  {  
public:
  morkNext*  mNext_Link;
  
public:
  morkNext(int inZero) : mNext_Link( 0 ) { }
  
  morkNext(morkNext* ioLink) : mNext_Link( ioLink ) { }
  
  morkNext(); 
  
public:
  morkNext*  GetNextLink() const { return mNext_Link; }
  
public: 
  static void* MakeNewNext(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev);
  void ZapOldNext(morkEnv* ev, nsIMdbHeap* ioHeap);

public: 
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkNext::MakeNewNext(inSize, ioHeap, ev); }
  
  void operator delete(void* ioAddress) 
  { ((morkNext*) 0)->ZapOldNext((morkEnv*) 0, (nsIMdbHeap*) 0); } 
};

















class morkList   {  
public:
  morkNext*  mList_Head; 
  morkNext*  mList_Tail; 
  
public:
  morkNext*  GetListHead() const { return mList_Head; }
  morkNext*  GetListTail() const { return mList_Tail; }

  mork_bool IsListEmpty() const { return ( mList_Head == 0 ); }
  mork_bool HasListMembers() const { return ( mList_Head != 0 ); }
  
public:
  morkList(); 
  
  void CutAndZapAllListMembers(morkEnv* ev, nsIMdbHeap* ioHeap);
  

  void CutAllListMembers();
  

public:
  morkNext* PopHead(); 
  
  
  
  void PushHead(morkNext* ioLink); 
  void PushTail(morkNext* ioLink); 
};





class morkLink  {  
public:
  morkLink*  mLink_Next;
  morkLink*  mLink_Prev;
  
public:
  morkLink(int inZero) : mLink_Next( 0 ), mLink_Prev( 0 ) { }
  
  morkLink(); 
  
public:
  morkLink*  Next() const { return mLink_Next; }
  morkLink*  Prev() const { return mLink_Prev; }
  
  void SelfRefer() { mLink_Next = mLink_Prev = this; }
  void Clear() { mLink_Next = mLink_Prev = 0; }
  
  void AddBefore(morkLink* old)
  {
    ((old)->mLink_Prev->mLink_Next = (this))->mLink_Prev = (old)->mLink_Prev;
    ((this)->mLink_Next = (old))->mLink_Prev = this;
  }
  
  void AddAfter(morkLink* old)
  {
    ((old)->mLink_Next->mLink_Prev = (this))->mLink_Next = (old)->mLink_Next;
    ((this)->mLink_Prev = (old))->mLink_Next = this;
  }
  
  void Remove()
  {
    (mLink_Prev->mLink_Next = mLink_Next)->mLink_Prev = mLink_Prev;
  }
  
public: 
  static void* MakeNewLink(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev);
  void ZapOldLink(morkEnv* ev, nsIMdbHeap* ioHeap);

public: 
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkLink::MakeNewLink(inSize, ioHeap, ev); }
  
};





class morkDeque  {
public:
  morkLink  mDeque_Head;

public: 
  morkDeque(); 

public:
  morkLink* RemoveFirst();

  morkLink* RemoveLast();

  morkLink* At(mork_pos index) const ; 

  mork_pos IndexOf(const morkLink* inMember) const; 
    

  mork_num Length() const;

  
  int LengthCompare(mork_num inCount) const;
  

public: 

  mork_bool IsEmpty()const 
  { return (mDeque_Head.mLink_Next == (morkLink*) &mDeque_Head); }

  morkLink* After(const morkLink* old) const
  { return (((old)->mLink_Next != &mDeque_Head)?
            (old)->mLink_Next : (morkLink*) 0); }

  morkLink* Before(const morkLink* old) const
  { return (((old)->mLink_Prev != &mDeque_Head)?
            (old)->mLink_Prev : (morkLink*) 0); }

  morkLink*  First() const
  { return ((mDeque_Head.mLink_Next != &mDeque_Head)?
    mDeque_Head.mLink_Next : (morkLink*) 0); }

  morkLink*  Last() const
  { return ((mDeque_Head.mLink_Prev != &mDeque_Head)?
    mDeque_Head.mLink_Prev : (morkLink*) 0); }
    









  void AddFirst(morkLink* in)  
  {
    ( (mDeque_Head.mLink_Next->mLink_Prev = 
      (in))->mLink_Next = mDeque_Head.mLink_Next, 
        ((in)->mLink_Prev = &mDeque_Head)->mLink_Next = (in) );
  }









  void AddLast(morkLink* in)
  {
    ( (mDeque_Head.mLink_Prev->mLink_Next = 
      (in))->mLink_Prev = mDeque_Head.mLink_Prev, 
        ((in)->mLink_Next = &mDeque_Head)->mLink_Prev = (in) );
  }
};

#endif 
