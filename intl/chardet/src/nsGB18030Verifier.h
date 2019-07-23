










































#include "nsVerifier.h"
static const PRUint32 gb18030_cls [ 256 / 8 ] = {
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,0,0),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,0,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,1,1,1,1,1,1),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,4),  
PCK4BITS(5,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,6),  
PCK4BITS(6,6,6,6,6,6,6,0)   
};


static const PRUint32 gb18030_st [ 6] = {
PCK4BITS(eError,eStart,eStart,eStart,eStart,eStart,     3,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError,eError,eStart),
PCK4BITS(     4,eError,eStart,eStart,eError,eError,eError,eError),
PCK4BITS(eError,eError,     5,eError,eError,eError,eItsMe,eError),
PCK4BITS(eError,eError,eStart,eStart,eStart,eStart,eStart,eStart) 
};


static nsVerifier nsGB18030Verifier = {
     "gb18030",
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       gb18030_cls 
    },
    7,
    {
       eIdxSft4bits, 
       eSftMsk4bits, 
       eBitSft4bits, 
       eUnitMsk4bits, 
       gb18030_st 
    }
};
