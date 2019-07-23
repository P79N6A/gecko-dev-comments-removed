










































#include "nsVerifier.h"
static const PRUint32 UTF8_cls [ 256 / 8 ] = {
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,0,0),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,0,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(2,2,2,2,3,3,3,3),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(5,5,5,5,5,5,5,5),  
PCK4BITS(0,0,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(7,8,8,8,8,8,8,8),  
PCK4BITS(8,8,8,8,8,9,8,8),  
PCK4BITS(10,11,11,11,11,11,11,11),  
PCK4BITS(12,13,13,13,14,15,0,0)   
};


static const PRUint32 UTF8_st [ 26] = {
PCK4BITS(eError,eStart,eError,eError,eError,eError,     12,     10),
PCK4BITS(     9,     11,     8,     7,     6,     5,     4,     3),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eError,eError,     5,     5,     5,     5,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,     5,     5,     5,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,     7,     7,     7,     7,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,     7,     7,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,     9,     9,     9,     9,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,     9,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,     12,     12,     12,     12,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,     12,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,     12,     12,     12,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eStart,eStart,eStart,eStart,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError) 
};


static nsVerifier nsUTF8Verifier = {
     "UTF-8",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       UTF8_cls 
    },
    16,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       UTF8_st 
    }
};
