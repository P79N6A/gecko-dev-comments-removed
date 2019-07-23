




































#ifndef _MORKROWOBJECT_
#define _MORKROWOBJECT_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif



class nsIMdbRow;
#define morkDerived_kRowObject 0x724F /* ascii 'rO' */

class morkRowObject : public morkObject, public nsIMdbRow  { 

public: 
  NS_DECL_ISUPPORTS_INHERITED
  
  morkRow*    mRowObject_Row;     
  morkStore*  mRowObject_Store;   
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkRowObject(); 
  
public: 
  morkRowObject(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkRow* ioRow, morkStore* ioStore);
  void CloseRowObject(morkEnv* ev); 



  
  NS_IMETHOD GetSeed(nsIMdbEnv* ev,
    mdb_seed* outSeed);    
  NS_IMETHOD GetCount(nsIMdbEnv* ev,
    mdb_count* outCount); 

  NS_IMETHOD GetPort(nsIMdbEnv* ev,
    nsIMdbPort** acqPort); 
  

  
  NS_IMETHOD GetCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inMemberPos, 
    nsIMdbCursor** acqCursor); 
  

  
  NS_IMETHOD GetOid(nsIMdbEnv* ev,
    mdbOid* outOid); 
  NS_IMETHOD BecomeContent(nsIMdbEnv* ev,
    const mdbOid* inOid); 
  

  
  NS_IMETHOD DropActivity( 
    nsIMdbEnv* ev);
  




  
  NS_IMETHOD GetRowCellCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbRowCellCursor** acqCursor); 
  

  
  NS_IMETHOD AddColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    const mdbYarn* inYarn); 

  NS_IMETHOD CutColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn); 

  NS_IMETHOD CutAllColumns( 
    nsIMdbEnv* ev); 
  

  
  NS_IMETHOD NewCell( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbCell** acqCell); 
    
  NS_IMETHOD AddCell( 
    nsIMdbEnv* ev, 
    const nsIMdbCell* inCell); 
    
  NS_IMETHOD GetCell( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbCell** acqCell); 
    
  NS_IMETHOD EmptyAllCells( 
    nsIMdbEnv* ev); 
  

  
  NS_IMETHOD AddRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioSourceRow); 
    
  NS_IMETHOD SetRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioSourceRow); 
  

  
  NS_IMETHOD SetCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    const mdbYarn* inYarn);   
  
  
  NS_IMETHOD GetCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdbYarn* outYarn);  
  
  
  NS_IMETHOD AliasCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdbYarn* outYarn); 
  
  NS_IMETHOD NextCellYarn(nsIMdbEnv* ev, 
    mdb_column* ioColumn, 
    mdbYarn* outYarn);  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_IMETHOD SeekCellYarn( 
    nsIMdbEnv* ev, 
    mdb_pos inPos, 
    mdb_column* outColumn, 
    mdbYarn* outYarn); 
  
  
  
  

  



private: 
  morkRowObject(const morkRowObject& other);
  morkRowObject& operator=(const morkRowObject& other);

public: 
  mork_bool IsRowObject() const
  { return IsNode() && mNode_Derived == morkDerived_kRowObject; }


public: 
  static void NonRowObjectTypeError(morkEnv* ev);
  static void NilRowError(morkEnv* ev);
  static void NilStoreError(morkEnv* ev);
  static void RowObjectRowNotSelfError(morkEnv* ev);

public: 

  nsIMdbRow* AcquireRowHandle(morkEnv* ev); 
  
public: 
  static void SlotWeakRowObject(morkRowObject* me,
    morkEnv* ev, morkRowObject** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongRowObject(morkRowObject* me,
    morkEnv* ev, morkRowObject** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
