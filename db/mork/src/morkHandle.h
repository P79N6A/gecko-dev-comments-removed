




































#ifndef _MORKHANDLE_
#define _MORKHANDLE_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKDEQUE_
#include "morkDeque.h"
#endif

#ifndef _MORKPOOL_
#include "morkPool.h"
#endif



class morkPool;
class morkObject;
class morkFactory;

#define morkDerived_kHandle 0x486E /* ascii 'Hn' */
#define morkHandle_kTag   0x68416E44 /* ascii 'hAnD' */



class morkHandle : public morkNode {
  

  

  
  
  
  
  
  
  
  
  
  

public: 

  mork_u4         mHandle_Tag;     
  morkEnv*        mHandle_Env;     
  morkHandleFace* mHandle_Face;    
  morkObject*     mHandle_Object;  
  mork_magic      mHandle_Magic;   
  

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkHandle(); 
  
public: 
  morkHandle(morkEnv* ev, 
    morkHandleFace* ioFace,  
    morkObject* ioObject,    
    mork_magic inMagic);     
  void CloseHandle(morkEnv* ev); 

private: 
  morkHandle(const morkHandle& other);
  morkHandle& operator=(const morkHandle& other);
  
protected: 
  friend class morkHandleFrame;
  morkHandle() { }

public: 
  mork_bool IsHandle() const
  { return IsNode() && mNode_Derived == morkDerived_kHandle; }


public: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  

public: 
  mork_bool GoodHandleTag() const
  { return mHandle_Tag == morkHandle_kTag; }
  
  void NewBadMagicHandleError(morkEnv* ev, mork_magic inMagic) const;
  void NewDownHandleError(morkEnv* ev) const;
  void NilFactoryError(morkEnv* ev) const;
  void NilHandleObjectError(morkEnv* ev) const;
  void NonNodeObjectError(morkEnv* ev) const;
  void NonOpenObjectError(morkEnv* ev) const;
  
  morkObject* GetGoodHandleObject(morkEnv* ev,
    mork_bool inMutable, mork_magic inMagicType, mork_bool inClosedOkay) const;

public: 

  morkEnv* CanUseHandle(nsIMdbEnv* mev, mork_bool inMutable,
    mork_bool inClosedOkay, mdb_err* outErr) const;
    
  
  mdb_err Handle_IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);

  mdb_err Handle_GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  mdb_err Handle_GetWeakRefCount(nsIMdbEnv* ev,  mdb_count* outCount);  
  mdb_err Handle_GetStrongRefCount(nsIMdbEnv* ev, mdb_count* outCount);

  mdb_err Handle_AddWeakRef(nsIMdbEnv* ev);
  mdb_err Handle_AddStrongRef(nsIMdbEnv* ev);

  mdb_err Handle_CutWeakRef(nsIMdbEnv* ev);
  mdb_err Handle_CutStrongRef(nsIMdbEnv* ev);
  
  mdb_err Handle_CloseMdbObject(nsIMdbEnv* ev);
  mdb_err Handle_IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  

public: 
  static void SlotWeakHandle(morkHandle* me,
    morkEnv* ev, morkHandle** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongHandle(morkHandle* me,
    morkEnv* ev, morkHandle** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};

#define morkHandleFrame_kPadSlotCount 4









class morkHandleFrame {
public:
  morkLink    mHandleFrame_Link;    
  morkHandle  mHandleFrame_Handle;
  mork_ip     mHandleFrame_Padding[ morkHandleFrame_kPadSlotCount ];
  
public:
  morkHandle*  AsHandle() { return &mHandleFrame_Handle; }
  
  morkHandleFrame() {}  

private: 
  morkHandleFrame(const morkHandleFrame& other);
  morkHandleFrame& operator=(const morkHandleFrame& other);
};

#define morkHandleFrame_kHandleOffset \
  mork_OffsetOf(morkHandleFrame,mHandleFrame_Handle)
  
#define morkHandle_AsHandleFrame(h) ((h)->mHandle_Block , \
 ((morkHandleFrame*) (((mork_u1*)(h)) - morkHandleFrame_kHandleOffset)))



#endif 
