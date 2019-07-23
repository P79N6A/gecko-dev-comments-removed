




































#ifndef _MORKBEAD_
#define _MORKBEAD_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif

#ifndef _MORKPROBEMAP_
#include "morkProbeMap.h"
#endif



#define morkDerived_kBead 0x426F /* ascii 'Bo' */





class morkBead : public morkNode { 


  

  
  
  
  
  
  
  
  
  
  
  
public: 

  mork_color      mBead_Color;   

public: 

  mork_u4 BeadHash() const { return (mork_u4) mBead_Color; }
  mork_bool BeadEqual(const morkBead* inBead) const
  { return ( mBead_Color == inBead->mBead_Color); }
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkBead(); 
  
public: 
  morkBead(mork_color inBeadColor); 
  
protected: 
  morkBead(const morkUsage& inUsage, nsIMdbHeap* ioHeap,
    mork_color inBeadColor);
  
public: 
  morkBead(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap, 
     mork_color inBeadColor);
  void CloseBead(morkEnv* ev); 

private: 
  morkBead(const morkBead& other);
  morkBead& operator=(const morkBead& other);

public: 
  mork_bool IsBead() const
  { return IsNode() && mNode_Derived == morkDerived_kBead; }


  
  
public: 
  static void SlotWeakBead(morkBead* me,
    morkEnv* ev, morkBead** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongBead(morkBead* me,
    morkEnv* ev, morkBead** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#define morkDerived_kBeadMap 0x744D /* ascii 'bM' */



class morkBeadMap : public morkMap {



public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkBeadMap(); 
  
public: 
  morkBeadMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void CloseBeadMap(morkEnv* ev); 

public: 
  mork_bool IsBeadMap() const
  { return IsNode() && mNode_Derived == morkDerived_kBeadMap; }



public:
  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const;

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const;


public: 

  mork_bool  AddBead(morkEnv* ev, morkBead* ioBead);
  

  mork_bool  CutBead(morkEnv* ev, mork_color inColor);
  
  
  morkBead*  GetBead(morkEnv* ev, mork_color inColor);
  

  mork_num CutAllBeads(morkEnv* ev);
  
};

class morkBeadMapIter: public morkMapIter{ 

public:
  morkBeadMapIter(morkEnv* ev, morkBeadMap* ioMap)
  : morkMapIter(ev, ioMap) { }
 
  morkBeadMapIter( ) : morkMapIter()  { }
  void InitBeadMapIter(morkEnv* ev, morkBeadMap* ioMap)
  { this->InitMapIter(ev, ioMap); }
   
  morkBead* FirstBead(morkEnv* ev);
  morkBead* NextBead(morkEnv* ev);
  morkBead* HereBead(morkEnv* ev);
  void      CutHereBead(morkEnv* ev);
  
};



#define morkDerived_kBeadProbeMap 0x6D74 /* ascii 'mb' */



class morkBeadProbeMap : public morkProbeMap {



public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkBeadProbeMap(); 
  
public: 
  morkBeadProbeMap(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap);
  void CloseBeadProbeMap(morkEnv* ev); 

public: 
  mork_bool IsBeadProbeMap() const
  { return IsNode() && mNode_Derived == morkDerived_kBeadProbeMap; }


  
public:
  virtual mork_test 
  MapTest(morkEnv* ev, const void* inMapKey, const void* inAppKey) const;

  virtual mork_u4 
  MapHash(morkEnv* ev, const void* inAppKey) const;

  virtual mork_u4 ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const;

  

  
  

  
  
  

  
  
  
  

public: 

  mork_bool  AddBead(morkEnv* ev, morkBead* ioBead);
  
  
  morkBead*  GetBead(morkEnv* ev, mork_color inColor);
  

  mork_num   CutAllBeads(morkEnv* ev);
  
};

class morkBeadProbeMapIter: public morkProbeMapIter { 

public:
  morkBeadProbeMapIter(morkEnv* ev, morkBeadProbeMap* ioMap)
  : morkProbeMapIter(ev, ioMap) { }
 
  morkBeadProbeMapIter( ) : morkProbeMapIter()  { }
  void InitBeadProbeMapIter(morkEnv* ev, morkBeadProbeMap* ioMap)
  { this->InitProbeMapIter(ev, ioMap); }
   
  morkBead* FirstBead(morkEnv* ev)
  { return (morkBead*) this->IterFirstKey(ev); }
  
  morkBead* NextBead(morkEnv* ev)
  { return (morkBead*) this->IterNextKey(ev); }
  
  morkBead* HereBead(morkEnv* ev)
  { return (morkBead*) this->IterHereKey(ev); }
  
};



#endif 
