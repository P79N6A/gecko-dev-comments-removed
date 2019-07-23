




































#ifndef _MDB_
#include "mdb.h"
#endif

#ifndef _MORK_
#include "mork.h"
#endif

#ifndef _MORKCH_
#include "morkCh.h"
#endif

#ifndef _MORKENV_
#include "morkEnv.h"
#endif









const mork_flags morkCh_Type[] = 
{
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  morkCh_kW,        
  morkCh_kW,        
  morkCh_kW,        
  0,                
  morkCh_kW,        
  morkCh_kW,        
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  0,                
  
  morkCh_kV|morkCh_kW,     
  morkCh_kV|morkCh_kM,     
  morkCh_kV,               
  morkCh_kV,               
  0,                       
  morkCh_kV,               
  morkCh_kV,               
  morkCh_kV,               
  morkCh_kV,               
  0,                       
  morkCh_kV,               
  morkCh_kV|morkCh_kM,     
  morkCh_kV,               
  morkCh_kV|morkCh_kM,     
  morkCh_kV,               
  morkCh_kV,               
  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kD|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM,        
  morkCh_kV,                
  morkCh_kV,                
  morkCh_kV,                
  morkCh_kV,                
  morkCh_kV|morkCh_kM,      
  
  morkCh_kV,                  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kU,          
  morkCh_kV,                
  0,                
  morkCh_kV,                
  morkCh_kV,          
  morkCh_kV|morkCh_kN|morkCh_kM,          
  
  morkCh_kV,                
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL|morkCh_kX,  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV|morkCh_kN|morkCh_kM|morkCh_kL,          
  morkCh_kV,                
  morkCh_kV,                
  morkCh_kV,                
  morkCh_kV,          
  morkCh_kW,          


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  


  0,    0,    0,    0,    0,    0,    0,    0,  
  0,    0,    0,    0,    0,    0,    0,    0,  
};



