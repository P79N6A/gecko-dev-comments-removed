




































#ifndef _ORKINROWCELLCURSOR_
#define _ORKINROWCELLCURSOR_ 1

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

#ifndef _MORKROWCELLCURSOR_
#include "morkRowCellCursor.h"
#endif



#define morkMagic_kRowCellCursor 0x52634375 /* ascii 'RcCu' */










class orkinRowCellCursor :
  public morkHandle, public nsIMdbRowCellCursor { 


public: 
  
  virtual ~orkinRowCellCursor(); 
  
protected: 
  orkinRowCellCursor(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkRowCellCursor* ioObject); 
    
  

private: 
  orkinRowCellCursor(const morkHandle& other);
  orkinRowCellCursor& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
public: 

  static orkinRowCellCursor* MakeRowCellCursor(morkEnv* ev, 
    morkRowCellCursor* ioObject);

public: 

  morkEnv* CanUseRowCellCursor(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr, morkRow** outRow) const;

public: 
  mork_bool IsOrkinRowCellCursor() const
  { return mHandle_Magic == morkMagic_kRowCellCursor; }

  mork_bool IsOrkinRowCellCursorHandle() const
  { return this->IsHandle() && this->IsOrkinRowCellCursor(); }

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
  





  
  NS_IMETHOD SetRow(nsIMdbEnv* ev, nsIMdbRow* ioRow); 
  NS_IMETHOD GetRow(nsIMdbEnv* ev, nsIMdbRow** acqRow);
  

  
  NS_IMETHOD MakeCell( 
    nsIMdbEnv* ev, 
    mdb_column* outColumn, 
    mdb_pos* outPos, 
    nsIMdbCell** acqCell); 
  

  
  NS_IMETHOD SeekCell( 
    nsIMdbEnv* ev, 
    mdb_pos inPos, 
    mdb_column* outColumn, 
    nsIMdbCell** acqCell); 
  

  
  NS_IMETHOD NextCell( 
    nsIMdbEnv* ev, 
    nsIMdbCell* ioCell, 
    mdb_column* outColumn, 
    mdb_pos* outPos); 
    
  NS_IMETHOD PickNextCell( 
    nsIMdbEnv* ev, 
    nsIMdbCell* ioCell, 
    const mdbColumnSet* inFilterSet, 
    mdb_column* outColumn, 
    mdb_pos* outPos); 

  
  
  
  


};



#endif 
