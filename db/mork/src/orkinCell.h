




































#ifndef _ORKINCELL_
#define _ORKINCELL_ 1

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

#ifndef _MORKCELL_
#include "morkCell.h"
#endif



#define morkMagic_kCell 0x43656C6C /* ascii 'Cell' */

class orkinCell : public morkHandle, public nsIMdbCell { 


public: 
  
  virtual ~orkinCell(); 
  
protected: 
  orkinCell(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkCellObject* ioObject); 
    
  

private: 
  orkinCell(const morkHandle& other);
  orkinCell& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
  
public: 

  static orkinCell* MakeCell(morkEnv* ev, morkCellObject* ioObject);

public: 

  
  

  morkEnv* CanUseCell(nsIMdbEnv* ev, mork_bool inMutable,
    mdb_err* outErr, morkCell** outCell) const;

public: 
  mork_bool IsOrkinCell() const
  { return mHandle_Magic == morkMagic_kCell; }

  mork_bool IsOrkinCellHandle() const
  { return this->IsHandle() && this->IsOrkinCell(); }

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
  
  




  
  NS_IMETHOD SetBlob(nsIMdbEnv* ev,
    nsIMdbBlob* ioBlob); 
  
  
  NS_IMETHOD ClearBlob( 
    nsIMdbEnv* ev);
  
  
  NS_IMETHOD GetBlobFill(nsIMdbEnv* ev,
    mdb_fill* outFill);  
  
  
  
  NS_IMETHOD SetYarn(nsIMdbEnv* ev, 
    const mdbYarn* inYarn);   
  
  
  NS_IMETHOD GetYarn(nsIMdbEnv* ev, 
    mdbYarn* outYarn);  
  
  
  NS_IMETHOD AliasYarn(nsIMdbEnv* ev, 
    mdbYarn* outYarn); 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  





  
  NS_IMETHOD SetColumn(nsIMdbEnv* ev, mdb_column inColumn); 
  NS_IMETHOD GetColumn(nsIMdbEnv* ev, mdb_column* outColumn);
  
  NS_IMETHOD GetCellInfo(  
    nsIMdbEnv* ev, 
    mdb_column* outColumn,           
    mdb_fill*   outBlobFill,         
    mdbOid*     outChildOid,         
    mdb_bool*   outIsRowChild);  

  
  
  
  NS_IMETHOD GetRow(nsIMdbEnv* ev, 
    nsIMdbRow** acqRow);
  NS_IMETHOD GetPort(nsIMdbEnv* ev, 
    nsIMdbPort** acqPort);
  

  
  NS_IMETHOD HasAnyChild( 
    nsIMdbEnv* ev,
    mdbOid* outOid,  
    mdb_bool* outIsRow); 

  NS_IMETHOD GetAnyChild( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow, 
    nsIMdbTable** acqTable); 


  NS_IMETHOD SetChildRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow* ioRow); 

  NS_IMETHOD GetChildRow( 
    nsIMdbEnv* ev, 
    nsIMdbRow** acqRow); 


  NS_IMETHOD SetChildTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable* inTable); 

  NS_IMETHOD GetChildTable( 
    nsIMdbEnv* ev, 
    nsIMdbTable** acqTable); 
  


};



#endif 
