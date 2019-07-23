




































#ifndef _ORKINTABLEROWCURSOR_
#define _ORKINTABLEROWCURSOR_ 1

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

#ifndef _MORKTABLEROWCURSOR_
#include "morkTableRowCursor.h"
#endif



class morkTableRowCursor;
#define morkMagic_kTableRowCursor 0x54724375 /* ascii 'TrCu' */



class orkinTableRowCursor :
  public morkHandle, public nsIMdbTableRowCursor { 


public: 
  
  virtual ~orkinTableRowCursor(); 
  
protected: 
  orkinTableRowCursor(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkTableRowCursor* ioObject); 
    
  

private: 
  orkinTableRowCursor(const morkHandle& other);
  orkinTableRowCursor& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
  
public: 

  static orkinTableRowCursor* MakeTableRowCursor(morkEnv* ev, 
    morkTableRowCursor* ioObject);

public: 

  morkEnv* CanUseTableRowCursor(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinTableRowCursor() const
  { return mHandle_Magic == morkMagic_kTableRowCursor; }

  mork_bool IsOrkinTableRowCursorHandle() const
  { return this->IsHandle() && this->IsOrkinTableRowCursor(); }

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
  





  
  NS_IMETHOD GetTable(nsIMdbEnv* ev, nsIMdbTable** acqTable);
  

  
  NS_IMETHOD NextRowOid( 
    nsIMdbEnv* ev, 
    mdbOid* outOid, 
    mdb_pos* outRowPos);    
  

  
  NS_IMETHOD NextRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    mdb_pos* outRowPos);    
  

  
  NS_IMETHOD CanHaveDupRowMembers(nsIMdbEnv* ev, 
    mdb_bool* outCanHaveDups);
    
  NS_IMETHOD MakeUniqueCursor( 
    nsIMdbEnv* ev, 
    nsIMdbTableRowCursor** acqCursor);    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
  


};



#endif 
