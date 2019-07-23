




































#ifndef _MORKNODE_
#define _MORKNODE_ 1

#ifndef _MORK_
#include "mork.h"
#endif



#define morkUsage_kHeap    'h'
#define morkUsage_kStack   's'
#define morkUsage_kMember  'm'
#define morkUsage_kGlobal  'g'
#define morkUsage_kPool    'p'
#define morkUsage_kNone    'n'

class morkUsage {
public:
  mork_usage     mUsage_Code;  
  
public:
  morkUsage(mork_usage inCode);

  morkUsage(); 
  void InitUsage( mork_usage inCode)
  { mUsage_Code = inCode; }
  
  ~morkUsage() { }
  mork_usage Code() const { return mUsage_Code; }
  
  static void EnsureReadyStaticUsage();
  
public:
  static const morkUsage& kHeap;      
  static const morkUsage& kStack;     
  static const morkUsage& kMember;    
  static const morkUsage& kGlobal;    
  static const morkUsage& kPool;      
  static const morkUsage& kNone;      
  
  static const morkUsage& GetHeap();   
  static const morkUsage& GetStack();  
  static const morkUsage& GetMember(); 
  static const morkUsage& GetGlobal(); 
  static const morkUsage& GetPool();   
  static const morkUsage& GetNone();   
    
};



#define morkNode_kMaxRefCount 0x0FFFF /* count sticks if it hits this */

#define morkBase_kNode 0x4E64 /* ascii 'Nd' */




















class morkNode  { 

public: 
  

  nsIMdbHeap*    mNode_Heap;     

  mork_base      mNode_Base;     
  mork_derived   mNode_Derived;  
  
  mork_access    mNode_Access;   
  mork_usage     mNode_Usage;    
  mork_able      mNode_Mutable;  
  mork_load      mNode_Load;     
  
  mork_uses      mNode_Uses;     
  mork_refs      mNode_Refs;     
  
protected: 
  friend class morkHandleFrame;
  morkNode() { }
  
public: 

  void SetFrozen() { mNode_Mutable = morkAble_kDisabled; }
  void SetMutable() { mNode_Mutable = morkAble_kEnabled; }
  void SetAsleep() { mNode_Mutable = morkAble_kAsleep; }
  
  mork_bool IsFrozen() const { return mNode_Mutable == morkAble_kDisabled; }
  mork_bool IsMutable() const { return mNode_Mutable == morkAble_kEnabled; }
  mork_bool IsAsleep() const { return mNode_Mutable == morkAble_kAsleep; }

  void SetNodeClean() { mNode_Load = morkLoad_kClean; }
  void SetNodeDirty() { mNode_Load = morkLoad_kDirty; }
  
  mork_bool IsNodeClean() const { return mNode_Load == morkLoad_kClean; }
  mork_bool IsNodeDirty() const { return mNode_Load == morkLoad_kDirty; }
  
public: 
  static void* MakeNew(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev);
  
  void ZapOld(morkEnv* ev, nsIMdbHeap* ioHeap); 
  
  
  

public: 
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkNode::MakeNew(inSize, ioHeap, ev); }
  

protected: 
  morkNode(const morkUsage& inUsage, nsIMdbHeap* ioHeap);

  morkNode(mork_usage inCode); 


public: 
  
  
  
  virtual ~morkNode(); 
  virtual void CloseMorkNode(morkEnv* ev); 
  
  
  
  
  
  
  
  
public: 
  morkNode(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap);
  void CloseNode(morkEnv* ev); 
  mdb_err CloseMdbObject(morkEnv *ev);
  NS_IMETHOD CloseMdbObject(nsIMdbEnv *ev);
private: 
  morkNode(const morkNode& other);
  morkNode& operator=(const morkNode& other);

public: 
  mork_bool IsNode() const
  { return mNode_Base == morkBase_kNode; }

    
public: 

  void RefsUnderUsesWarning(morkEnv* ev) const; 
  void NonNodeError(morkEnv* ev) const; 
  void NilHeapError(morkEnv* ev) const; 
  void NonOpenNodeError(morkEnv* ev) const; 

  void NonMutableNodeError(morkEnv* ev) const; 

  void RefsOverflowWarning(morkEnv* ev) const; 
  void UsesOverflowWarning(morkEnv* ev) const; 
  void RefsUnderflowWarning(morkEnv* ev) const; 
  void UsesUnderflowWarning(morkEnv* ev) const; 

private: 
  mork_bool  cut_use_count(morkEnv* ev); 

public: 

  mork_bool  GoodRefs() const { return mNode_Refs >= mNode_Uses; }
  mork_bool  BadRefs() const { return mNode_Refs < mNode_Uses; }

  mork_uses  StrongRefsOnly() const { return mNode_Uses; }
  mork_refs  WeakRefsOnly() const { return (mork_refs) ( mNode_Refs - mNode_Uses ); }

  
  virtual mork_refs    AddStrongRef(morkEnv* ev);
  virtual mork_refs    CutStrongRef(morkEnv* ev);
  mork_refs    AddWeakRef(morkEnv* ev);
  mork_refs    CutWeakRef(morkEnv* ev);

  const char* GetNodeAccessAsString() const; 
  const char* GetNodeUsageAsString() const; 

  mork_usage NodeUsage() const { return mNode_Usage; }

  mork_bool IsHeapNode() const
  { return mNode_Usage == morkUsage_kHeap; }

  mork_bool IsOpenNode() const
  { return mNode_Access == morkAccess_kOpen; }

  mork_bool IsShutNode() const
  { return mNode_Access == morkAccess_kShut; }

  mork_bool IsDeadNode() const
  { return mNode_Access == morkAccess_kDead; }

  mork_bool IsClosingNode() const
  { return mNode_Access == morkAccess_kClosing; }

  mork_bool IsOpenOrClosingNode() const
  { return IsOpenNode() || IsClosingNode(); }

  mork_bool HasNodeAccess() const
  { return ( IsOpenNode() || IsShutNode() || IsClosingNode() ); }
    
  void MarkShut() { mNode_Access = morkAccess_kShut; }
  void MarkClosing() { mNode_Access = morkAccess_kClosing; }
  void MarkDead() { mNode_Access = morkAccess_kDead; }

public: 
  static void SlotWeakNode(morkNode* me, morkEnv* ev, morkNode** ioSlot);
  
  
  
  
  
  
  static void SlotStrongNode(morkNode* me, morkEnv* ev, morkNode** ioSlot);
  
  
  
  
  
};

extern void 
nsIMdbHeap_SlotStrongHeap(nsIMdbHeap* self, morkEnv* ev, nsIMdbHeap** ioSlot);
  
  
  
  
  

extern void 
nsIMdbFile_SlotStrongFile(nsIMdbFile* self, morkEnv* ev, nsIMdbFile** ioSlot);
  
  
  
  
  

extern void 
nsIMdbCompare_SlotStrongCompare(nsIMdbCompare* self, morkEnv* ev,
  nsIMdbCompare** ioSlot);
  
  
  
  
  



#endif 
