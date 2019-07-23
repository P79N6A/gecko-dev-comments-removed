




































#ifndef _ORKINPORTTABLECURSOR_
#define _ORKINPORTTABLECURSOR_ 1

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

#ifndef _MORKPORTTABLECURSOR_
#include "morkPortTableCursor.h"
#endif



class morkPortTableCursor;
#define morkMagic_kPortTableCursor 0x50744375 /* ascii 'PtCu' */








class orkinPortTableCursor :
  public morkHandle, public nsIMdbPortTableCursor { 


public: 
  
  virtual ~orkinPortTableCursor(); 
  
protected: 
  orkinPortTableCursor(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkPortTableCursor* ioObject); 
    
  

private: 
  orkinPortTableCursor(const morkHandle& other);
  orkinPortTableCursor& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
 
public: 

  static orkinPortTableCursor* MakePortTableCursor(morkEnv* ev, 
    morkPortTableCursor* ioObject);

public: 

  morkEnv* CanUsePortTableCursor(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinPortTableCursor() const
  { return mHandle_Magic == morkMagic_kPortTableCursor; }

  mork_bool IsOrkinPortTableCursorHandle() const
  { return this->IsHandle() && this->IsOrkinPortTableCursor(); }

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
  
  




  
  NS_IMETHOD GetCount(nsIMdbEnv* ev, mdb_count* outCount); 
  NS_IMETHOD GetSeed(nsIMdbEnv* ev, mdb_seed* outSeed);    
  
  NS_IMETHOD SetPos(nsIMdbEnv* ev, mdb_pos inPos);   
  NS_IMETHOD GetPos(nsIMdbEnv* ev, mdb_pos* outPos);
  
  NS_IMETHOD SetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool inFail);
  NS_IMETHOD GetDoFailOnSeedOutOfSync(nsIMdbEnv* ev, mdb_bool* outFail);
  





  
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
  


};



#endif 
