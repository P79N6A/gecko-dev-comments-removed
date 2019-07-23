




































#ifndef _MORKZONE_
#define _MORKZONE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif





#ifdef MORK_DEBUG
#define morkZone_CONFIG_DEBUG 1 /* debug paranoid if defined */
#endif 



#define morkZone_CONFIG_VOL_STATS 1 /* count space used by zone instance */





#ifdef MORK_ENABLE_ZONE_ARENAS
#define morkZone_CONFIG_ARENA 1 /* be arena, if defined; otherwise no-op */
#endif 




#ifdef MORK_CONFIG_ALIGN_8
#define morkZone_CONFIG_ALIGN_8 1 /* ifdef: align to 8 bytes, otherwise 4 */
#endif 




#ifdef MORK_CONFIG_PTR_SIZE_4
#define morkZone_CONFIG_PTR_SIZE_4 1 /* ifdef: sizeof(void*) == 4 */
#endif 




#if defined(morkZone_CONFIG_ALIGN_8) && defined(morkZone_CONFIG_PTR_SIZE_4)
#define morkRun_USE_TAG_SLOT 1  /* need mRun_Tag slot inside morkRun */
#define morkHunk_USE_TAG_SLOT 1 /* need mHunk_Tag slot inside morkHunk */
#endif



#define morkRun_kTag ((mork_u4) 0x6D52754E ) /* ascii 'mRuN' */



class morkRun {

protected: 
#ifdef morkRun_USE_TAG_SLOT
  mork_u4   mRun_Tag; 
#endif 

  morkRun*  mRun_Next;
  
public: 
  morkRun*   RunNext() const { return mRun_Next; }
  void       RunSetNext(morkRun* ioNext) { mRun_Next = ioNext; }
  
public: 
  mork_size  RunSize() const { return (mork_size) ((mork_ip) mRun_Next); }
  void       RunSetSize(mork_size inSize)
             { mRun_Next = (morkRun*) ((mork_ip) inSize); }
  
public: 
#ifdef morkRun_USE_TAG_SLOT
  void       RunInitTag() { mRun_Tag = morkRun_kTag; }
  mork_bool  RunGoodTag() { return ( mRun_Tag == morkRun_kTag ); }
#endif 
  
public: 
  void* RunAsBlock() { return (((mork_u1*) this) + sizeof(morkRun)); }
  
  static morkRun* BlockAsRun(void* ioBlock)
  { return (morkRun*) (((mork_u1*) ioBlock) - sizeof(morkRun)); }

public: 
  static void BadRunTagError(morkEnv* ev);
  static void RunSizeAlignError(morkEnv* ev);
};






class morkOldRun : public morkRun {

protected: 
  mdb_size mOldRun_Size;
  
public: 
  mork_size  OldSize() const { return mOldRun_Size; }
  void       OldSetSize(mork_size inSize) { mOldRun_Size = inSize; }
  
};



#define morkHunk_kTag ((mork_u4) 0x68556E4B ) /* ascii 'hUnK' */



class morkHunk {

protected: 

#ifdef morkHunk_USE_TAG_SLOT
  mork_u4   mHunk_Tag; 
#endif 

  morkHunk* mHunk_Next;
  
  morkRun   mHunk_Run;
  
public: 
  void      HunkSetNext(morkHunk* ioNext) { mHunk_Next = ioNext; }
  
public: 
  morkHunk* HunkNext() const { return mHunk_Next; }

  morkRun*  HunkRun() { return &mHunk_Run; }
  
public: 
#ifdef morkHunk_USE_TAG_SLOT
  void       HunkInitTag() { mHunk_Tag = morkHunk_kTag; }
  mork_bool  HunkGoodTag() { return ( mHunk_Tag == morkHunk_kTag ); }
#endif 

public: 
  static void BadHunkTagWarning(morkEnv* ev);
  
};








#define morkZone_kNewHunkSize ((mork_size) (64 * 1024)) /* 64K per hunk */









#define morkZone_kMaxFreeVolume (morkZone_kNewHunkSize * 3)








#define morkZone_kMaxHunkWaste ((mork_size) 4096) /* 1/16 kNewHunkSize */














#define morkZone_kRoundBits 4 /* bits to round-up size for free lists */
#define morkZone_kRoundSize (1 << morkZone_kRoundBits)
#define morkZone_kRoundAdd ((1 << morkZone_kRoundBits) - 1)
#define morkZone_kRoundMask (~ ((mork_ip) morkZone_kRoundAdd))

#define morkZone_kBuckets 256 /* number of distinct free lists */






#define morkZone_kMaxCachedRun (morkZone_kBuckets * morkZone_kRoundSize)

#define morkDerived_kZone 0x5A6E /* ascii 'Zn' */






class morkZone : public morkNode, public nsIMdbHeap {


  

  
  
  
  
  
  
  
  
  
  

public: 

  nsIMdbHeap*  mZone_Heap; 
  
  mork_size    mZone_HeapVolume;  
  mork_size    mZone_BlockVolume; 
  mork_size    mZone_RunVolume;   
  mork_size    mZone_ChipVolume;  
  
  mork_size    mZone_FreeOldRunVolume; 
  
  mork_count   mZone_HunkCount;        
  mork_count   mZone_FreeOldRunCount;  

  morkHunk*    mZone_HunkList;       
  morkRun*     mZone_FreeOldRunList; 
  
  
  mork_u1*     mZone_At;     
  mork_size    mZone_AtSize; 
  
  
  
  morkRun*     mZone_FreeRuns[ morkZone_kBuckets + 1 ];
  
  
  
  
  
  
  

protected: 
  
  mork_size zone_grow_at(morkEnv* ev, mork_size inNeededSize);

  void*     zone_new_chip(morkEnv* ev, mdb_size inSize); 
  morkHunk* zone_new_hunk(morkEnv* ev, mdb_size inRunSize); 


public:
  NS_IMETHOD Alloc(nsIMdbEnv* ev, 
    mdb_size inSize,   
    void** outBlock);  
    
  NS_IMETHOD Free(nsIMdbEnv* ev, 
    void* inBlock);
    
  NS_IMETHOD HeapAddStrongRef(nsIMdbEnv* ev); 
  NS_IMETHOD HeapCutStrongRef(nsIMdbEnv* ev); 

  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkZone(); 
  
public: 
  morkZone(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioNodeHeap, 
    nsIMdbHeap* ioZoneHeap);
  
  void CloseZone(morkEnv* ev); 
  
public: 
  mork_bool IsZone() const
  { return IsNode() && mNode_Derived == morkDerived_kZone; }



public: 
  void* ZoneNewChip(morkEnv* ev, mdb_size inSize); 
    
public: 
  void* ZoneNewRun(morkEnv* ev, mdb_size inSize); 
  void  ZoneZapRun(morkEnv* ev, void* ioRunBody); 
  void* ZoneGrowRun(morkEnv* ev, void* ioRunBody, mdb_size inSize); 
    


public: 
  static void NonZoneTypeError(morkEnv* ev);
  static void NilZoneHeapError(morkEnv* ev);
  static void BadZoneTagError(morkEnv* ev);
};



#endif 
