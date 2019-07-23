




































#ifndef _MORKTHUMB_
#define _MORKTHUMB_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif




#define morkThumb_kMagic_OpenFilePort               1 /* factory method */
#define morkThumb_kMagic_OpenFileStore              2 /* factory method */
#define morkThumb_kMagic_ExportToFormat             3 /* port method */
#define morkThumb_kMagic_ImportContent              4 /* store method */
#define morkThumb_kMagic_LargeCommit                5 /* store method */
#define morkThumb_kMagic_SessionCommit              6 /* store method */
#define morkThumb_kMagic_CompressCommit             7 /* store method */
#define morkThumb_kMagic_SearchManyColumns          8 /* table method */
#define morkThumb_kMagic_NewSortColumn              9 /* table metho) */
#define morkThumb_kMagic_NewSortColumnWithCompare  10 /* table method */
#define morkThumb_kMagic_CloneSortColumn           11 /* table method */
#define morkThumb_kMagic_AddIndex                  12 /* table method */
#define morkThumb_kMagic_CutIndex                  13 /* table method */

#define morkDerived_kThumb 0x5468 /* ascii 'Th' */



class morkThumb : public morkObject, public nsIMdbThumb {


  

  
  
  
  
  
  
  
  
  
  

  
  

public: 
  NS_DECL_ISUPPORTS_INHERITED


  NS_IMETHOD GetProgress(nsIMdbEnv* ev, mdb_count* outTotal,
    mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken);
  
  NS_IMETHOD DoMore(nsIMdbEnv* ev, mdb_count* outTotal,
    mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken);
  
  NS_IMETHOD CancelAndBreakThumb(nsIMdbEnv* ev);


  
  
  mork_magic   mThumb_Magic;   
  mork_count   mThumb_Total;
  mork_count   mThumb_Current;

  mork_bool    mThumb_Done;
  mork_bool    mThumb_Broken;
  mork_u2      mThumb_Seed;  
  
  morkStore*   mThumb_Store; 
  nsIMdbFile*  mThumb_File;  
  morkWriter*  mThumb_Writer;  
  morkBuilder* mThumb_Builder;  
  morkPort*    mThumb_SourcePort;  
  
  mork_bool    mThumb_DoCollect; 
  mork_bool    mThumb_Pad[ 3 ]; 
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkThumb(); 
  
public: 
  morkThumb(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, nsIMdbHeap* ioSlotHeap, mork_magic inMagic);
  void CloseThumb(morkEnv* ev); 

private: 
  morkThumb(const morkThumb& other);
  morkThumb& operator=(const morkThumb& other);

public: 
  mork_bool IsThumb() const
  { return IsNode() && mNode_Derived == morkDerived_kThumb; }


public: 
  static void NonThumbTypeError(morkEnv* ev);
  static void UnsupportedThumbMagicError(morkEnv* ev);

  static void NilThumbStoreError(morkEnv* ev);
  static void NilThumbFileError(morkEnv* ev);
  static void NilThumbWriterError(morkEnv* ev);
  static void NilThumbBuilderError(morkEnv* ev);
  static void NilThumbSourcePortError(morkEnv* ev);

public: 

  void DoMore_OpenFilePort(morkEnv* ev);
  void DoMore_OpenFileStore(morkEnv* ev);
  void DoMore_ExportToFormat(morkEnv* ev);
  void DoMore_ImportContent(morkEnv* ev);
  void DoMore_LargeCommit(morkEnv* ev);
  void DoMore_SessionCommit(morkEnv* ev);
  void DoMore_CompressCommit(morkEnv* ev);
  void DoMore_Commit(morkEnv* ev);
  void DoMore_SearchManyColumns(morkEnv* ev);
  void DoMore_NewSortColumn(morkEnv* ev);
  void DoMore_NewSortColumnWithCompare(morkEnv* ev);
  void DoMore_CloneSortColumn(morkEnv* ev);
  void DoMore_AddIndex(morkEnv* ev);
  void DoMore_CutIndex(morkEnv* ev);

public: 

  morkStore* ThumbToOpenStore(morkEnv* ev);
  

public: 

  static morkThumb* Make_OpenFileStore(morkEnv* ev, 
    nsIMdbHeap* ioHeap, morkStore* ioStore);

  static morkThumb* Make_CompressCommit(morkEnv* ev, 
    nsIMdbHeap* ioHeap, morkStore* ioStore, mork_bool inDoCollect);

  static morkThumb* Make_LargeCommit(morkEnv* ev, 
    nsIMdbHeap* ioHeap, morkStore* ioStore);


  void GetProgress(morkEnv* ev, mdb_count* outTotal,
    mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken);
  
  void DoMore(morkEnv* ev, mdb_count* outTotal,
    mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken);
  
  void CancelAndBreakThumb(morkEnv* ev);




public: 
  static void SlotWeakThumb(morkThumb* me,
    morkEnv* ev, morkThumb** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongThumb(morkThumb* me,
    morkEnv* ev, morkThumb** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};




#endif 
