



































#include "nsCodingStateMachine.h"

static PRUint32 HZ_cls[ 256 / 8 ] = {
PCK4BITS(1,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,1,0,0,0,0),  
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
PCK4BITS(0,0,0,4,0,5,2,0),  
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
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1),  
PCK4BITS(1,1,1,1,1,1,1,1)   
};


static PRUint32 HZ_st [ 6] = {
PCK4BITS(eStart,eError,     3,eStart,eStart,eStart,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eError,eError,eStart,eStart,     4,eError),
PCK4BITS(     5,eError,     6,eError,     5,     5,     4,eError),
PCK4BITS(     4,eError,     4,     4,     4,eError,     4,eError),
PCK4BITS(     4,eItsMe,eStart,eStart,eStart,eStart,eStart,eStart) 
};

static const PRUint32 HZCharLenTable[] = {0, 0, 0, 0, 0, 0};

SMModel HZSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, HZ_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, HZ_st },
  HZCharLenTable,
  "HZ-GB-2312",
};


static PRUint32 ISO2022CN_cls [ 256 / 8 ] = {
PCK4BITS(2,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,1,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,3,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,4,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
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


static PRUint32 ISO2022CN_st [ 8] = {
PCK4BITS(eStart,     3,eError,eStart,eStart,eStart,eStart,eStart),
PCK4BITS(eStart,eError,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eError,eError,eError,     4,eError),
PCK4BITS(eError,eError,eError,eItsMe,eError,eError,eError,eError),
PCK4BITS(     5,     6,eError,eError,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eItsMe,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eError,eItsMe,eError,eStart) 
};

static const PRUint32 ISO2022CNCharLenTable[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

SMModel ISO2022CNSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022CN_cls },
  9,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022CN_st },
  ISO2022CNCharLenTable,
  "ISO-2022-CN",
};

static PRUint32 ISO2022JP_cls [ 256 / 8 ] = {
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


static PRUint32 ISO2022JP_st [ 9] = {
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

static const PRUint32 ISO2022JPCharLenTable[] = {0, 0, 0, 0, 0, 0, 0, 0};

SMModel ISO2022JPSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022JP_cls },
  10,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022JP_st },
  ISO2022JPCharLenTable,
  "ISO-2022-JP",
};

static PRUint32 ISO2022KR_cls [ 256 / 8 ] = {
PCK4BITS(2,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,1,0,0,0,0),  
PCK4BITS(0,0,0,0,3,0,0,0),  
PCK4BITS(0,4,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,5,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
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


static PRUint32 ISO2022KR_st [ 5] = {
PCK4BITS(eStart,     3,eError,eStart,eStart,eStart,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eError,eError,eError,     4,eError,eError),
PCK4BITS(eError,eError,eError,eError,     5,eError,eError,eError),
PCK4BITS(eError,eError,eError,eItsMe,eStart,eStart,eStart,eStart) 
};

static const PRUint32 ISO2022KRCharLenTable[] = {0, 0, 0, 0, 0, 0};

SMModel ISO2022KRSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022KR_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, ISO2022KR_st },
  ISO2022KRCharLenTable,
  "ISO-2022-KR",
};

