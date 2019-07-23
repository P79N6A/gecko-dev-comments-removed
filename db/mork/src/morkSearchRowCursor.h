




































#ifndef _MORKSEARCHROWCURSOR_
#define _MORKSEARCHROWCURSOR_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif



class morkUniqRowCursor;
class orkinTableRowCursor;


class morkSearchRowCursor : public morkTableRowCursor { 


  
  
  
  
  
  
  
  
  

  

  
  
  
  

  

public: 
    

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkSearchRowCursor(); 
  
public: 
  morkSearchRowCursor(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos);
  void CloseSearchRowCursor(morkEnv* ev); 

private: 
  morkSearchRowCursor(const morkSearchRowCursor& other);
  morkSearchRowCursor& operator=(const morkSearchRowCursor& other);

public: 
  
  


public: 
  static void NonSearchRowCursorTypeError(morkEnv* ev);

public: 

  morkUniqRowCursor* MakeUniqCursor(morkEnv* ev);

public: 

  virtual mork_bool CanHaveDupRowMembers(morkEnv* ev);
  virtual mork_count GetMemberCount(morkEnv* ev);

#if 0
  virtual orkinTableRowCursor* AcquireUniqueRowCursorHandle(morkEnv* ev);
#endif

  
  virtual morkRow* NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos);

public: 
  static void SlotWeakSearchRowCursor(morkSearchRowCursor* me,
    morkEnv* ev, morkSearchRowCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongSearchRowCursor(morkSearchRowCursor* me,
    morkEnv* ev, morkSearchRowCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
