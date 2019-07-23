




































#ifndef _ORKINHEAP_
#define _ORKINHEAP_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif



#define orkinHeap_kTag 0x68456150 /* ascii 'hEaP' */



class orkinHeap : public nsIMdbHeap { 

#ifdef MORK_DEBUG_HEAP_STATS
protected:
  mork_num sHeap_AllocCount;  
  mork_num sHeap_FreeCount;   
  mork_num sHeap_BlockCount;  
  
  mork_num sHeap_BlockVolume; 
  mork_num sHeap_HighWaterVolume;  
  mork_num sHeap_HighWaterTenKilo; 
  mork_num sHeap_HighWaterHundredKilo; 
  
public: 
  mork_num HeapAllocCount() const { return sHeap_AllocCount; }
  mork_num HeapFreeCount() const { return sHeap_FreeCount; }
  mork_num HeapBlockCount() const { return sHeap_AllocCount - sHeap_FreeCount; }
  
  mork_num HeapBlockVolume() const { return sHeap_BlockVolume; }
  mork_num HeapHighWaterVolume() const { return sHeap_HighWaterVolume; }
#endif 
  
public:
  orkinHeap(); 
  virtual ~orkinHeap(); 
    
private: 
  orkinHeap(const orkinHeap& other);
  orkinHeap& operator=(const orkinHeap& other);

public:


  NS_IMETHOD Alloc(nsIMdbEnv* ev, 
    mdb_size inSize,   
    void** outBlock);  
    
  NS_IMETHOD Free(nsIMdbEnv* ev, 
    void* inBlock);
    
  NS_IMETHOD HeapAddStrongRef(nsIMdbEnv* ev); 
  NS_IMETHOD HeapCutStrongRef(nsIMdbEnv* ev); 


};



#endif 
