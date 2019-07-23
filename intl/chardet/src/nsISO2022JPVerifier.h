










































#include "nsVerifier.h"
static const PRUint32 ISO2022JP_cls [ 256 / 8 ] = {
PCK4BITS(2,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,2,2),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,1,0,0,0,0),  
PCK4BITS(0,0,0,0,7,0,0,0),  
PCK4BITS(3,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(6,0,4,0,8,0,0,0),  
PCK4BITS(0,9,5,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
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
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2)   
};


static const PRUint32 ISO2022JP_st [ 9] = {
PCK4BITS(eStart,     3,eError,eStart,eStart,eStart,eStart,eStart),
PCK4BITS(eStart,eStart,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError,eError),
PCK4BITS(eError,     5,eError,eError,eError,     4,eError,eError),
PCK4BITS(eError,eError,eError,     6,eItsMe,eError,eItsMe,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eItsMe,eItsMe),
PCK4BITS(eError,eError,eError,eItsMe,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eError,eStart,eStart) 
};


static nsVerifier nsISO2022JPVerifier = {
     "ISO-2022-JP",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       ISO2022JP_cls 
    },
    10,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       ISO2022JP_st 
    }
};
