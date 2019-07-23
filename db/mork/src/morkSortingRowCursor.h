




































#ifndef _MORKSORTINGROWCURSOR_
#define _MORKSORTINGROWCURSOR_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCURSOR_
#include "morkCursor.h"
#endif

#ifndef _MORKSORTINGROWCURSOR_
#include "morkSortingRowCursor.h"
#endif

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif

#ifndef _MORKMAP_
#include "morkMap.h"
#endif



class orkinSortingRowCursor;


class morkSortingRowCursor : public morkTableRowCursor { 


  
  
  
  
  
  
  
  
  

  

  
  
  
  

  

public: 

  morkSorting*  mSortingRowCursor_Sorting; 
    

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkSortingRowCursor(); 
  
public: 
  morkSortingRowCursor(morkEnv* ev, const morkUsage& inUsage,
    nsIMdbHeap* ioHeap, morkTable* ioTable, mork_pos inRowPos,
    morkSorting* ioSorting);
  void CloseSortingRowCursor(morkEnv* ev); 

private: 
  morkSortingRowCursor(const morkSortingRowCursor& other);
  morkSortingRowCursor& operator=(const morkSortingRowCursor& other);

public: 
  
  


public: 
  static void NonSortingRowCursorTypeError(morkEnv* ev);

public: 

  virtual mork_bool CanHaveDupRowMembers(morkEnv* ev);
  virtual mork_count GetMemberCount(morkEnv* ev);

  virtual orkinTableRowCursor* AcquireUniqueRowCursorHandle(morkEnv* ev);
  
  
  virtual morkRow* NextRow(morkEnv* ev, mdbOid* outOid, mdb_pos* outPos);

public: 
  static void SlotWeakSortingRowCursor(morkSortingRowCursor* me,
    morkEnv* ev, morkSortingRowCursor** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongSortingRowCursor(morkSortingRowCursor* me,
    morkEnv* ev, morkSortingRowCursor** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
