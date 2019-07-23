





































#ifndef _MORKTABLEROWCURSOR_
#define _MORKTABLEROWCURSOR_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif



class orkinTableRowCursor;
#define morkDerived_kTableRowCursor 0x7243 /* ascii 'rC' */

class morkTableRowCursor : public morkCursor, public nsIMdbTableRowCursor { 


  
  
  
  
  
  
  
  
  

  

  
  
  
  

public: 
  morkTable*  mTableRowCursor_Table; 
    

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkTableRowCursor(); 
  
public: 
  morkTableRowCursor(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos);
  void CloseTableRowCursor(morkEnv* ev); 

private: 
  morkTableRowCursor(const morkTableRowCursor& other);
  morkTableRowCursor& operator=(const morkTableRowCursor& other);

public:
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetCount(nsIMdbEnv* ev, mdb_count* outCount); 
  NS_IMETHOD GetSeed(nsIMdbEnv* ev, mdb_seed* outSeed);    
  
  NS_IMETHOD SetPos(nsIMdbEnv* ev, mdb_pos inPos);   
  NS_IMETHOD GetPos(nsIMdbEnv* ev, mdb_pos* outPos);
  
  NS_IMETHOD SetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool inFail);
  NS_IMETHOD GetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool* outFail);

  
    NS_IMETHOD GetTable(nsIMdbEnv* ev, nsIMdbTable** acqTable);
  

  
  NS_IMETHOD CanHaveDupRowMembers(nsIMdbEnv* ev, 
    mdb_bool* outCanHaveDups);
    
  NS_IMETHOD MakeUniqueCursor( 
    nsIMdbEnv* ev, 
    nsIMdbTableRowCursor** acqCursor);    
  

  
  NS_IMETHOD NextRowOid( 
    nsIMdbEnv* ev, 
    mdbOid* outOid, 
    mdb_pos* outRowPos);    
  NS_IMETHOD PrevRowOid( 
    nsIMdbEnv* ev, 
    mdbOid* outOid, 
    mdb_pos* outRowPos);    
  

  
  NS_IMETHOD NextRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    mdb_pos* outRowPos);    
  NS_IMETHOD PrevRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    mdb_pos* outRowPos);    
  


public: 
  mork_bool IsTableRowCursor() const
  { return IsNode() && mNode_Derived == morkDerived_kTableRowCursor; }


public: 
  static void NonTableRowCursorTypeError(morkEnv* ev);

public: 
  mdb_pos NextRowOid(morkEnv* ev, mdbOid* outOid);
  mdb_pos PrevRowOid(morkEnv* ev, mdbOid* outOid);

public: 

  virtual mork_bool CanHaveDupRowMembers(morkEnv* ev);
  virtual mork_count GetMemberCount(morkEnv* ev);

  virtual morkRow* NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos);
  virtual morkRow* PrevRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos);

public: 
  static void SlotWeakTableRowCursor(morkTableRowCursor* me,
    morkEnv* ev, morkTableRowCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongTableRowCursor(morkTableRowCursor* me,
    morkEnv* ev, morkTableRowCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
