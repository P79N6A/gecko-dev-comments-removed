




































#ifndef _MORKROWCELLCURSOR_
#define _MORKROWCELLCURSOR_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif



class orkinRowCellCursor;
#define morkDerived_kRowCellCursor 0x6343 /* ascii 'cC' */

class morkRowCellCursor : public morkCursor, public nsIMdbRowCellCursor { 


  
  
  
  
  
  
  
  
  

  

  
  
  
  

public: 

  NS_DECL_ISUPPORTS_INHERITED
  morkRowObject*   mRowCellCursor_RowObject;  
  mork_column      mRowCellCursor_Col;        
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkRowCellCursor(); 
  
public: 
  morkRowCellCursor(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkRowObject* ioRowObject);
  void CloseRowCellCursor(morkEnv* ev); 

  
  NS_IMETHOD SetRow(nsIMdbEnv* ev, nsIMdbRow* ioRow); 
  NS_IMETHOD GetRow(nsIMdbEnv* ev, nsIMdbRow** acqRow);
  

  
  NS_IMETHOD MakeCell( 
    nsIMdbEnv* ev, 
    mdb_column* outColumn, 
    mdb_pos* outPos, 
    nsIMdbCell** acqCell); 
  

  
  NS_IMETHOD SeekCell( 
    nsIMdbEnv* ev, 
    mdb_pos inPos, 
    mdb_column* outColumn, 
    nsIMdbCell** acqCell); 
  

  
  NS_IMETHOD NextCell( 
    nsIMdbEnv* ev, 
    nsIMdbCell** acqCell, 
    mdb_column* outColumn, 
    mdb_pos* outPos); 
    
  NS_IMETHOD PickNextCell( 
    nsIMdbEnv* ev, 
    nsIMdbCell* ioCell, 
    const mdbColumnSet* inFilterSet, 
    mdb_column* outColumn, 
    mdb_pos* outPos); 

  
  
  
  


private: 
  morkRowCellCursor(const morkRowCellCursor& other);
  morkRowCellCursor& operator=(const morkRowCellCursor& other);

public: 
  mork_bool IsRowCellCursor() const
  { return IsNode() && mNode_Derived == morkDerived_kRowCellCursor; }


public: 
  static void NilRowObjectError(morkEnv* ev);
  static void NonRowCellCursorTypeError(morkEnv* ev);

public: 
  static void SlotWeakRowCellCursor(morkRowCellCursor* me,
    morkEnv* ev, morkRowCellCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongRowCellCursor(morkRowCellCursor* me,
    morkEnv* ev, morkRowCellCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
