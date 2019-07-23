










































#include "nsVerifier.h"
static const PRUint32 EUCJP_cls [ 256 / 8 ] = {
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,5,5),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,5,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(5,5,5,5,5,5,1,3),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(5,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,5)   
};


static const PRUint32 EUCJP_st [ 5] = {
PCK4BITS(     3,     4,     3,     5,eStart,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eStart,eError,eStart,eError,eError,eError),
PCK4BITS(eError,eError,eStart,eError,eError,eError,     3,eError),
PCK4BITS(     3,eError,eError,eError,eStart,eStart,eStart,eStart) 
};


static nsVerifier nsEUCJPVerifier = {
     "EUC-JP",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       EUCJP_cls 
    },
    6,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       EUCJP_st 
    }
};
