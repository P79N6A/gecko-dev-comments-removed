










































#include "nsVerifier.h"
static const PRUint32 UCS2LE_cls [ 256 / 8 ] = {
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,1,0,0,2,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,3,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,3,3,3,3,3,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,4,5)   
};


static const PRUint32 UCS2LE_st [ 7] = {
PCK4BITS(     6,     6,     7,     6,     4,     3,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,     5,     5,     5,eError,eItsMe,eError),
PCK4BITS(     5,     5,     5,eError,     5,eError,     6,     6),
PCK4BITS(     7,     6,     8,     8,     5,     5,     5,eError),
PCK4BITS(     5,     5,     5,eError,eError,eError,     5,     5),
PCK4BITS(     5,     5,     5,eError,     5,eError,eStart,eStart) 
};


static nsVerifier nsUCS2LEVerifier = {
     "UTF-16LE",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       UCS2LE_cls 
    },
    6,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       UCS2LE_st 
    }
};
