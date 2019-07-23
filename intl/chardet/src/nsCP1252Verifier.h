










































#include "nsVerifier.h"
static const PRUint32 CP1252_cls [ 256 / 8 ] = {
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,0,0),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,0,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,0,2,2,2,2,2,2),  
PCK4BITS(2,2,1,2,1,0,1,0),  
PCK4BITS(0,2,2,2,2,2,2,2),  
PCK4BITS(2,2,1,2,1,0,1,1),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,2),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,2),  
PCK4BITS(1,1,1,1,1,1,1,1)   
};


static const PRUint32 CP1252_st [ 3] = {
PCK4BITS(eError,     3,eStart,eError,eError,eError,eItsMe,eItsMe),
PCK4BITS(eItsMe,eError,     4,eStart,eError,     5,     4,eError),
PCK4BITS(eError,     4,eStart,eStart,eStart,eStart,eStart,eStart) 
};


static nsVerifier nsCP1252Verifier = {
     "windows-1252",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       CP1252_cls 
    },
    3,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       CP1252_st 
    }
};
