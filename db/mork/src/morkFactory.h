




































#ifndef _MORKFACTORY_
#define _MORKFACTORY_ 1

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif

#ifndef _MORKOBJECT_
#include "morkObject.h"
#endif

#ifndef _ORKINHEAP_
#include "orkinHeap.h"
#endif



class nsIMdbFactory;

#define morkDerived_kFactory 0x4663 /* ascii 'Fc' */
#define morkFactory_kWeakRefCountBonus 0 /* try NOT to leak all factories */



class morkFactory : public morkObject, public nsIMdbFactory { 


  
  
  
  
  
  
  
  
  

  
  

public: 

  morkEnv        mFactory_Env; 
  orkinHeap      mFactory_Heap;

  NS_DECL_ISUPPORTS_INHERITED

public: 
  virtual void CloseMorkNode(morkEnv* ev); 
  virtual ~morkFactory(); 




  
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
  


  
public: 
  morkFactory(); 
  morkFactory(nsIMdbHeap* ioHeap); 
  morkFactory(morkEnv* ev, const morkUsage& inUsage, nsIMdbHeap* ioHeap);
  void CloseFactory(morkEnv* ev); 
  
  
public: 
  void* operator new(size_t inSize) CPP_THROW_NEW
  { return ::operator new(inSize); }
  
  void* operator new(size_t inSize, nsIMdbHeap& ioHeap, morkEnv* ev) CPP_THROW_NEW
  { return morkNode::MakeNew(inSize, ioHeap, ev); }
  
private: 
  morkFactory(const morkFactory& other);
  morkFactory& operator=(const morkFactory& other);

public: 
  mork_bool IsFactory() const
  { return IsNode() && mNode_Derived == morkDerived_kFactory; }


public: 

  void NonFactoryTypeError(morkEnv* ev);
  morkEnv* GetInternalFactoryEnv(mdb_err* outErr);
  mork_bool CanOpenMorkTextFile(morkEnv* ev, nsIMdbFile* ioFile);
  
public: 
  static void SlotWeakFactory(morkFactory* me,
    morkEnv* ev, morkFactory** ioSlot)
  { morkNode::SlotWeakNode((morkNode*) me, ev, (morkNode**) ioSlot); }
  
  static void SlotStrongFactory(morkFactory* me,
    morkEnv* ev, morkFactory** ioSlot)
  { morkNode::SlotStrongNode((morkNode*) me, ev, (morkNode**) ioSlot); }
};



#endif 
