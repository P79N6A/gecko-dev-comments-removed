




































#ifndef _MORKPORTTABLECURSOR_
#define _MORKPORTTABLECURSOR_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif

#ifndef _MORKROWSPACE_
#include "morkRowSpace.h"
#endif



class orkinPortTableCursor;
#define morkDerived_kPortTableCursor 0x7443 /* ascii 'tC' */

class morkPortTableCursor : public morkCursor, public nsIMdbPortTableCursor { 
public:
  NS_DECL_ISUPPORTS_INHERITED

  
  
  
  
  
  
  
  
  

  

  
  
  
  

public: 
  
  NS_IMETHOD SetPort(nsIMdbEnv* ev, nsIMdbPort* ioPort); 
  NS_IMETHOD GetPort(nsIMdbEnv* ev, nsIMdbPort** acqPort);
  
  NS_IMETHOD SetRowScope(nsIMdbEnv* ev, 
    mdb_scope inRowScope);
  NS_IMETHOD GetRowScope(nsIMdbEnv* ev, mdb_scope* outRowScope); 
  
    
  NS_IMETHOD SetTableKind(nsIMdbEnv* ev, 
    mdb_kind inTableKind);
  NS_IMETHOD GetTableKind(nsIMdbEnv* ev, mdb_kind* outTableKind);
  
  

  
  NS_IMETHOD NextTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable** acqTable); 
  
  morkStore*    mPortTableCursor_Store;  
  
  mdb_scope     mPortTableCursor_RowScope;
  mdb_kind      mPortTableCursor_TableKind;
  
  
  
  
  morkTable* mPortTableCursor_LastTable; 
  morkRowSpace* mPortTableCursor_RowSpace; 

  morkRowSpaceMapIter mPortTableCursor_SpaceIter; 
  morkTableMapIter    mPortTableCursor_TableIter; 
  
  
  
  mork_bool           mPortTableCursor_TablesDidEnd; 
  mork_bool           mPortTableCursor_SpacesDidEnd; 
  mork_u1             mPortTableCursor_Pad[ 2 ]; 
   

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkPortTableCursor(); 
  
public: 
  morkPortTableCursor(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkStore* ioStore, mdb_scope inRowScope,
      mdb_kind inTableKind, nsIMdbHeap* ioSlotHeap);
  void ClosePortTableCursor(morkEnv* ev); 

private: 
  morkPortTableCursor(const morkPortTableCursor& other);
  morkPortTableCursor& operator=(const morkPortTableCursor& other);

public: 
  mork_bool IsPortTableCursor() const
  { return IsNode() && mNode_Derived == morkDerived_kPortTableCursor; }


protected: 

  void init_space_tables_map(morkEnv* ev);

public: 

  static void NilCursorStoreError(morkEnv* ev);
  static void NonPortTableCursorTypeError(morkEnv* ev);

 morkEnv* CanUsePortTableCursor(nsIMdbEnv* mev,
  mork_bool inMutable, mdb_err* outErr) const;

  
  morkRowSpace* NextSpace(morkEnv* ev);
  morkTable* NextTable(morkEnv* ev);

  mork_bool SetRowScope(morkEnv* ev, mork_scope inRowScope);
  mork_bool SetTableKind(morkEnv* ev, mork_kind inTableKind);

public: 
  static void SlotWeakPortTableCursor(morkPortTableCursor* me,
    morkEnv* ev, morkPortTableCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongPortTableCursor(morkPortTableCursor* me,
    morkEnv* ev, morkPortTableCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};





#endif 
