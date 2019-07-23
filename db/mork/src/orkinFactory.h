




































#ifndef _ORKINFACTORY_
#define _ORKINFACTORY_ 1

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

#ifndef _MORKFACTORY_
#include "morkFactory.h"
#endif



#define morkMagic_kFactory 0x46616374 /* ascii 'Fact' */



class orkinFactory : public morkHandle, public nsIMdbFactory { 


public: 
  
  virtual ~orkinFactory(); 
  
protected: 
  orkinFactory(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkFactory* ioObject); 
    
  

private: 
  orkinFactory(const morkHandle& other);
  orkinFactory& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
 
public: 

  static orkinFactory* MakeGlobalFactory();
  
  
  static orkinFactory* MakeFactory(morkEnv* ev, morkFactory* ioObject);

public: 

  morkEnv* CanUseFactory(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;
    
  morkEnv* GetInternalFactoryEnv(mdb_err* outErr);
  
  mork_bool CanOpenMorkTextFile(morkEnv* ev,
    
    nsIMdbFile* ioFile);

public: 
  mork_bool IsOrkinFactory() const
  { return mHandle_Magic == morkMagic_kFactory; }

  mork_bool IsOrkinFactoryHandle() const
  { return this->IsHandle() && this->IsOrkinFactory(); }
  
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
  
  




  
  NS_IMETHOD OpenOldFile(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath,
    mdb_bool inFrozen, nsIMdbFile** acqFile);
  
  
  
  
  
  

  NS_IMETHOD CreateNewFile(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    const char* inFilePath,
    nsIMdbFile** acqFile);
  
  
  
  
  
  
  

  
  NS_IMETHOD MakeEnv(nsIMdbHeap* ioHeap, nsIMdbEnv** acqEnv); 
  
  

  
  NS_IMETHOD MakeHeap(nsIMdbEnv* ev, nsIMdbHeap** acqHeap); 
  

  
  NS_IMETHOD MakeCompare(nsIMdbEnv* ev, nsIMdbCompare** acqCompare); 
  

  
  NS_IMETHOD MakeRow(nsIMdbEnv* ev, nsIMdbHeap* ioHeap, nsIMdbRow** acqRow); 
  
  
  
  
  NS_IMETHOD CanOpenFilePort(
    nsIMdbEnv* ev, 
    
    
    nsIMdbFile* ioFile, 
    mdb_bool* outCanOpen, 
    mdbYarn* outFormatVersion); 
    
  NS_IMETHOD OpenFilePort(
    nsIMdbEnv* ev, 
    nsIMdbHeap* ioHeap, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy, 
    nsIMdbThumb** acqThumb); 
  
  

  NS_IMETHOD ThumbToOpenPort( 
    nsIMdbEnv* ev, 
    nsIMdbThumb* ioThumb, 
    nsIMdbPort** acqPort); 
  
  
  
  NS_IMETHOD CanOpenFileStore(
    nsIMdbEnv* ev, 
    
    
    nsIMdbFile* ioFile, 
    mdb_bool* outCanOpenAsStore, 
    mdb_bool* outCanOpenAsPort, 
    mdbYarn* outFormatVersion); 
    
  NS_IMETHOD OpenFileStore( 
    nsIMdbEnv* ev, 
    nsIMdbHeap* ioHeap, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy, 
    nsIMdbThumb** acqThumb); 
  
  
    
  NS_IMETHOD
  ThumbToOpenStore( 
    nsIMdbEnv* ev, 
    nsIMdbThumb* ioThumb, 
    nsIMdbStore** acqStore); 
  
  NS_IMETHOD CreateNewFileStore( 
    nsIMdbEnv* ev, 
    nsIMdbHeap* ioHeap, 
    
    nsIMdbFile* ioFile, 
    const mdbOpenPolicy* inOpenPolicy, 
    nsIMdbStore** acqStore); 
  


};



#endif 
