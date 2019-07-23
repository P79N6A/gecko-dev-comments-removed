



































#include "nsCodingStateMachine.h"









static PRUint32 BIG5_cls [ 256 / 8 ] = {

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


static PRUint32 BIG5_st [ 3] = {
PCK4BITS(eError,eStart,eStart,     3,eError,eError,eError,eError),
PCK4BITS(eError,eError,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError),
PCK4BITS(eError,eStart,eStart,eStart,eStart,eStart,eStart,eStart) 
};

static const PRUint32 Big5CharLenTable[] = {0, 1, 1, 2, 0};

SMModel Big5SMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, BIG5_cls },
    5,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, BIG5_st },
  Big5CharLenTable,
  "Big5",
};

static PRUint32 EUCJP_cls [ 256 / 8 ] = {

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


static PRUint32 EUCJP_st [ 5] = {
PCK4BITS(     3,     4,     3,     5,eStart,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eStart,eError,eStart,eError,eError,eError),
PCK4BITS(eError,eError,eStart,eError,eError,eError,     3,eError),
PCK4BITS(     3,eError,eError,eError,eStart,eStart,eStart,eStart) 
};

static const PRUint32 EUCJPCharLenTable[] = {2, 2, 2, 3, 1, 0};

SMModel EUCJPSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCJP_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCJP_st },
  EUCJPCharLenTable,
  "EUC-JP",
};

static PRUint32 EUCKR_cls [ 256 / 8 ] = {

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
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,0,0,0,0,0,0,0),  
PCK4BITS(0,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,3,3,3),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,3,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,0)   
};


static PRUint32 EUCKR_st [ 2] = {
PCK4BITS(eError,eStart,     3,eError,eError,eError,eError,eError),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eError,eError,eStart,eStart) 
};

static const PRUint32 EUCKRCharLenTable[] = {0, 1, 2, 0};

SMModel EUCKRSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCKR_cls },
  4,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCKR_st },
  EUCKRCharLenTable,
  "EUC-KR",
};

static PRUint32 EUCTW_cls [ 256 / 8 ] = {

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


static PRUint32 EUCTW_st [ 6] = {
PCK4BITS(eError,eError,eStart,     3,     3,     3,     4,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError,eStart,eError),
PCK4BITS(eStart,eStart,eStart,eError,eError,eError,eError,eError),
PCK4BITS(     5,eError,eError,eError,eStart,eError,eStart,eStart),
PCK4BITS(eStart,eError,eStart,eStart,eStart,eStart,eStart,eStart) 
};

static const PRUint32 EUCTWCharLenTable[] = {0, 0, 1, 2, 2, 2, 3};

SMModel EUCTWSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCTW_cls },
   7,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCTW_st },
  EUCTWCharLenTable,
  "x-euc-tw",
};

























































static PRUint32 GB18030_cls [ 256 / 8 ] = {
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


static PRUint32 GB18030_st [ 6] = {
PCK4BITS(eError,eStart,eStart,eStart,eStart,eStart,     3,eError),
PCK4BITS(eError,eError,eError,eError,eError,eError,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eError,eError,eStart),
PCK4BITS(     4,eError,eStart,eStart,eError,eError,eError,eError),
PCK4BITS(eError,eError,     5,eError,eError,eError,eItsMe,eError),
PCK4BITS(eError,eError,eStart,eStart,eStart,eStart,eStart,eStart) 
};






static const PRUint32 GB18030CharLenTable[] = {0, 1, 1, 1, 1, 1, 2};

SMModel GB18030SMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, GB18030_cls },
   7,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, GB18030_st },
  GB18030CharLenTable,
  "GB18030",
};



static PRUint32 SJIS_cls [ 256 / 8 ] = {

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
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,3,3,3),  


PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(2,2,2,2,2,2,2,2),  
PCK4BITS(3,3,3,3,3,3,3,3),  
PCK4BITS(3,3,3,3,3,4,4,4),  
PCK4BITS(4,4,4,4,4,4,4,4),  
PCK4BITS(4,4,4,4,4,0,0,0)   
};


static PRUint32 SJIS_st [ 3] = {
PCK4BITS(eError,eStart,eStart,     3,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eError,eError,eStart,eStart,eStart,eStart) 
};

static const PRUint32 SJISCharLenTable[] = {0, 1, 1, 2, 0, 0};

SMModel SJISSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, SJIS_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, SJIS_st },
  SJISCharLenTable,
  "Shift_JIS",
};


static PRUint32 UCS2BE_cls [ 256 / 8 ] = {
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


static PRUint32 UCS2BE_st [ 7] = {
PCK4BITS(     5,     7,     7,eError,     4,     3,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,     6,     6,     6,     6,eError,eError),
PCK4BITS(     6,     6,     6,     6,     6,eItsMe,     6,     6),
PCK4BITS(     6,     6,     6,     6,     5,     7,     7,eError),
PCK4BITS(     5,     8,     6,     6,eError,     6,     6,     6),
PCK4BITS(     6,     6,     6,     6,eError,eError,eStart,eStart) 
};

static const PRUint32 UCS2BECharLenTable[] = {2, 2, 2, 0, 2, 2};

SMModel UCS2BESMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UCS2BE_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UCS2BE_st },
  UCS2BECharLenTable,
  "UTF-16BE",
};

static PRUint32 UCS2LE_cls [ 256 / 8 ] = {
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


static PRUint32 UCS2LE_st [ 7] = {
PCK4BITS(     6,     6,     7,     6,     4,     3,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,     5,     5,     5,eError,eItsMe,eError),
PCK4BITS(     5,     5,     5,eError,     5,eError,     6,     6),
PCK4BITS(     7,     6,     8,     8,     5,     5,     5,eError),
PCK4BITS(     5,     5,     5,eError,eError,eError,     5,     5),
PCK4BITS(     5,     5,     5,eError,     5,eError,eStart,eStart) 
};

static const PRUint32 UCS2LECharLenTable[] = {2, 2, 2, 2, 2, 2};

SMModel UCS2LESMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UCS2LE_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UCS2LE_st },
  UCS2LECharLenTable,
  "UTF-16LE",
};


static PRUint32 UTF8_cls [ 256 / 8 ] = {

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


static PRUint32 UTF8_st [ 26] = {
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

static const PRUint32 UTF8CharLenTable[] = {0, 1, 0, 0, 0, 0, 2, 3, 
                            3, 3, 4, 4, 5, 5, 6, 6 };

SMModel UTF8SMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UTF8_cls },
   16,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UTF8_st },
  UTF8CharLenTable,
  "UTF-8",
};

