




































#ifndef _ORKINCOMPARE_
#define _ORKINCOMPARE_ 1

#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif





class orkinCompare : public nsIMdbCompare { 
  
public:
  orkinCompare(); 
  virtual ~orkinCompare(); 
    
private: 
  orkinCompare(const orkinCompare& other);
  orkinCompare& operator=(const orkinCompare& other);

public:


  NS_IMETHOD Order(nsIMdbEnv* ev,      
    const mdbYarn* inFirst,   
    const mdbYarn* inSecond,  
    mdb_order* outOrder);     
    
  NS_IMETHOD AddStrongRef(nsIMdbEnv* ev); 
  NS_IMETHOD CutStrongRef(nsIMdbEnv* ev); 


};

extern mdb_order 
mdbYarn_Order(const mdbYarn* inSelf, morkEnv* ev, const mdbYarn* inSecond);



#endif 
