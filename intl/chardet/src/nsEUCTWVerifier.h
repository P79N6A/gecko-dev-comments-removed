










































#include "nsVerifier.h"
static const PRUint32 EUCTW_cls [ 256 / 8 ] = {
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
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,6,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,3,4,4,4,4,4,4),  
PCK4BITS(5,5,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,3,1,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,0)   
};


static const PRUint32 EUCTW_st [ 6] = {
PCK4BITS(eError,eError,eStart,     3,     3,     3,     4,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError,eStart,eError),
PCK4BITS(eStart,eStart,eStart,eError,eError,eError,eError,eError),
PCK4BITS(     5,eError,eError,eError,eStart,eError,eStart,eStart),
PCK4BITS(eStart,eError,eStart,eStart,eStart,eStart,eStart,eStart) 
};


static nsVerifier nsEUCTWVerifier = {
     "x-euc-tw",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       EUCTW_cls 
    },
    7,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       EUCTW_st 
    }
};
