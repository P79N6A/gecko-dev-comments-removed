




































#ifndef _MORKPOOL_
#define _MORKPOOL_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif



class morkHandle;
class morkHandleFrame;
class morkHandleFace; 
class morkBigBookAtom;
class morkFarBookAtom;

#define morkDerived_kPool 0x706C /* ascii 'pl' */





class morkPool : public morkNode {
  

  

  
  
  
  
  
  
  
  
  
  

public: 
  nsIMdbHeap*  mPool_Heap; 
  
  morkDeque    mPool_Blocks;      
  
  
  morkDeque    mPool_UsedHandleFrames; 
  morkDeque    mPool_FreeHandleFrames; 
  
  mork_count   mPool_UsedFramesCount; 
  mork_count   mPool_FreeFramesCount; 
    

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkPool(); 
  
public: 
  morkPool(const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    nsIMdbHeap* ioSlotHeap);
  morkPool(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    nsIMdbHeap* ioSlotHeap);
  void ClosePool(morkEnv* ev); 

private: 
  morkPool(const morkPool& other);
  morkPool& operator=(const morkPool& other);

public: 
  mork_bool IsPool() const
  { return IsNode() && mNode_Derived == morkDerived_kPool; }


public: 
  void NonPoolTypeError(morkEnv* ev);

public: 
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkNode::MakeNew(inSize, ioHeap, ev); }
  
  void* operator new(size_t inSize) CPP_THROW_NEW
  { return ::operator new(inSize); }
  

public: 

  
  morkHandleFace*  NewHandle(morkEnv* ev, mork_size inSize, morkZone* ioZone);
  void             ZapHandle(morkEnv* ev, morkHandleFace* ioHandle);

  
  morkRow*  NewRow(morkEnv* ev, morkZone* ioZone); 
  void      ZapRow(morkEnv* ev, morkRow* ioRow, morkZone* ioZone); 

  
  morkCell* NewCells(morkEnv* ev, mork_size inSize, morkZone* ioZone);
  void      ZapCells(morkEnv* ev, morkCell* ioVector, mork_size inSize, morkZone* ioZone);
  
  
  mork_bool AddRowCells(morkEnv* ev, morkRow* ioRow, mork_size inNewSize, morkZone* ioZone);
  mork_bool CutRowCells(morkEnv* ev, morkRow* ioRow, mork_size inNewSize, morkZone* ioZone);
  
  
  void ZapAtom(morkEnv* ev, morkAtom* ioAtom, morkZone* ioZone); 
  
  morkOidAtom* NewRowOidAtom(morkEnv* ev, const mdbOid& inOid, morkZone* ioZone); 
  morkOidAtom* NewTableOidAtom(morkEnv* ev, const mdbOid& inOid, morkZone* ioZone);
  
  morkAtom* NewAnonAtom(morkEnv* ev, const morkBuf& inBuf,
    mork_cscode inForm, morkZone* ioZone);
    
    
    
  morkBookAtom* NewBookAtom(morkEnv* ev, const morkBuf& inBuf,
    mork_cscode inForm, morkAtomSpace* ioSpace, mork_aid inAid, morkZone* ioZone);
    
    
    
  morkBookAtom* NewBookAtomCopy(morkEnv* ev, const morkBigBookAtom& inAtom, morkZone* ioZone);
    
    
    
    
  morkBookAtom* NewFarBookAtomCopy(morkEnv* ev, const morkFarBookAtom& inAtom, morkZone* ioZone);
    
    
    

public: 
  static void SlotWeakPool(morkPool* me,
    morkEnv* ev, morkPool** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongPool(morkPool* me,
    morkEnv* ev, morkPool** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 

