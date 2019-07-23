




































#ifndef _ORKINENV_
#define _ORKINENV_ 1

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

#ifndef _MORKEnv_
#include "morkEnv.h"
#endif



#define morkMagic_kEnv 0x456E7669 /* ascii 'Envi' */



class orkinEnv : public morkHandle, public nsIMdbEnv { 


public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~orkinEnv(); 
  
protected: 
  orkinEnv(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkEnv* ioObject); 
    
  

private: 
  orkinEnv(const morkHandle& other);
  orkinEnv& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
  
public: 

  static orkinEnv* MakeEnv(morkEnv* ev, morkEnv* ioObject);

public: 

  morkEnv* CanUseEnv(mork_bool inMutable, mdb_err* outErr) const;

public: 
  mork_bool IsOrkinEnv() const
  { return mHandle_Magic == morkMagic_kEnv; }

  mork_bool IsOrkinEnvHandle() const
  { return this->IsHandle() && this->IsOrkinEnv(); }

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
  
  




  
  NS_IMETHOD GetErrorCount(mdb_count* outCount,
    mdb_bool* outShouldAbort);
  NS_IMETHOD GetWarningCount(mdb_count* outCount,
    mdb_bool* outShouldAbort);
  
  NS_IMETHOD GetEnvBeVerbose(mdb_bool* outBeVerbose);
  NS_IMETHOD SetEnvBeVerbose(mdb_bool inBeVerbose);
  
  NS_IMETHOD GetDoTrace(mdb_bool* outDoTrace);
  NS_IMETHOD SetDoTrace(mdb_bool inDoTrace);
  
  NS_IMETHOD GetAutoClear(mdb_bool* outAutoClear);
  NS_IMETHOD SetAutoClear(mdb_bool inAutoClear);
  
  NS_IMETHOD GetErrorHook(nsIMdbErrorHook** acqErrorHook);
  NS_IMETHOD SetErrorHook(
    nsIMdbErrorHook* ioErrorHook); 
  
  NS_IMETHOD GetHeap(nsIMdbHeap** acqHeap);
  NS_IMETHOD SetHeap(
    nsIMdbHeap* ioHeap); 
  
  
  NS_IMETHOD ClearErrors(); 
  NS_IMETHOD ClearWarnings(); 
  NS_IMETHOD ClearErrorsAndWarnings(); 

};



#endif 
