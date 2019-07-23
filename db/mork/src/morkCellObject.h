




































#ifndef _MORKCELLOBJECT_
#define _MORKCELLOBJECT_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif



#define morkDerived_kCellObject 0x634F /* ascii 'cO' */

class morkCellObject : public morkObject, public nsIMdbCell { 


  
  
  
  
  
  
  
  
  

  
  

public: 
  NS_DECL_ISUPPORTS_INHERITED

  morkRowObject*  mCellObject_RowObject;  
  morkRow*        mCellObject_Row;        
  morkCell*       mCellObject_Cell;       
  mork_column     mCellObject_Col;        
  mork_u2         mCellObject_RowSeed;    
  mork_u2         mCellObject_Pos;        
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkCellObject(); 
  
public: 
  morkCellObject(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkRow* ioRow, morkCell* ioCell,
    mork_column inCol, mork_pos inPos);
  void CloseCellObject(morkEnv* ev); 

  NS_IMETHOD SetBlob(nsIMdbEnv* ev,
    nsIMdbBlob* ioBlob); 
  
  
  NS_IMETHOD ClearBlob( 
    nsIMdbEnv* ev);
  
  
  NS_IMETHOD GetBlobFill(nsIMdbEnv* ev,
    mdb_fill* outFill);  
  
  
  
  NS_IMETHOD SetYarn(nsIMdbEnv* ev, 
    const mdbYarn* inYarn);   
  
  
  NS_IMETHOD GetYarn(nsIMdbEnv* ev, 
    mdbYarn* outYarn);  
  
  
  NS_IMETHOD AliasYarn(nsIMdbEnv* ev, 
    mdbYarn* outYarn); 
  NS_IMETHOD SetColumn(nsIMdbEnv* ev, mdb_column inColumn); 
  NS_IMETHOD GetColumn(nsIMdbEnv* ev, mdb_column* outColumn);
  
  NS_IMETHOD GetCellInfo(  
    nsIMdbEnv* ev, 
    mdb_column* outColumn,           
    mdb_fill*   outBlobFill,         
    mdbOid*     outChildOid,         
    mdb_bool*   outIsRowChild);  

  
  
  
  NS_IMETHOD GetRow(nsIMdbEnv* ev, 
    nsIMdbRow** acqRow);
  NS_IMETHOD GetPort(nsIMdbEnv* ev, 
    nsIMdbPort** acqPort);
  

  
  NS_IMETHOD HasAnyChild( 
    nsIMdbEnv* ev,
    mdbOid* outOid,  
    mdb_bool* outIsRow); 

  NS_IMETHOD GetAnyChild( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    nsIMdbTable** acqTable); 


  NS_IMETHOD SetChildRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow); 

  NS_IMETHOD GetChildRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow); 


  NS_IMETHOD SetChildTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable* inTable); 

  NS_IMETHOD GetChildTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable** acqTable); 
  


private: 
  morkCellObject(const morkCellObject& other);
  morkCellObject& operator=(const morkCellObject& other);

public: 
  mork_bool IsCellObject() const
  { return IsNode() && mNode_Derived == morkDerived_kCellObject; }


public: 

  morkEnv*  CanUseCell(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr, morkCell** outCell) ;

  mork_bool ResyncWithRow(morkEnv* ev); 
  morkAtom* GetCellAtom(morkEnv* ev) const;

  static void MissingRowColumnError(morkEnv* ev);
  static void NilRowError(morkEnv* ev);
  static void NilCellError(morkEnv* ev);
  static void NilRowObjectError(morkEnv* ev);
  static void WrongRowObjectRowError(morkEnv* ev);
  static void NonCellObjectTypeError(morkEnv* ev);

  nsIMdbCell* AcquireCellHandle(morkEnv* ev);

public: 
  static void SlotWeakCellObject(morkCellObject* me,
    morkEnv* ev, morkCellObject** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongCellObject(morkCellObject* me,
    morkEnv* ev, morkCellObject** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
