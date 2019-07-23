




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _ORKINCOMPARE_
#include "orkinCompare.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif




orkinCompare::orkinCompare() 
{
}


orkinCompare::~orkinCompare() 
{
}


mdb_order 
mdbYarn_Order(const mdbYarn* inSelf, morkEnv* ev, const mdbYarn* inYarn)
{
  
  
  
  mork_u1* self = (mork_u1*) inSelf->mYarn_Buf;
  mork_u1* yarn = (mork_u1*) inYarn->mYarn_Buf;
  
  mdb_fill selfFill = inSelf->mYarn_Fill;
  mdb_fill yarnFill = inYarn->mYarn_Fill;
  
  if ( selfFill && yarnFill ) 
  {
    register int a; 
    register int b; 
    
    
    ++selfFill; 
    ++yarnFill; 
    
    
    while ( --selfFill ) 
    {
      if ( !--yarnFill ) 
        return 1;
      
      b = (mork_u1) *yarn++;  
      a = (mork_u1) *self++;  

      if ( a != b ) 
        return ( a - b ); 
    }
    
    return ( yarnFill == 1 )? 0 : -1; 
  }
  else
    return ((mdb_order) selfFill) - ((mdb_order) yarnFill);
}


 mdb_err
orkinCompare::Order(nsIMdbEnv* mev, 
  const mdbYarn* inFirst,   
  const mdbYarn* inSecond,  
  mdb_order* outOrder)      
{
  mdb_err outErr = 1; 

  morkEnv* ev = morkEnv::FromMdbEnv(mev);
  if ( ev )
  {
    if ( inFirst && inSecond && outOrder )
    {
      *outOrder = mdbYarn_Order(inFirst, ev, inSecond);
    }
    else
      ev->NilPointerError();

    outErr = ev->AsErr();
  }
  return outErr;
}

 mdb_err
orkinCompare::AddStrongRef(nsIMdbEnv* ev) 
{
  MORK_USED_1(ev);
  return 0;
}

 mdb_err
orkinCompare::CutStrongRef(nsIMdbEnv* ev) 
{
  MORK_USED_1(ev);
  return 0;
}




