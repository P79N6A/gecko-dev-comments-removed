




































#ifndef _ORKINSORTING_
#define _ORKINSORTING_ 1

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

#ifndef _MORKSORTING_
#include "morkSorting.h"
#endif



#define morkMagic_kSorting 0x536F7274 /* ascii 'Sort' */



class orkinSorting : public morkHandle, public nsIMdbSorting { 


public: 
  
  virtual ~orkinSorting(); 
  
protected: 
  orkinSorting(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkSorting* ioObject); 
    
  

private: 
  orkinSorting(const orkinSorting& other);
  orkinSorting& operator=(const orkinSorting& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
  
public: 

  static orkinSorting* MakeSorting(morkEnv* ev, morkSorting* ioObject);

public: 

  morkEnv* CanUseSorting(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinSorting() const
  { return mHandle_Magic == morkMagic_kSorting; }

  mork_bool IsOrkinSortingHandle() const
  { return this->IsHandle() && this->IsOrkinSorting(); }

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
  
  




  
  
  
  
  NS_IMETHOD GetTable(nsIMdbEnv* ev, nsIMdbTable** acqTable);
  NS_IMETHOD GetSortColumn( 
    nsIMdbEnv* ev, 
    mdb_column* outColumn); 

  NS_IMETHOD SetNewCompare(nsIMdbEnv* ev,
    nsIMdbCompare* ioNewCompare);
    
    
    
    
    
    

  NS_IMETHOD GetOldCompare(nsIMdbEnv* ev,
    nsIMdbCompare** acqOldCompare);
    
    
    
    
    
    
  
  

  
  NS_IMETHOD GetSortingRowCursor( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbTableRowCursor** acqCursor); 
    
  

  
  NS_IMETHOD PosToOid( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    mdbOid* outOid); 
    
  NS_IMETHOD PosToRow( 
    nsIMdbEnv* ev, 
    mdb_pos inRowPos, 
    nsIMdbRow** acqRow); 
  



};
 


#endif 
