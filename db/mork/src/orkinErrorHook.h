




































#ifndef _ORKINERRORHOOK_
#define _ORKINERRORHOOK_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif





class orkinErrorHook : public nsIMdbErrorHook { 
  
public:
  orkinErrorHook(); 
  virtual ~orkinErrorHook(); 
    
private: 
  orkinErrorHook(const orkinErrorHook& other);
  orkinErrorHook& operator=(const orkinErrorHook& other);

public:


  virtual mdb_err Alloc(nsIMdbEnv* ev, 
    mdb_size inSize,   
    void** outBlock);  
    
  virtual mdb_err Free(nsIMdbEnv* ev, 
    void* inBlock);
    
  virtual mdb_err AddStrongRef(nsIMdbEnv* ev); 
  virtual mdb_err CutStrongRef(nsIMdbEnv* ev); 


};



#endif 
