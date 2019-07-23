




































#ifndef _MORKYARN_
#define _MORKYARN_ 1

#ifndef _MORK_
#include "mork.h"
#endif




#define morkDerived_kYarn 0x7952 /* ascii 'yR' */






class morkYarn : public morkNode { 
  

  

  
  
  
  
  
  
  
  
  
  

public: 
  mdbYarn  mYarn_Body;
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkYarn(); 
  
public: 
  morkYarn(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap);
  void CloseYarn(morkEnv* ev); 

private: 
  morkYarn(const morkYarn& other);
  morkYarn& operator=(const morkYarn& other);

public: 
  mork_bool IsYarn() const
  { return IsNode() && mNode_Derived == morkDerived_kYarn; }


public: 
  static void NonYarnTypeError(morkEnv* ev);

public: 
  static void SlotWeakYarn(morkYarn* me,
    morkEnv* ev, morkYarn** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongYarn(morkYarn* me,
    morkEnv* ev, morkYarn** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
