









































#ifndef _MORKPROBEMAP_
#define _MORKPROBEMAP_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif



class morkMapScratch { 
public:
  nsIMdbHeap*  sMapScratch_Heap;     
  mork_count   sMapScratch_Slots;    
  
  mork_u1*     sMapScratch_Keys;     
  mork_u1*     sMapScratch_Vals;     
  
public:
  void halt_map_scratch(morkEnv* ev);
};



#define morkDerived_kProbeMap   0x7072 /* ascii 'pr' */
#define morkProbeMap_kTag     0x70724D50 /* ascii 'prMP' */

#define morkProbeMap_kLazyClearOnAdd ((mork_u1) 'c')
 
class morkProbeMap: public morkNode {

protected:


  

  
  
  
  
  
  
  
  
  
  

protected:
  
  nsIMdbHeap* sMap_Heap; 
    
  mork_u1*    sMap_Keys;
  mork_u1*    sMap_Vals;
  
  mork_count  sMap_Seed;   
    
  mork_count  sMap_Slots;  
  mork_fill   sMap_Fill;   

  mork_size   sMap_KeySize; 
  mork_size   sMap_ValSize; 
  
  mork_bool   sMap_KeyIsIP;     
  mork_bool   sMap_ValIsIP;     
  mork_u1     sMap_Pad[ 2 ];    
  
    
  friend class morkProbeMapIter; 

public: 
  mork_count  MapSeed() const { return sMap_Seed; }
    
  mork_count  MapSlots() const { return sMap_Slots; }
  mork_fill   MapFill() const { return sMap_Fill; }

  mork_size   MapKeySize() const { return sMap_KeySize; }
  mork_size   MapValSize() const { return sMap_ValSize; }
  
  mork_bool   MapKeyIsIP() const { return sMap_KeyIsIP; }
  mork_bool   MapValIsIP() const { return sMap_ValIsIP; }

protected: 
  
   
  mork_fill   sProbeMap_MaxFill; 
  
  mork_u1     sProbeMap_LazyClearOnAdd; 
  mork_bool   sProbeMap_ZeroIsClearKey; 
  mork_u1     sProbeMap_Pad[ 2 ]; 
  
  mork_u4     sProbeMap_Tag; 
 
  
    
public: 

  mork_bool need_lazy_init() const 
  { return sProbeMap_LazyClearOnAdd == morkProbeMap_kLazyClearOnAdd; }

public: 
  mork_bool   GoodProbeMap() const
  { return sProbeMap_Tag == morkProbeMap_kTag; }
    
protected: 

  void* clear_alloc(morkEnv* ev, mork_size inSize);

  mork_u1*    map_new_vals(morkEnv* ev, mork_num inSlots);
  mork_u1*    map_new_keys(morkEnv* ev, mork_num inSlots);

  void clear_probe_map(morkEnv* ev, nsIMdbHeap* ioMapHeap);
  void init_probe_map(morkEnv* ev, mork_size inSlots);
  void probe_map_lazy_init(morkEnv* ev);

  mork_bool new_slots(morkEnv* ev, morkMapScratch* old, mork_num inSlots);
  
  mork_test find_key_pos(morkEnv* ev, const void* inAppKey,
    mork_u4 inHash, mork_pos* outPos) const;
  
  void put_probe_kv(morkEnv* ev,
    const void* inAppKey, const void* inAppVal, mork_pos inPos);
  void get_probe_kv(morkEnv* ev,
    void* outAppKey, void* outAppVal, mork_pos inPos) const;
    
  mork_bool grow_probe_map(morkEnv* ev);
  void      rehash_old_map(morkEnv* ev, morkMapScratch* ioScratch);
  void      revert_map(morkEnv* ev, morkMapScratch* ioScratch);

public: 
  void ProbeMapBadTagError(morkEnv* ev) const;
  void WrapWithNoVoidSlotError(morkEnv* ev) const;
  void GrowFailsMaxFillError(morkEnv* ev) const;
  void MapKeyIsNotIPError(morkEnv* ev) const;
  void MapValIsNotIPError(morkEnv* ev) const;

  void MapNilKeysError(morkEnv* ev);
  void MapZeroKeySizeError(morkEnv* ev);

  void MapSeedOutOfSyncError(morkEnv* ev);
  void MapFillUnderflowWarning(morkEnv* ev);

  static void ProbeMapCutError(morkEnv* ev);

  
public:

  virtual mork_test 
  MapTest(morkEnv* ev, const void* inMapKey, const void* inAppKey) const;
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

  virtual mork_u4 
  MapHash(morkEnv* ev, const void* inAppKey) const;

  virtual mork_bool
  MapAtPut(morkEnv* ev, const void* inAppKey, const void* inAppVal,
    void* outAppKey, void* outAppVal);
    
  virtual mork_bool
  MapAt(morkEnv* ev, const void* inAppKey, void* outAppKey, void* outAppVal);
    
  virtual mork_num
  MapCutAll(morkEnv* ev);
  

    
  
public:

  virtual mork_u4
  ProbeMapHashMapKey(morkEnv* ev, const void* inMapKey) const;
    
    
    
    
    
    
    
    
    
    
    
    

  virtual mork_bool
  ProbeMapIsKeyNil(morkEnv* ev, void* ioMapKey);
    
    
    
    
    
    

  virtual void
  ProbeMapClearKey(morkEnv* ev, 
    void* ioMapKey, mork_count inKeyCount); 
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

  virtual void
  ProbeMapPushIn(morkEnv* ev, 
    const void* inAppKey, const void* inAppVal, 
    void* outMapKey, void* outMapVal);      
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

  virtual void
  ProbeMapPullOut(morkEnv* ev, 
    const void* inMapKey, const void* inMapVal, 
    void* outAppKey, void* outAppVal) const;    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  
  
    
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkProbeMap(); 
  
public: 
  morkProbeMap(morkEnv* ev, const morkUsage& inUsage,
  nsIMdbHeap* ioNodeHeap,
  mork_size inKeySize, mork_size inValSize,
  nsIMdbHeap* ioMapHeap, mork_size inSlots,
  mork_bool inZeroIsClearKey);
  
  void CloseProbeMap(morkEnv* ev); 
  
public: 
  mork_bool IsProbeMap() const
  { return IsNode() && mNode_Derived == morkDerived_kProbeMap; }


public: 
  static void SlotWeakMap(morkMap* me,
    morkEnv* ev, morkMap** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongMap(morkMap* me,
    morkEnv* ev, morkMap** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};




#define morkProbeMapIter_kBeforeIx ((mork_i4) -1) /* before first member */
#define morkProbeMapIter_kAfterIx  ((mork_i4) -2) /* after last member */

class morkProbeMapIter {

protected:
  morkProbeMap* sProbeMapIter_Map;      
  mork_num      sProbeMapIter_Seed;     
  
  mork_i4       sProbeMapIter_HereIx;
  
  mork_change   sProbeMapIter_Change;   
  mork_u1       sProbeMapIter_Pad[ 3 ]; 
  
public:
  morkProbeMapIter(morkEnv* ev, morkProbeMap* ioMap);
  void CloseMapIter(morkEnv* ev);
 
  morkProbeMapIter( ); 

protected: 

  void InitProbeMapIter(morkEnv* ev, morkProbeMap* ioMap);
   
  void InitMapIter(morkEnv* ev, morkProbeMap* ioMap) 
  { this->InitProbeMapIter(ev, ioMap); }
   
  mork_bool IterFirst(morkEnv* ev, void* outKey, void* outVal);
  mork_bool IterNext(morkEnv* ev, void* outKey, void* outVal);
  mork_bool IterHere(morkEnv* ev, void* outKey, void* outVal);
   
  
  
  
  void*     IterFirstVal(morkEnv* ev, void* outKey);
  
  
  void*     IterNextVal(morkEnv* ev, void* outKey);
  

  void*     IterHereVal(morkEnv* ev, void* outKey);
  

  
  
  
  void*     IterFirstKey(morkEnv* ev);
  
  
  void*     IterNextKey(morkEnv* ev);
  

  void*     IterHereKey(morkEnv* ev);
  

public: 
  mork_change* First(morkEnv* ev, void* outKey, void* outVal);
  mork_change* Next(morkEnv* ev, void* outKey, void* outVal);
  mork_change* Here(morkEnv* ev, void* outKey, void* outVal);
  
  mork_change* CutHere(morkEnv* ev, void* outKey, void* outVal);
};



#endif 
