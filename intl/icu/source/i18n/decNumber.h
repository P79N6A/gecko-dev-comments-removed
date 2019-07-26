





























#if !defined(DECNUMBER)
  #define DECNUMBER
  #define DECNAME     "decNumber"                       /* Short name */
  #define DECFULLNAME "Decimal Number Module"         /* Verbose name */
  #define DECAUTHOR   "Mike Cowlishaw"                /* Who to blame */

  #if !defined(DECCONTEXT)
    #include "decContext.h"
  #endif

  
  #define DECNEG    0x80      /* Sign; 1=negative, 0=positive or zero */
  #define DECINF    0x40      /* 1=Infinity                           */
  #define DECNAN    0x20      /* 1=NaN                                */
  #define DECSNAN   0x10      /* 1=sNaN                               */
  
  #define DECSPECIAL (DECINF|DECNAN|DECSNAN) /* any special value     */

  
  
  
  

  


  #define DECDPUN 1           /* DECimal Digits Per UNit [must be >0  */
                              

  
  
  
  
  #if !defined(DECNUMDIGITS)
    #define DECNUMDIGITS 1
  #endif

  
  
  #if   DECDPUN<=2
    #define decNumberUnit uint8_t
  #elif DECDPUN<=4
    #define decNumberUnit uint16_t
  #else
    #define decNumberUnit uint32_t
  #endif
  
  #define DECNUMUNITS ((DECNUMDIGITS+DECDPUN-1)/DECDPUN)

  
  typedef struct {
    int32_t digits;      
    int32_t exponent;    
                         
    uint8_t bits;        
                         
    decNumberUnit lsu[DECNUMUNITS];
    } decNumber;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  


  
  
  
  
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberFromInt32(decNumber *, int32_t);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberFromUInt32(decNumber *, uint32_t);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberFromString(decNumber *, const char *, decContext *);
  U_INTERNAL char      * U_EXPORT2 uprv_decNumberToString(const decNumber *, char *);
  U_INTERNAL char      * U_EXPORT2 uprv_decNumberToEngString(const decNumber *, char *);
  U_INTERNAL uint32_t    U_EXPORT2 uprv_decNumberToUInt32(const decNumber *, decContext *);
  U_INTERNAL int32_t     U_EXPORT2 uprv_decNumberToInt32(const decNumber *, decContext *);
  U_INTERNAL uint8_t   * U_EXPORT2 uprv_decNumberGetBCD(const decNumber *, uint8_t *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberSetBCD(decNumber *, const uint8_t *, uint32_t);

  
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberAbs(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberAdd(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberAnd(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberCompare(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberCompareSignal(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberCompareTotal(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberCompareTotalMag(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberDivide(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberDivideInteger(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberExp(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberFMA(decNumber *, const decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberInvert(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberLn(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberLogB(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberLog10(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberMax(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberMaxMag(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberMin(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberMinMag(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberMinus(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberMultiply(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberNormalize(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberOr(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberPlus(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberPower(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberQuantize(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberReduce(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberRemainder(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberRemainderNear(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberRescale(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberRotate(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberSameQuantum(decNumber *, const decNumber *, const decNumber *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberScaleB(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberShift(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberSquareRoot(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberSubtract(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberToIntegralExact(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberToIntegralValue(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber * U_EXPORT2 uprv_decNumberXor(decNumber *, const decNumber *, const decNumber *, decContext *);

  
  enum decClass uprv_decNumberClass(const decNumber *, decContext *);
  U_INTERNAL const char * U_EXPORT2 uprv_decNumberClassToString(enum decClass);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberCopy(decNumber *, const decNumber *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberCopyAbs(decNumber *, const decNumber *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberCopyNegate(decNumber *, const decNumber *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberCopySign(decNumber *, const decNumber *, const decNumber *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberNextMinus(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberNextPlus(decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberNextToward(decNumber *, const decNumber *, const decNumber *, decContext *);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberTrim(decNumber *);
  U_INTERNAL const char * U_EXPORT2 uprv_decNumberVersion(void);
  U_INTERNAL decNumber  * U_EXPORT2 uprv_decNumberZero(decNumber *);

  
  U_INTERNAL int32_t U_EXPORT2 uprv_decNumberIsNormal(const decNumber *, decContext *);
  U_INTERNAL int32_t U_EXPORT2 uprv_decNumberIsSubnormal(const decNumber *, decContext *);

  
  #define decNumberIsCanonical(dn) (1)  /* All decNumbers are saintly */
  #define decNumberIsFinite(dn)    (((dn)->bits&DECSPECIAL)==0)
  #define decNumberIsInfinite(dn)  (((dn)->bits&DECINF)!=0)
  #define decNumberIsNaN(dn)       (((dn)->bits&(DECNAN|DECSNAN))!=0)
  #define decNumberIsNegative(dn)  (((dn)->bits&DECNEG)!=0)
  #define decNumberIsQNaN(dn)      (((dn)->bits&(DECNAN))!=0)
  #define decNumberIsSNaN(dn)      (((dn)->bits&(DECSNAN))!=0)
  #define decNumberIsSpecial(dn)   (((dn)->bits&DECSPECIAL)!=0)
  #define decNumberIsZero(dn)      (*(dn)->lsu==0 \
                                    && (dn)->digits==1 \
                                    && (((dn)->bits&DECSPECIAL)==0))
  #define decNumberRadix(dn)       (10)

#endif
