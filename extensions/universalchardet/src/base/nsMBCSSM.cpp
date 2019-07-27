



#include "nsCodingStateMachine.h"







static const uint32_t EUCJP_cls [ 256 / 8 ] = {

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


static const uint32_t EUCJP_st [ 5] = {
PCK4BITS(     3,     4,     3,     5,eStart,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eStart,eError,eStart,eError,eError,eError),
PCK4BITS(eError,eError,eStart,eError,eError,eError,     3,eError),
PCK4BITS(     3,eError,eError,eError,eStart,eStart,eStart,eStart) 
};

static const uint32_t EUCJPCharLenTable[] = {2, 2, 2, 3, 1, 0};

const SMModel EUCJPSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCJP_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, EUCJP_st },
  CHAR_LEN_TABLE(EUCJPCharLenTable),
  "EUC-JP",
};



static const uint32_t SJIS_cls [ 256 / 8 ] = {

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


static const uint32_t SJIS_st [ 3] = {
PCK4BITS(eError,eStart,eStart,     3,eError,eError,eError,eError),
PCK4BITS(eError,eError,eError,eError,eItsMe,eItsMe,eItsMe,eItsMe),
PCK4BITS(eItsMe,eItsMe,eError,eError,eStart,eStart,eStart,eStart) 
};

static const uint32_t SJISCharLenTable[] = {0, 1, 1, 2, 0, 0};

const SMModel SJISSMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, SJIS_cls },
   6,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, SJIS_st },
  CHAR_LEN_TABLE(SJISCharLenTable),
  "Shift_JIS",
};


static const uint32_t UTF8_cls [ 256 / 8 ] = {
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 0, 0),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 0, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 1, 1, 1, 1, 1, 1, 1, 1),  
PCK4BITS( 2, 2, 2, 2, 2, 2, 2, 2),  
PCK4BITS( 2, 2, 2, 2, 2, 2, 2, 2),  
PCK4BITS( 3, 3, 3, 3, 3, 3, 3, 3),  
PCK4BITS( 3, 3, 3, 3, 3, 3, 3, 3),  
PCK4BITS( 4, 4, 4, 4, 4, 4, 4, 4),  
PCK4BITS( 4, 4, 4, 4, 4, 4, 4, 4),  
PCK4BITS( 4, 4, 4, 4, 4, 4, 4, 4),  
PCK4BITS( 4, 4, 4, 4, 4, 4, 4, 4),  
PCK4BITS( 0, 0, 5, 5, 5, 5, 5, 5),  
PCK4BITS( 5, 5, 5, 5, 5, 5, 5, 5),  
PCK4BITS( 5, 5, 5, 5, 5, 5, 5, 5),  
PCK4BITS( 5, 5, 5, 5, 5, 5, 5, 5),  
PCK4BITS( 6, 7, 7, 7, 7, 7, 7, 7),  
PCK4BITS( 7, 7, 7, 7, 7, 8, 7, 7),  
PCK4BITS( 9,10,10,10,11, 0, 0, 0),  
PCK4BITS( 0, 0, 0, 0, 0, 0, 0, 0)   
};


static const uint32_t UTF8_st [ 15] = {
PCK4BITS(eError,eStart,eError,eError,eError,     3,     4,     5),  
PCK4BITS(     6,     7,     8,     9,eError,eError,eError,eError),  
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),  
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe,eItsMe),  
PCK4BITS(eItsMe,eItsMe,eItsMe,eItsMe,eError,eError,eStart,eStart),  
PCK4BITS(eStart,eError,eError,eError,eError,eError,eError,eError),  
PCK4BITS(eError,eError,eError,eError,     3,eError,eError,eError),  
PCK4BITS(eError,eError,eError,eError,eError,eError,     3,     3),  
PCK4BITS(     3,eError,eError,eError,eError,eError,eError,eError),  
PCK4BITS(eError,eError,     3,     3,eError,eError,eError,eError),  
PCK4BITS(eError,eError,eError,eError,eError,eError,     5,     5),  
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError),  
PCK4BITS(eError,eError,     5,     5,     5,eError,eError,eError),  
PCK4BITS(eError,eError,eError,eError,eError,eError,     5,eError),  
PCK4BITS(eError,eError,eError,eError,eError,eError,eError,eError)   
};

static const uint32_t UTF8CharLenTable[] = {0, 1, 0, 0, 0, 2, 3, 3, 3, 4, 4, 4};

const SMModel UTF8SMModel = {
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UTF8_cls },
   12,
  {eIdxSft4bits, eSftMsk4bits, eBitSft4bits, eUnitMsk4bits, UTF8_st },
  CHAR_LEN_TABLE(UTF8CharLenTable),
  "UTF-8",
};
