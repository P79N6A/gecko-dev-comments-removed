










































#include "nsVerifier.h"
static const PRUint32 BIG5_cls [ 256 / 8 ] = {
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,0,0),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,0,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,1),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,0)   
};


static const PRUint32 BIG5_st [ 3] = {
PCK4BITS(eError,eStart,eStart,     3,eError,eError,eError,eError),
PCK4BITS(eError,eError,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError),
PCK4BITS(eError,eStart,eStart,eStart,eStart,eStart,eStart,eStart) 
};


static nsVerifier nsBIG5Verifier = {
     "Big5",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       BIG5_cls 
    },
    5,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       BIG5_st 
    }
};
