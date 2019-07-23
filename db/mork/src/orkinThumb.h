




































#ifndef _ORKINTHUMB_
#define _ORKINTHUMB_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKNODE_
#include "morkNode.h"
#endif

#ifndef _MORKHANDLE_
#include "morkHandle.h"
#endif

#ifndef _MORKTHUMB_
#include "morkThumb.h"
#endif



#define morkMagic_kThumb 0x54686D62 /* ascii 'Thmb' */



class orkinThumb : public morkHandle, public nsIMdbThumb { 


public: 
  
  virtual ~orkinThumb(); 
  
protected: 
  orkinThumb(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkThumb* ioObject); 
    
  

private: 
  orkinThumb(const morkHandle& other);
  orkinThumb& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
 
public: 

  static orkinThumb* MakeThumb(morkEnv* ev, morkThumb* ioObject);

public: 

  morkEnv* CanUseThumb(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinThumb() const
  { return mHandle_Magic == morkMagic_kThumb; }

  mork_bool IsOrkinThumbHandle() const
  { return this->IsHandle() && this->IsOrkinThumb(); }

public: 


  NS_DECL_ISUPPORTS


  
  NS_IMETHOD IsFrozenMdbObject(nsIMdbEnv* ev, mdb_bool* outIsReadonly);
  
  

  
  NS_IMETHOD GetMdbFactory(nsIMdbEnv* ev, nsIMdbFactory** acqFactory); 
  

  
  NS_IMETHOD GetWeakRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);  
  NS_IMETHOD GetStrongRefCount(nsIMdbEnv* ev, 
    mdb_count* outCount);

  NS_IMETHOD AddWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev);

  NS_IMETHOD CutWeakRef(nsIMdbEnv* ev);
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev);
  
  NS_IMETHOD CloseMdbObject(nsIMdbEnv* ev); 
  NS_IMETHOD IsOpenMdbObject(nsIMdbEnv* ev, mdb_bool* outOpen);
  
  



  NS_IMETHOD GetProgress(nsIMdbEnv* ev, mdb_count* outTotal,
    mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken);
  
  NS_IMETHOD DoMore(nsIMdbEnv* ev, mdb_count* outTotal,
    mdb_count* outCurrent, mdb_bool* outDone, mdb_bool* outBroken);
  
  NS_IMETHOD CancelAndBreakThumb(nsIMdbEnv* ev);

};




#endif 
