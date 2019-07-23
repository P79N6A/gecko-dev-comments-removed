




































#ifndef _MORKUNIQROWCURSOR_
#define _MORKUNIQROWCURSOR_ 1

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


class morkUniqRowCursor : public morkTableRowCursor { 


  
  
  
  
  
  
  
  
  

  

  
  
  
  

  

public: 
    

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkUniqRowCursor(); 
  
public: 
  morkUniqRowCursor(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos);
  void CloseUniqRowCursor(morkEnv* ev); 

private: 
  morkUniqRowCursor(const morkUniqRowCursor& other);
  morkUniqRowCursor& operator=(const morkUniqRowCursor& other);

public: 
  
  


public: 
  static void NonUniqRowCursorTypeError(morkEnv* ev);

public: 

  virtual mork_bool CanHaveDupRowMembers(morkEnv* ev);
  virtual mork_count GetMemberCount(morkEnv* ev);

  virtual orkinTableRowCursor* AcquireUniqueRowCursorHandle(morkEnv* ev);
  
  
  virtual morkRow* NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos);

public: 
  static void SlotWeakUniqRowCursor(morkUniqRowCursor* me,
    morkEnv* ev, morkUniqRowCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongUniqRowCursor(morkUniqRowCursor* me,
    morkEnv* ev, morkUniqRowCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
