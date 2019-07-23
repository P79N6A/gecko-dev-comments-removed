




































#ifndef _ORKINROW_
#define _ORKINROW_ 1

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

#ifndef _MORKROW_
#include "morkRow.h"
#endif



#define morkMagic_kRow 0x526F774D /* ascii 'RowM' */




class orkinRow : public morkHandle, public nsIMdbRow { 


public: 
  
  virtual ~orkinRow(); 
  
protected: 
  orkinRow(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkRowObject* ioObject); 
    
  

private: 
  orkinRow(const morkHandle& other);
  orkinRow& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
  
public: 

  static orkinRow* MakeRow(morkEnv* ev, morkRowObject* ioObject);

public: 

  morkEnv* CanUseRow(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr, morkRow** outRow) const;

  morkStore* CanUseRowStore(morkEnv* ev) const;

public: 
  mork_bool IsOrkinRow() const
  { return mHandle_Magic == morkMagic_kRow; }

  mork_bool IsOrkinRowHandle() const
  { return this->IsHandle() && this->IsOrkinRow(); }

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
  
  




  
  NS_IMETHOD GetSeed(nsIMdbEnv* ev,
    mdb_seed* outSeed);    
  NS_IMETHOD GetCount(nsIMdbEnv* ev,
    mdb_count* outCount); 

  NS_IMETHOD GetPort(nsIMdbEnv* ev,
    nsIMdbPort** acqPort); 
  

  
  NS_IMETHOD GetCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inMemberPos, 
    nsIMdbCursor** acqCursor); 
  

  
  NS_IMETHOD GetOid(nsIMdbEnv* ev,
    mdbOid* outOid); 
  NS_IMETHOD BecomeContent(nsIMdbEnv* ev,
    const mdbOid* inOid); 
  

  
  NS_IMETHOD DropActivity( 
    nsIMdbEnv* ev);
  





  
  NS_IMETHOD GetRowCellCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbRowCellCursor** acqCursor); 
  

  
  NS_IMETHOD AddColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    const mdbYarn* inYarn); 

  NS_IMETHOD CutColumn( 
    nsIMdbEnv* ev, 
    mdb_column inColumn); 

  NS_IMETHOD CutAllColumns( 
    nsIMdbEnv* ev); 
  

  
  NS_IMETHOD NewCell( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbCell** acqCell); 
    
  NS_IMETHOD AddCell( 
    nsIMdbEnv* ev, 
    const nsIMdbCell* inCell); 
    
  NS_IMETHOD GetCell( 
    nsIMdbEnv* ev, 
    mdb_column inColumn, 
    nsIMdbCell** acqCell); 
    
  NS_IMETHOD EmptyAllCells( 
    nsIMdbEnv* ev); 
  

  
  NS_IMETHOD AddRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioSourceRow); 
    
  NS_IMETHOD SetRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioSourceRow); 
  

  
  NS_IMETHOD SetCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    const mdbYarn* inYarn);   
  
  
  NS_IMETHOD GetCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdbYarn* outYarn);  
  
  
  NS_IMETHOD AliasCellYarn(nsIMdbEnv* ev, 
    mdb_column inColumn, 
    mdbYarn* outYarn); 
  
  NS_IMETHOD NextCellYarn(nsIMdbEnv* ev, 
    mdb_column* ioColumn, 
    mdbYarn* outYarn);  
  
  
  
  
  
  
  
  
  
  
  
  

  NS_IMETHOD SeekCellYarn( 
    nsIMdbEnv* ev, 
    mdb_pos inPos, 
    mdb_column* outColumn, 
    mdbYarn* outYarn); 
  
  
  
  

  


};



#endif 

