




































#ifndef _ORKINTABLE_
#define _ORKINTABLE_ 1

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

#ifndef _MORKFILE_
#include "morkFile.h"
#endif



#define morkMagic_kFile 0x46696C65 /* ascii 'File' */



class orkinFile : public morkHandle, public nsIMdbFile { 


public: 
  
  virtual ~orkinFile(); 
  
protected: 
  orkinFile(morkEnv* ev, 
    morkHandleFace* ioFace,    
    morkFile* ioObject); 
    
  

private: 
  orkinFile(const morkHandle& other);
  orkinFile& operator=(const morkHandle& other);


  
  


protected: 
  void* operator new(size_t inSize, morkPool& ioPool, morkZone& ioZone, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, &ioZone); }
  
  void* operator new(size_t inSize, morkPool& ioPool, morkEnv* ev) CPP_THROW_NEW
  { return ioPool.NewHandle(ev, inSize, (morkZone*) 0); }
  
  void* operator new(size_t inSize, morkHandleFace* ioFace) CPP_THROW_NEW
  { MORK_USED_1(inSize); return ioFace; }
  
 
public: 

  static orkinFile* MakeFile(morkEnv* ev, morkFile* ioObject);

public: 

  morkEnv* CanUseFile(nsIMdbEnv* mev, mork_bool inMutable,
    mdb_err* outErr) const;

public: 
  mork_bool IsOrkinFile() const
  { return mHandle_Magic == morkMagic_kFile; }

  mork_bool IsOrkinFileHandle() const
  { return this->IsHandle() && this->IsOrkinFile(); }

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
  
  




  
  NS_IMETHOD Tell(nsIMdbEnv* ev, mdb_pos* outPos);
  NS_IMETHOD Seek(nsIMdbEnv* ev, mdb_pos inPos);
  NS_IMETHOD Eof(nsIMdbEnv* ev, mdb_pos* outPos);
  

  
  NS_IMETHOD Read(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_size* outActualSize);
  NS_IMETHOD Get(nsIMdbEnv* ev, void* outBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize);
  
    
  
  NS_IMETHOD  Write(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_size* outActualSize);
  NS_IMETHOD  Put(nsIMdbEnv* ev, const void* inBuf, mdb_size inSize,
    mdb_pos inPos, mdb_size* outActualSize);
  NS_IMETHOD  Flush(nsIMdbEnv* ev);
  
    
  
  NS_IMETHOD  Path(nsIMdbEnv* ev, mdbYarn* outFilePath);
  
    
  
  NS_IMETHOD  Steal(nsIMdbEnv* ev, nsIMdbFile* ioThief);
  NS_IMETHOD  Thief(nsIMdbEnv* ev, nsIMdbFile** acqThief);
  

  
  NS_IMETHOD BecomeTrunk(nsIMdbEnv* ev);
  
  
  
  
  
  
  

  NS_IMETHOD AcquireBud(nsIMdbEnv* ev, nsIMdbHeap* ioHeap,
    nsIMdbFile** acqBud); 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


};
 


#endif 
