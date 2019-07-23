




































#ifndef _MORKMAP_
#define _MORKMAP_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif







typedef mork_bool (* morkMap_mEqual)
(const morkMap* self, morkEnv* ev, const void* inKeyA, const void* inKeyB);



typedef mork_u4 (* morkMap_mHash)
(const morkMap* self, morkEnv* ev, const void* inKey);



typedef mork_bool (* morkMap_mIsNil)
(const morkMap* self, morkEnv* ev, const void* inKey);









class morkMapForm { 
public:
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  mork_size       mMapForm_KeySize; 
  mork_size       mMapForm_ValSize; 
  
  mork_bool       mMapForm_HoldChanges; 
  mork_change     mMapForm_DummyChange; 
  mork_bool       mMapForm_KeyIsIP;     
  mork_bool       mMapForm_ValIsIP;     
};




















class morkAssoc {
public:
  morkAssoc*   mAssoc_Next;
};


#define morkDerived_kMap 0x4D70 /* ascii 'Mp' */

#define morkMap_kTag 0x6D4D6150 /* ascii 'mMaP' */




class morkMap : public morkNode {


  

  
  
  
  
  
  
  
  
  
  

public: 

  nsIMdbHeap*       mMap_Heap; 
  mork_u4           mMap_Tag; 

  
  
  
  morkMapForm       mMap_Form; 
  
  
  
  
  mork_seed         mMap_Seed;   
  
  
  
  
  
  
  mork_count        mMap_Slots;  
  mork_fill         mMap_Fill;   
  
  
  
  
  
  
  mork_u1*          mMap_Keys;  
  mork_u1*          mMap_Vals;  

  
  
  
  
  
  
  
  morkAssoc*        mMap_Assocs;   
  
  

  mork_change*      mMap_Changes;  
  
  
  
  
  
  
  morkAssoc**       mMap_Buckets;  
  
  
  
  
  
  
  
  morkAssoc*        mMap_FreeList; 

public: 
  mork_fill        MapFill() const { return mMap_Fill; }
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkMap(); 
  
public: 
  morkMap(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioNodeHeap, 
    mork_size inKeySize, mork_size inValSize,
    mork_size inSlots, nsIMdbHeap* ioSlotHeap, mork_bool inHoldChanges);
  
  void CloseMap(morkEnv* ev); 
  
public: 
  mork_bool IsMap() const
  { return IsNode() && mNode_Derived == morkDerived_kMap; }


public: 


  virtual mork_bool 
  Equal(morkEnv* ev, const void* inKeyA, const void* inKeyB) const = 0;

  virtual mork_u4 
  Hash(morkEnv* ev, const void* inKey) const = 0;


public: 

  mork_bool GoodMapTag() const { return mMap_Tag == morkMap_kTag; }
  mork_bool GoodMap() const
  { return ( IsNode() && GoodMapTag() ); }
  
  void NewIterOutOfSyncError(morkEnv* ev);
  void NewBadMapError(morkEnv* ev);
  void NewSlotsUnderflowWarning(morkEnv* ev);
  void InitMap(morkEnv* ev, mork_size inSlots);

protected: 

  friend class morkMapIter;
  void clear_map(morkEnv* ev, nsIMdbHeap* ioHeap);

  void* alloc(morkEnv* ev, mork_size inSize);
  void* clear_alloc(morkEnv* ev, mork_size inSize);
  
  void push_free_assoc(morkAssoc* ioAssoc)
  {
    ioAssoc->mAssoc_Next = mMap_FreeList;
    mMap_FreeList = ioAssoc;
  }
  
  morkAssoc* pop_free_assoc()
  {
    morkAssoc* assoc = mMap_FreeList;
    if ( assoc )
      mMap_FreeList = assoc->mAssoc_Next;
    return assoc;
  }

  morkAssoc** find(morkEnv* ev, const void* inKey, mork_u4 inHash) const;
  
  mork_u1* new_keys(morkEnv* ev, mork_num inSlots);
  mork_u1* new_values(morkEnv* ev, mork_num inSlots);
  mork_change* new_changes(morkEnv* ev, mork_num inSlots);
  morkAssoc** new_buckets(morkEnv* ev, mork_num inSlots);
  morkAssoc* new_assocs(morkEnv* ev, mork_num inSlots);
  mork_bool new_arrays(morkEnv* ev, morkHashArrays* old, mork_num inSlots);
  
  mork_bool grow(morkEnv* ev);

  void get_assoc(void* outKey, void* outVal, mork_pos inPos) const;
  void put_assoc(const void* inKey, const void* inVal, mork_pos inPos) const;
  
public: 
  
  
  
  
  
    
  
  
  
  
  
  mork_size       FormKeySize() const { return mMap_Form.mMapForm_KeySize; }
  mork_size       FormValSize() const { return mMap_Form.mMapForm_ValSize; }

  mork_bool       FormKeyIsIP() const { return mMap_Form.mMapForm_KeyIsIP; }
  mork_bool       FormValIsIP() const { return mMap_Form.mMapForm_ValIsIP; }

  mork_bool       FormHoldChanges() const
  { return mMap_Form.mMapForm_HoldChanges; }
  
  mork_change*    FormDummyChange()
  { return &mMap_Form.mMapForm_DummyChange; }

public: 
 
  mork_bool Put(morkEnv* ev, const void* inKey, const void* inVal,
    void* outKey, void* outVal, mork_change** outChange);
    
  mork_bool Cut(morkEnv* ev, const void* inKey,
    void* outKey, void* outVal, mork_change** outChange);
    
  mork_bool Get(morkEnv* ev, const void* inKey, 
    void* outKey, void* outVal, mork_change** outChange);
    
  mork_num CutAll(morkEnv* ev);

private: 
  morkMap(const morkMap& other);
  morkMap& operator=(const morkMap& other);


public: 
  static void SlotWeakMap(morkMap* me,
    morkEnv* ev, morkMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongMap(morkMap* me,
    morkEnv* ev, morkMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};






class morkMapIter{  

protected:
  morkMap*    mMapIter_Map;      
  mork_seed   mMapIter_Seed;     
  
  morkAssoc** mMapIter_Bucket;   
  morkAssoc** mMapIter_AssocRef; 
  morkAssoc*  mMapIter_Assoc;    
  morkAssoc*  mMapIter_Next;     

public:
  morkMapIter(morkEnv* ev, morkMap* ioMap);
  void CloseMapIter(morkEnv* ev);
 
  morkMapIter( ); 

protected: 

  void InitMapIter(morkEnv* ev, morkMap* ioMap);
 
  
  
  
  
  
  mork_change* First(morkEnv* ev, void* outKey, void* outVal);
  mork_change* Next(morkEnv* ev, void* outKey, void* outVal);
  mork_change* Here(morkEnv* ev, void* outKey, void* outVal);
  
  mork_change* CutHere(morkEnv* ev, void* outKey, void* outVal);
};



#endif 
