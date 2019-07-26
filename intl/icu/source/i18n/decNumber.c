















































































































































































#include <stdlib.h>                
        
#include <string.h>                
#include <ctype.h>                 
#include "cmemory.h"               
#include "decNumber.h"             
#include "decNumberLocal.h"        
#include "uassert.h"



static const uByte d2utable[DECMAXD2U+1]=D2UTABLE;

#define DECVERB     1              /* set to 1 for verbose DECCHECK  */
#define powers      DECPOWERS      /* old internal name  */


#define DIVIDE      0x80           /* Divide operators  */
#define REMAINDER   0x40           /* ..  */
#define DIVIDEINT   0x20           /* ..  */
#define REMNEAR     0x10           /* ..  */
#define COMPARE     0x01           /* Compare operators  */
#define COMPMAX     0x02           /* ..  */
#define COMPMIN     0x03           /* ..  */
#define COMPTOTAL   0x04           /* ..  */
#define COMPNAN     0x05           /* .. [NaN processing]  */
#define COMPSIG     0x06           /* .. [signaling COMPARE]  */
#define COMPMAXMAG  0x07           /* ..  */
#define COMPMINMAG  0x08           /* ..  */

#define DEC_sNaN     0x40000000    /* local status: sNaN signal  */
#define BADINT  (Int)0x80000000    /* most-negative Int; error indicator  */

#define BIGEVEN (Int)0x80000002
#define BIGODD  (Int)0x80000003

static const Unit uarrone[1]={1};   




static const uByte DECSTICKYTAB[10]={1,1,2,3,4,6,6,7,8,9}; 




static const uInt DECPOWERS[10]={1, 10, 100, 1000, 10000, 100000, 1000000,
                          10000000, 100000000, 1000000000};



#if DECDPUN<=4
  #define eInt  Int           /* extended integer  */
  #define ueInt uInt          /* unsigned extended integer  */
  
  
  
  static const uInt multies[]={131073, 26215, 5243, 1049, 210};
  
  #define QUOT10(u, n) ((((uInt)(u)>>(n))*multies[n])>>17)
#else
  
  #if !DECUSE64
    #error decNumber.c: DECUSE64 must be 1 when DECDPUN>4
  #endif
  #define eInt  Long          /* extended integer  */
  #define ueInt uLong         /* unsigned extended integer  */
#endif


static decNumber * decAddOp(decNumber *, const decNumber *, const decNumber *,
                              decContext *, uByte, uInt *);
static Flag        decBiStr(const char *, const char *, const char *);
static uInt        decCheckMath(const decNumber *, decContext *, uInt *);
static void        decApplyRound(decNumber *, decContext *, Int, uInt *);
static Int         decCompare(const decNumber *lhs, const decNumber *rhs, Flag);
static decNumber * decCompareOp(decNumber *, const decNumber *,
                              const decNumber *, decContext *,
                              Flag, uInt *);
static void        decCopyFit(decNumber *, const decNumber *, decContext *,
                              Int *, uInt *);
static decNumber * decDecap(decNumber *, Int);
static decNumber * decDivideOp(decNumber *, const decNumber *,
                              const decNumber *, decContext *, Flag, uInt *);
static decNumber * decExpOp(decNumber *, const decNumber *,
                              decContext *, uInt *);
static void        decFinalize(decNumber *, decContext *, Int *, uInt *);
static Int         decGetDigits(Unit *, Int);
static Int         decGetInt(const decNumber *);
static decNumber * decLnOp(decNumber *, const decNumber *,
                              decContext *, uInt *);
static decNumber * decMultiplyOp(decNumber *, const decNumber *,
                              const decNumber *, decContext *,
                              uInt *);
static decNumber * decNaNs(decNumber *, const decNumber *,
                              const decNumber *, decContext *, uInt *);
static decNumber * decQuantizeOp(decNumber *, const decNumber *,
                              const decNumber *, decContext *, Flag,
                              uInt *);
static void        decReverse(Unit *, Unit *);
static void        decSetCoeff(decNumber *, decContext *, const Unit *,
                              Int, Int *, uInt *);
static void        decSetMaxValue(decNumber *, decContext *);
static void        decSetOverflow(decNumber *, decContext *, uInt *);
static void        decSetSubnormal(decNumber *, decContext *, Int *, uInt *);
static Int         decShiftToLeast(Unit *, Int, Int);
static Int         decShiftToMost(Unit *, Int, Int);
static void        decStatus(decNumber *, uInt, decContext *);
static void        decToString(const decNumber *, char[], Flag);
static decNumber * decTrim(decNumber *, decContext *, Flag, Flag, Int *);
static Int         decUnitAddSub(const Unit *, Int, const Unit *, Int, Int,
                              Unit *, Int);
static Int         decUnitCompare(const Unit *, Int, const Unit *, Int, Int);

#if !DECSUBSET

#define decFinish(a,b,c,d) decFinalize(a,b,c,d)
#else
static void        decFinish(decNumber *, decContext *, Int *, uInt *);
static decNumber * decRoundOperand(const decNumber *, decContext *, uInt *);
#endif



#define SPECIALARG  (rhs->bits & DECSPECIAL)
#define SPECIALARGS ((lhs->bits | rhs->bits) & DECSPECIAL)


#define malloc(a) uprv_malloc(a)
#define free(a) uprv_free(a)


#if DECALLOC



#define malloc(a) decMalloc(a)
#define free(a) decFree(a)
#define DECFENCE 0x5a              /* corruption detector  */

static void *decMalloc(size_t);
static void  decFree(void *);
uInt decAllocBytes=0;              





#endif

#if DECCHECK








#define DECUNRESU ((decNumber *)(void *)0xffffffff)
#define DECUNUSED ((const decNumber *)(void *)0xffffffff)
#define DECUNCONT ((decContext *)(void *)(0xffffffff))
static Flag decCheckOperands(decNumber *, const decNumber *,
                             const decNumber *, decContext *);
static Flag decCheckNumber(const decNumber *);
static void decCheckInexact(const decNumber *, decContext *);
#endif

#if DECTRACE || DECCHECK

void decNumberShow(const decNumber *);  
static void decDumpAr(char, const Unit *, Int);
#endif














U_CAPI decNumber * U_EXPORT2 uprv_decNumberFromInt32(decNumber *dn, Int in) {
  uInt unsig;
  if (in>=0) unsig=in;
   else {                               
    if (in==BADINT) unsig=(uInt)1073741824*2; 
     else unsig=-in;                    
    }
  
  uprv_decNumberFromUInt32(dn, unsig);
  if (in<0) dn->bits=DECNEG;            
  return dn;
  } 

U_CAPI decNumber * U_EXPORT2 uprv_decNumberFromUInt32(decNumber *dn, uInt uin) {
  Unit *up;                             
  uprv_decNumberZero(dn);                    
  if (uin==0) return dn;                
  for (up=dn->lsu; uin>0; up++) {
    *up=(Unit)(uin%(DECDPUNMAX+1));
    uin=uin/(DECDPUNMAX+1);
    }
  dn->digits=decGetDigits(dn->lsu, up-dn->lsu);
  return dn;
  } 











U_CAPI Int U_EXPORT2 uprv_decNumberToInt32(const decNumber *dn, decContext *set) {
  #if DECCHECK
  if (decCheckOperands(DECUNRESU, DECUNUSED, dn, set)) return 0;
  #endif

  
  if (dn->bits&DECSPECIAL || dn->digits>10 || dn->exponent!=0) ; 
   else { 
    Int d;                         
    const Unit *up;                
    uInt hi=0, lo;                 
    up=dn->lsu;                    
    lo=*up;                        
    #if DECDPUN>1                  
      hi=lo/10;
      lo=lo%10;
    #endif
    up++;
    
    for (d=DECDPUN; d<dn->digits; up++, d+=DECDPUN) hi+=*up*powers[d-1];
    
    if (hi>214748364 || (hi==214748364 && lo>7)) { 
      
      if (dn->bits&DECNEG && hi==214748364 && lo==8) return 0x80000000;
      
      }
     else { 
      Int i=X10(hi)+lo;
      if (dn->bits&DECNEG) return -i;
      return i;
      }
    } 
  uprv_decContextSetStatus(set, DEC_Invalid_operation); 
  return 0;
  } 

U_CAPI uInt U_EXPORT2 uprv_decNumberToUInt32(const decNumber *dn, decContext *set) {
  #if DECCHECK
  if (decCheckOperands(DECUNRESU, DECUNUSED, dn, set)) return 0;
  #endif
  
  if (dn->bits&DECSPECIAL || dn->digits>10 || dn->exponent!=0
    || (dn->bits&DECNEG && !ISZERO(dn)));                   
   else { 
    Int d;                         
    const Unit *up;                
    uInt hi=0, lo;                 
    up=dn->lsu;                    
    lo=*up;                        
    #if DECDPUN>1                  
      hi=lo/10;
      lo=lo%10;
    #endif
    up++;
    
    for (d=DECDPUN; d<dn->digits; up++, d+=DECDPUN) hi+=*up*powers[d-1];

    
    if (hi>429496729 || (hi==429496729 && lo>5)) ; 
     else return X10(hi)+lo;
    } 
  uprv_decContextSetStatus(set, DEC_Invalid_operation); 
  return 0;
  } 















U_CAPI char * U_EXPORT2 uprv_decNumberToString(const decNumber *dn, char *string){
  decToString(dn, string, 0);
  return string;
  } 

U_CAPI char * U_EXPORT2 uprv_decNumberToEngString(const decNumber *dn, char *string){
  decToString(dn, string, 1);
  return string;
  } 





















U_CAPI decNumber * U_EXPORT2 uprv_decNumberFromString(decNumber *dn, const char chars[],
                                decContext *set) {
  Int   exponent=0;                
  uByte bits=0;                    
  Unit  *res;                      
  Unit  resbuff[SD2U(DECBUFFER+9)];
                                   
  Unit  *allocres=NULL;            
  Int   d=0;                       
  const char *dotchar=NULL;        
  const char *cfirst=chars;        
  const char *last=NULL;           
  const char *c;                   
  Unit  *up;                       
  #if DECDPUN>1
  Int   cut, out;                  
  #endif
  Int   residue;                   
  uInt  status=0;                  

  #if DECCHECK
  if (decCheckOperands(DECUNRESU, DECUNUSED, DECUNUSED, set))
    return uprv_decNumberZero(dn);
  #endif

  do {                             
    for (c=chars;; c++) {          
      if (*c>='0' && *c<='9') {    
        last=c;
        d++;                       
        continue;                  
        }
      if (*c=='.' && dotchar==NULL) { 
        dotchar=c;                 
        if (c==cfirst) cfirst++;   
        continue;}
      if (c==chars) {              
        if (*c=='-') {             
          cfirst++;
          bits=DECNEG;
          continue;}
        if (*c=='+') {             
          cfirst++;
          continue;}
        }
      
      break;
      } 

    if (last==NULL) {              
      status=DEC_Conversion_syntax;
      if (*c=='\0') break;         
      #if DECSUBSET
      
      if (!set->extended) break;   
      #endif
      
      if (dotchar!=NULL) break;    
      uprv_decNumberZero(dn);           
      if (decBiStr(c, "infinity", "INFINITY")
       || decBiStr(c, "inf", "INF")) {
        dn->bits=bits | DECINF;
        status=0;                  
        break; 
        }
      
      
      dn->bits=bits | DECNAN;      
      if (*c=='s' || *c=='S') {    
        c++;
        dn->bits=bits | DECSNAN;
        }
      if (*c!='n' && *c!='N') break;    
      c++;
      if (*c!='a' && *c!='A') break;    
      c++;
      if (*c!='n' && *c!='N') break;    
      c++;
      
      
      for (cfirst=c; *cfirst=='0';) cfirst++;
      if (*cfirst=='\0') {         
        status=0;                  
        break;                     
        }
      
      for (c=cfirst;; c++, d++) {
        if (*c<'0' || *c>'9') break; 
        last=c;
        }
      if (*c!='\0') break;         
      if (d>set->digits-1) {
        
        
        if (set->clamp) break;
        if (d>set->digits) break;
        } 
      
      status=0;                    
      bits=dn->bits;               
      } 

     else if (*c!='\0') {          
      
      Flag nege;                   
      const char *firstexp;        
      status=DEC_Conversion_syntax;
      if (*c!='e' && *c!='E') break;
      
      
      nege=0;
      c++;                         
      if (*c=='-') {nege=1; c++;}
       else if (*c=='+') c++;
      if (*c=='\0') break;

      for (; *c=='0' && *(c+1)!='\0';) c++;  
      firstexp=c;                            
      for (; ;c++) {
        if (*c<'0' || *c>'9') break;         
        exponent=X10(exponent)+(Int)*c-(Int)'0';
        } 
      
      if (*c!='\0') break;

      
      
      
      if (c>=firstexp+9+1) {
        if (c>firstexp+9+1 || *firstexp>'1') exponent=DECNUMMAXE*2;
        
        }
      if (nege) exponent=-exponent;     
      status=0;                         
      } 

    
    

    
    if (*cfirst=='0') {                 
      for (c=cfirst; c<last; c++, cfirst++) {
        if (*c=='.') continue;          
        if (*c!='0') break;             
        d--;                            
        } 
      #if DECSUBSET
      
      if (*cfirst=='0' && !set->extended) {
        uprv_decNumberZero(dn);              
        break;                          
        }
      #endif
      } 

    
    if (dotchar!=NULL && dotchar<last)  
      exponent-=(last-dotchar);         
    

    
    
    if (d<=set->digits) res=dn->lsu;    
     else {                             
      Int needbytes=D2U(d)*sizeof(Unit);
      res=resbuff;                      
      if (needbytes>(Int)sizeof(resbuff)) { 
        allocres=(Unit *)malloc(needbytes);
        if (allocres==NULL) {status|=DEC_Insufficient_storage; break;}
        res=allocres;
        }
      }
    

    
    
    #if DECDPUN>1
    out=0;                         
    up=res+D2U(d)-1;               
    cut=d-(up-res)*DECDPUN;        
    for (c=cfirst;; c++) {         
      if (*c=='.') continue;       
      out=X10(out)+(Int)*c-(Int)'0';
      if (c==last) break;          
      cut--;
      if (cut>0) continue;         
      *up=(Unit)out;               
      up--;                        
      cut=DECDPUN;                 
      out=0;                       
      } 
    *up=(Unit)out;                 

    #else
    
    up=res;                        
    for (c=last; c>=cfirst; c--) { 
      if (*c=='.') continue;       
      *up=(Unit)((Int)*c-(Int)'0');
      up++;
      } 
    #endif

    dn->bits=bits;
    dn->exponent=exponent;
    dn->digits=d;

    
    if (d>set->digits) {
      residue=0;
      decSetCoeff(dn, set, res, d, &residue, &status);
      
      decFinalize(dn, set, &residue, &status);
      }
     else { 
      
      if ((dn->exponent-1<set->emin-dn->digits)
       || (dn->exponent-1>set->emax-set->digits)) {
        residue=0;
        decFinalize(dn, set, &residue, &status);
        }
      }
    
    } while(0);                         

  if (allocres!=NULL) free(allocres);   
  if (status!=0) decStatus(dn, status, set);
  return dn;
  } 




















U_CAPI decNumber * U_EXPORT2 uprv_decNumberAbs(decNumber *res, const decNumber *rhs,
                         decContext *set) {
  decNumber dzero;                      
  uInt status=0;                        

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  uprv_decNumberZero(&dzero);                
  dzero.exponent=rhs->exponent;         
  decAddOp(res, &dzero, rhs, set, (uByte)(rhs->bits & DECNEG), &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 














U_CAPI decNumber * U_EXPORT2 uprv_decNumberAdd(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decAddOp(res, lhs, rhs, set, 0, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 
















U_CAPI decNumber * U_EXPORT2 uprv_decNumberAnd(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  const Unit *ua, *ub;                  
  const Unit *msua, *msub;              
  Unit *uc,  *msuc;                     
  Int   msudigs;                        
  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  if (lhs->exponent!=0 || decNumberIsSpecial(lhs) || decNumberIsNegative(lhs)
   || rhs->exponent!=0 || decNumberIsSpecial(rhs) || decNumberIsNegative(rhs)) {
    decStatus(res, DEC_Invalid_operation, set);
    return res;
    }

  
  ua=lhs->lsu;                          
  ub=rhs->lsu;                          
  uc=res->lsu;                          
  msua=ua+D2U(lhs->digits)-1;           
  msub=ub+D2U(rhs->digits)-1;           
  msuc=uc+D2U(set->digits)-1;           
  msudigs=MSUDIGITS(set->digits);       
  for (; uc<=msuc; ua++, ub++, uc++) {  
    Unit a, b;                          
    if (ua>msua) a=0;
     else a=*ua;
    if (ub>msub) b=0;
     else b=*ub;
    *uc=0;                              
    if (a|b) {                          
      Int i, j;
      *uc=0;                            
      
      for (i=0; i<DECDPUN; i++) {
        if (a&b&1) *uc=*uc+(Unit)powers[i];  
        j=a%10;
        a=a/10;
        j|=b%10;
        b=b/10;
        if (j>1) {
          decStatus(res, DEC_Invalid_operation, set);
          return res;
          }
        if (uc==msuc && i==msudigs-1) break; 
        } 
      } 
    } 
  
  res->digits=decGetDigits(res->lsu, uc-res->lsu);
  res->exponent=0;                      
  res->bits=0;                          
  return res;  
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberCompare(decNumber *res, const decNumber *lhs,
                             const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPARE, &status);
  if (status!=0) decStatus(res, status, set);
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberCompareSignal(decNumber *res, const decNumber *lhs,
                                   const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPSIG, &status);
  if (status!=0) decStatus(res, status, set);
  return res;
  } 














U_CAPI decNumber * U_EXPORT2 uprv_decNumberCompareTotal(decNumber *res, const decNumber *lhs,
                                  const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPTOTAL, &status);
  if (status!=0) decStatus(res, status, set);
  return res;
  } 














U_CAPI decNumber * U_EXPORT2 uprv_decNumberCompareTotalMag(decNumber *res, const decNumber *lhs,
                                     const decNumber *rhs, decContext *set) {
  uInt status=0;                   
  uInt needbytes;                  
  decNumber bufa[D2N(DECBUFFER+1)];
  decNumber *allocbufa=NULL;       
  decNumber bufb[D2N(DECBUFFER+1)];
  decNumber *allocbufb=NULL;       
  decNumber *a, *b;                

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  do {                                  
    
    if (decNumberIsNegative(lhs)) {     
      a=bufa;
      needbytes=sizeof(decNumber)+(D2U(lhs->digits)-1)*sizeof(Unit);
      if (needbytes>sizeof(bufa)) {     
        allocbufa=(decNumber *)malloc(needbytes);
        if (allocbufa==NULL) {          
          status|=DEC_Insufficient_storage;
          break;}
        a=allocbufa;                    
        }
      uprv_decNumberCopy(a, lhs);            
      a->bits&=~DECNEG;                 
      lhs=a;                            
      }
    if (decNumberIsNegative(rhs)) {     
      b=bufb;
      needbytes=sizeof(decNumber)+(D2U(rhs->digits)-1)*sizeof(Unit);
      if (needbytes>sizeof(bufb)) {     
        allocbufb=(decNumber *)malloc(needbytes);
        if (allocbufb==NULL) {          
          status|=DEC_Insufficient_storage;
          break;}
        b=allocbufb;                    
        }
      uprv_decNumberCopy(b, rhs);            
      b->bits&=~DECNEG;                 
      rhs=b;                            
      }
    decCompareOp(res, lhs, rhs, set, COMPTOTAL, &status);
    } while(0);                         

  if (allocbufa!=NULL) free(allocbufa); 
  if (allocbufb!=NULL) free(allocbufb); 
  if (status!=0) decStatus(res, status, set);
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberDivide(decNumber *res, const decNumber *lhs,
                            const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decDivideOp(res, lhs, rhs, set, DIVIDE, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberDivideInteger(decNumber *res, const decNumber *lhs,
                                   const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decDivideOp(res, lhs, rhs, set, DIVIDEINT, &status);
  if (status!=0) decStatus(res, status, set);
  return res;
  } 


























U_CAPI decNumber * U_EXPORT2 uprv_decNumberExp(decNumber *res, const decNumber *rhs,
                         decContext *set) {
  uInt status=0;                        
  #if DECSUBSET
  decNumber *allocrhs=NULL;        
  #endif

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  
  
  
  if (!decCheckMath(rhs, set, &status)) do { 
    #if DECSUBSET
    if (!set->extended) {
      
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, &status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif
    decExpOp(res, rhs, set, &status);
    } while(0);                         

  #if DECSUBSET
  if (allocrhs !=NULL) free(allocrhs);  
  #endif
  
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 

















U_CAPI decNumber * U_EXPORT2 uprv_decNumberFMA(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, const decNumber *fhs,
                         decContext *set) {
  uInt status=0;                   
  decContext dcmul;                
  uInt needbytes;                  
  decNumber bufa[D2N(DECBUFFER*2+1)];
  decNumber *allocbufa=NULL;       
  decNumber *acc;                  
  decNumber dzero;                 

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  if (decCheckOperands(res, fhs, DECUNUSED, set)) return res;
  #endif

  do {                                  
    #if DECSUBSET
    if (!set->extended) {               
      status|=DEC_Invalid_operation;
      break;}
    #endif
    
    if ((!decNumberIsSpecial(lhs) && decCheckMath(lhs, set, &status))
     || (!decNumberIsSpecial(rhs) && decCheckMath(rhs, set, &status))
     || (!decNumberIsSpecial(fhs) && decCheckMath(fhs, set, &status))) break;
    
    dcmul=*set;
    dcmul.digits=lhs->digits+rhs->digits; 
    
    dcmul.emax=DEC_MAX_EMAX;            
    dcmul.emin=DEC_MIN_EMIN;            
    
    acc=bufa;                           
    needbytes=sizeof(decNumber)+(D2U(dcmul.digits)-1)*sizeof(Unit);
    if (needbytes>sizeof(bufa)) {       
      allocbufa=(decNumber *)malloc(needbytes);
      if (allocbufa==NULL) {            
        status|=DEC_Insufficient_storage;
        break;}
      acc=allocbufa;                    
      }
    
    
    decMultiplyOp(acc, lhs, rhs, &dcmul, &status);
    
    
    
    
    
    
    if ((status&DEC_Invalid_operation)!=0) {
      if (!(status&DEC_sNaN)) {         
        uprv_decNumberZero(res);             
        res->bits=DECNAN;
        break;
        }
      uprv_decNumberZero(&dzero);            
      fhs=&dzero;                       
      }
    #if DECCHECK
     else { 
      if (status!=0) printf("Status=%08lx after FMA multiply\n", (LI)status);
      }
    #endif
    
    decAddOp(res, acc, fhs, set, 0, &status);
    } while(0);                         

  if (allocbufa!=NULL) free(allocbufa); 
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 















U_CAPI decNumber * U_EXPORT2 uprv_decNumberInvert(decNumber *res, const decNumber *rhs,
                            decContext *set) {
  const Unit *ua, *msua;                
  Unit  *uc, *msuc;                     
  Int   msudigs;                        
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  if (rhs->exponent!=0 || decNumberIsSpecial(rhs) || decNumberIsNegative(rhs)) {
    decStatus(res, DEC_Invalid_operation, set);
    return res;
    }
  
  ua=rhs->lsu;                          
  uc=res->lsu;                          
  msua=ua+D2U(rhs->digits)-1;           
  msuc=uc+D2U(set->digits)-1;           
  msudigs=MSUDIGITS(set->digits);       
  for (; uc<=msuc; ua++, uc++) {        
    Unit a;                             
    Int  i, j;                          
    if (ua>msua) a=0;
     else a=*ua;
    *uc=0;                              
    
    
    for (i=0; i<DECDPUN; i++) {
      if ((~a)&1) *uc=*uc+(Unit)powers[i];   
      j=a%10;
      a=a/10;
      if (j>1) {
        decStatus(res, DEC_Invalid_operation, set);
        return res;
        }
      if (uc==msuc && i==msudigs-1) break;   
      } 
    } 
  
  res->digits=decGetDigits(res->lsu, uc-res->lsu);
  res->exponent=0;                      
  res->bits=0;                          
  return res;  
  } 





























U_CAPI decNumber * U_EXPORT2 uprv_decNumberLn(decNumber *res, const decNumber *rhs,
                        decContext *set) {
  uInt status=0;                   
  #if DECSUBSET
  decNumber *allocrhs=NULL;        
  #endif

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  
  if (!decCheckMath(rhs, set, &status)) do { 
    #if DECSUBSET
    if (!set->extended) {
      
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, &status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      
      if (ISZERO(rhs)) {                
        status|=DEC_Invalid_operation;
        break;}
      } 
    #endif
    decLnOp(res, rhs, set, &status);
    } while(0);                         

  #if DECSUBSET
  if (allocrhs !=NULL) free(allocrhs);  
  #endif
  
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 

























U_CAPI decNumber * U_EXPORT2 uprv_decNumberLogB(decNumber *res, const decNumber *rhs,
                          decContext *set) {
  uInt status=0;                   

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  if (decNumberIsNaN(rhs)) decNaNs(res, rhs, NULL, set, &status);
   else if (decNumberIsInfinite(rhs)) uprv_decNumberCopyAbs(res, rhs);
   else if (decNumberIsZero(rhs)) {
    uprv_decNumberZero(res);                 
    res->bits=DECNEG|DECINF;            
    status|=DEC_Division_by_zero;       
    }
   else { 
    Int ae=rhs->exponent+rhs->digits-1; 
    uprv_decNumberFromInt32(res, ae);        
    }

  if (status!=0) decStatus(res, status, set);
  return res;
  } 
































#if defined(__clang__) || U_GCC_MAJOR_MINOR >= 406
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
U_CAPI decNumber * U_EXPORT2 uprv_decNumberLog10(decNumber *res, const decNumber *rhs,
                          decContext *set) {
  uInt status=0, ignore=0;         
  uInt needbytes;                  
  Int p;                           
  Int t;                           

  
  
  decNumber bufa[D2N(DECBUFFER+2)];
  decNumber *allocbufa=NULL;       
  decNumber *a=bufa;               
  decNumber bufb[D2N(DECBUFFER+2)];
  decNumber *allocbufb=NULL;       
  decNumber *b=bufb;               
  decNumber bufw[D2N(10)];         
  decNumber *w=bufw;               
  #if DECSUBSET
  decNumber *allocrhs=NULL;        
  #endif

  decContext aset;                 

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  
  if (!decCheckMath(rhs, set, &status)) do { 
    #if DECSUBSET
    if (!set->extended) {
      
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, &status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      
      if (ISZERO(rhs)) {                
        status|=DEC_Invalid_operation;
        break;}
      } 
    #endif

    uprv_decContextDefault(&aset, DEC_INIT_DECIMAL64); 

    
    if (!(rhs->bits&(DECNEG|DECSPECIAL)) && !ISZERO(rhs)) {
      Int residue=0;               
      uInt copystat=0;             

      
      aset.digits=1;
      decCopyFit(w, rhs, &aset, &residue, &copystat); 
      
      if (!(copystat&DEC_Inexact) && w->lsu[0]==1) {
        
        
        
        
        uprv_decNumberFromInt32(w, w->exponent);
        residue=0;
        decCopyFit(res, w, set, &residue, &status); 
        decFinish(res, set, &residue, &status);     
        break;
        } 
      } 

    
    
    
    
    t=6;                                

    
    p=(rhs->digits+t>set->digits?rhs->digits+t:set->digits)+3;
    needbytes=sizeof(decNumber)+(D2U(p)-1)*sizeof(Unit);
    if (needbytes>sizeof(bufa)) {       
      allocbufa=(decNumber *)malloc(needbytes);
      if (allocbufa==NULL) {            
        status|=DEC_Insufficient_storage;
        break;}
      a=allocbufa;                      
      }
    aset.digits=p;                      
    aset.emax=DEC_MAX_MATH;             
    aset.emin=-DEC_MAX_MATH;            
    aset.clamp=0;                       
    decLnOp(a, rhs, &aset, &status);    

    
    
    if (status&DEC_NaNs && !(status&DEC_sNaN)) break;
    if (a->bits&DECSPECIAL || ISZERO(a)) {
      uprv_decNumberCopy(res, a);            
      break;}

    
    p=set->digits+3;
    needbytes=sizeof(decNumber)+(D2U(p)-1)*sizeof(Unit);
    if (needbytes>sizeof(bufb)) {       
      allocbufb=(decNumber *)malloc(needbytes);
      if (allocbufb==NULL) {            
        status|=DEC_Insufficient_storage;
        break;}
      b=allocbufb;                      
      }
    uprv_decNumberZero(w);                   
    #if DECDPUN==1
    w->lsu[1]=1; w->lsu[0]=0;           
    #else
    w->lsu[0]=10;                       
    #endif
    w->digits=2;                        

    aset.digits=p;
    decLnOp(b, w, &aset, &ignore);      

    aset.digits=set->digits;            
    decDivideOp(res, a, b, &aset, DIVIDE, &status); 
    } while(0);                         

  if (allocbufa!=NULL) free(allocbufa); 
  if (allocbufb!=NULL) free(allocbufb); 
  #if DECSUBSET
  if (allocrhs !=NULL) free(allocrhs);  
  #endif
  
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 
#if defined(__clang__) || U_GCC_MAJOR_MINOR >= 406
#pragma GCC diagnostic pop
#endif













U_CAPI decNumber * U_EXPORT2 uprv_decNumberMax(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPMAX, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberMaxMag(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPMAXMAG, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberMin(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPMIN, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberMinMag(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decCompareOp(res, lhs, rhs, set, COMPMINMAG, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 















U_CAPI decNumber * U_EXPORT2 uprv_decNumberMinus(decNumber *res, const decNumber *rhs,
                           decContext *set) {
  decNumber dzero;
  uInt status=0;                        

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  uprv_decNumberZero(&dzero);                
  dzero.exponent=rhs->exponent;         
  decAddOp(res, &dzero, rhs, set, DECNEG, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 












U_CAPI decNumber * U_EXPORT2 uprv_decNumberNextMinus(decNumber *res, const decNumber *rhs,
                               decContext *set) {
  decNumber dtiny;                           
  decContext workset=*set;                   
  uInt status=0;                             
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  if ((rhs->bits&(DECINF|DECNEG))==DECINF) {
    decSetMaxValue(res, set);                
    
    return res;
    }
  uprv_decNumberZero(&dtiny);                     
  dtiny.lsu[0]=1;                            
  dtiny.exponent=DEC_MIN_EMIN-1;             
  workset.round=DEC_ROUND_FLOOR;
  decAddOp(res, rhs, &dtiny, &workset, DECNEG, &status);
  status&=DEC_Invalid_operation|DEC_sNaN;    
  if (status!=0) decStatus(res, status, set);
  return res;
  } 












U_CAPI decNumber * U_EXPORT2 uprv_decNumberNextPlus(decNumber *res, const decNumber *rhs,
                              decContext *set) {
  decNumber dtiny;                           
  decContext workset=*set;                   
  uInt status=0;                             
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  if ((rhs->bits&(DECINF|DECNEG))==(DECINF|DECNEG)) {
    decSetMaxValue(res, set);
    res->bits=DECNEG;                        
    
    return res;
    }
  uprv_decNumberZero(&dtiny);                     
  dtiny.lsu[0]=1;                            
  dtiny.exponent=DEC_MIN_EMIN-1;             
  workset.round=DEC_ROUND_CEILING;
  decAddOp(res, rhs, &dtiny, &workset, 0, &status);
  status&=DEC_Invalid_operation|DEC_sNaN;    
  if (status!=0) decStatus(res, status, set);
  return res;
  } 















U_CAPI decNumber * U_EXPORT2 uprv_decNumberNextToward(decNumber *res, const decNumber *lhs,
                                const decNumber *rhs, decContext *set) {
  decNumber dtiny;                           
  decContext workset=*set;                   
  Int result;                                
  uInt status=0;                             
  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  if (decNumberIsNaN(lhs) || decNumberIsNaN(rhs)) {
    decNaNs(res, lhs, rhs, set, &status);
    }
   else { 
    result=decCompare(lhs, rhs, 0);     
    if (result==BADINT) status|=DEC_Insufficient_storage; 
     else { 
      if (result==0) uprv_decNumberCopySign(res, lhs, rhs); 
       else { 
        uByte sub;                      
        if (result<0) {                 
          
          if ((lhs->bits&(DECINF|DECNEG))==(DECINF|DECNEG)) {
            decSetMaxValue(res, set);
            res->bits=DECNEG;           
            return res;                 
            }
          workset.round=DEC_ROUND_CEILING;
          sub=0;                        
          } 
         else {                         
          
          if ((lhs->bits&(DECINF|DECNEG))==DECINF) {
            decSetMaxValue(res, set);
            return res;                 
            }
          workset.round=DEC_ROUND_FLOOR;
          sub=DECNEG;                   
          } 
        uprv_decNumberZero(&dtiny);          
        dtiny.lsu[0]=1;                 
        dtiny.exponent=DEC_MIN_EMIN-1;  
        decAddOp(res, lhs, &dtiny, &workset, sub, &status); 
        
        
        if (uprv_decNumberIsNormal(res, set)) status=0;
        } 
      } 
    } 
  if (status!=0) decStatus(res, status, set);
  return res;
  } 
















U_CAPI decNumber * U_EXPORT2 uprv_decNumberOr(decNumber *res, const decNumber *lhs,
                        const decNumber *rhs, decContext *set) {
  const Unit *ua, *ub;                  
  const Unit *msua, *msub;              
  Unit  *uc, *msuc;                     
  Int   msudigs;                        
  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  if (lhs->exponent!=0 || decNumberIsSpecial(lhs) || decNumberIsNegative(lhs)
   || rhs->exponent!=0 || decNumberIsSpecial(rhs) || decNumberIsNegative(rhs)) {
    decStatus(res, DEC_Invalid_operation, set);
    return res;
    }
  
  ua=lhs->lsu;                          
  ub=rhs->lsu;                          
  uc=res->lsu;                          
  msua=ua+D2U(lhs->digits)-1;           
  msub=ub+D2U(rhs->digits)-1;           
  msuc=uc+D2U(set->digits)-1;           
  msudigs=MSUDIGITS(set->digits);       
  for (; uc<=msuc; ua++, ub++, uc++) {  
    Unit a, b;                          
    if (ua>msua) a=0;
     else a=*ua;
    if (ub>msub) b=0;
     else b=*ub;
    *uc=0;                              
    if (a|b) {                          
      Int i, j;
      
      for (i=0; i<DECDPUN; i++) {
        if ((a|b)&1) *uc=*uc+(Unit)powers[i];     
        j=a%10;
        a=a/10;
        j|=b%10;
        b=b/10;
        if (j>1) {
          decStatus(res, DEC_Invalid_operation, set);
          return res;
          }
        if (uc==msuc && i==msudigs-1) break;      
        } 
      } 
    } 
  
  res->digits=decGetDigits(res->lsu, uc-res->lsu);
  res->exponent=0;                      
  res->bits=0;                          
  return res;  
  } 

















U_CAPI decNumber * U_EXPORT2 uprv_decNumberPlus(decNumber *res, const decNumber *rhs,
                          decContext *set) {
  decNumber dzero;
  uInt status=0;                        
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  uprv_decNumberZero(&dzero);                
  dzero.exponent=rhs->exponent;         
  decAddOp(res, &dzero, rhs, set, 0, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberMultiply(decNumber *res, const decNumber *lhs,
                              const decNumber *rhs, decContext *set) {
  uInt status=0;                   
  decMultiplyOp(res, lhs, rhs, set, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 



























U_CAPI decNumber * U_EXPORT2 uprv_decNumberPower(decNumber *res, const decNumber *lhs,
                           const decNumber *rhs, decContext *set) {
  #if DECSUBSET
  decNumber *alloclhs=NULL;        
  decNumber *allocrhs=NULL;        
  #endif
  decNumber *allocdac=NULL;        
  decNumber *allocinv=NULL;        
  Int   reqdigits=set->digits;     
  Int   n;                         
  Flag  rhsint=0;                  
  Flag  useint=0;                  
  Flag  isoddint=0;                
  Int   i;                         
  #if DECSUBSET
  Int   dropped;                   
  #endif
  uInt  needbytes;                 
  Flag  seenbit;                   
  Int   residue=0;                 
  uInt  status=0;                  
  uByte bits=0;                    
  decContext aset;                 
  decNumber dnOne;                 
  
  decNumber dacbuff[D2N(DECBUFFER+9)];
  decNumber *dac=dacbuff;          
  
  decNumber invbuff[D2N(DECBUFFER+9)];

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) { 
      if (lhs->digits>reqdigits) {
        alloclhs=decRoundOperand(lhs, set, &status);
        if (alloclhs==NULL) break;
        lhs=alloclhs;
        }
      if (rhs->digits>reqdigits) {
        allocrhs=decRoundOperand(rhs, set, &status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif
    

    
    if (SPECIALARGS) {
      if (decNumberIsNaN(lhs) || decNumberIsNaN(rhs)) { 
        decNaNs(res, lhs, rhs, set, &status);
        break;}
      if (decNumberIsInfinite(rhs)) {   
        Flag rhsneg=rhs->bits&DECNEG;   
        if (decNumberIsNegative(lhs)    
         && !decNumberIsZero(lhs))      
          status|=DEC_Invalid_operation;
         else {                         
          uprv_decNumberZero(&dnOne);        
          dnOne.lsu[0]=1;
          uprv_decNumberCompare(dac, lhs, &dnOne, set); 
          uprv_decNumberZero(res);           
          if (decNumberIsNegative(dac)) {    
            if (rhsneg) res->bits|=DECINF;   
            }
           else if (dac->lsu[0]==0) {        
            
            Int shift=set->digits-1;
            *res->lsu=1;                     
            res->digits=decShiftToMost(res->lsu, 1, shift);
            res->exponent=-shift;            
            status|=DEC_Inexact|DEC_Rounded; 
            }
           else {                            
            if (!rhsneg) res->bits|=DECINF;  
            }
          } 
        break;}
      
      } 

    
    n=decGetInt(rhs);
    if (n!=BADINT) {                    
      rhsint=1;                         
      isoddint=(Flag)n&1;               
      if (n!=BIGEVEN && n!=BIGODD)      
        useint=1;                       
      }

    if (decNumberIsNegative(lhs)        
      && isoddint) bits=DECNEG;         

    
    if (decNumberIsInfinite(lhs)) {     
      uByte rbits=rhs->bits;            
      uprv_decNumberZero(res);               
      if (n==0) *res->lsu=1;            
       else {
        
        if (!rhsint && decNumberIsNegative(lhs)) {
          status|=DEC_Invalid_operation;     
          break;}
        if (!(rbits & DECNEG)) bits|=DECINF; 
        
        res->bits=bits;
        }
      break;}

    
    if (decNumberIsZero(lhs)) {
      if (n==0) {                            
        #if DECSUBSET
        if (!set->extended) {                
          uprv_decNumberZero(res);
          *res->lsu=1;                       
          break;}
        #endif
        status|=DEC_Invalid_operation;
        }
       else {                                
        uByte rbits=rhs->bits;               
        if (rbits & DECNEG) {                
          #if DECSUBSET
          if (!set->extended) {              
            status|=DEC_Invalid_operation;
            break;}
          #endif
          bits|=DECINF;
          }
        uprv_decNumberZero(res);                  
        
        res->bits=bits;
        }
      break;}

    
    
    if (!useint) {                      
      
      
      if (decNumberIsNegative(lhs)) {
        status|=DEC_Invalid_operation;
        break;}
      if (decCheckMath(lhs, set, &status)
       || decCheckMath(rhs, set, &status)) break; 

      uprv_decContextDefault(&aset, DEC_INIT_DECIMAL64); 
      aset.emax=DEC_MAX_MATH;           
      aset.emin=-DEC_MAX_MATH;          
      aset.clamp=0;                     

      
      
      
      
      
      
      
      
      
      
      aset.digits=MAXI(lhs->digits, set->digits)+6+4;
      } 

     else { 
      if (n==0) {                       
        
        uprv_decNumberZero(res);             
        *res->lsu=1;                    
        break;}
      
      if (n<0) n=-n;                    

      aset=*set;                        
      aset.round=DEC_ROUND_HALF_EVEN;   
      
      aset.digits=reqdigits+(rhs->digits+rhs->exponent)+2;
      #if DECSUBSET
      if (!set->extended) aset.digits--;     
      #endif
      
      if (aset.digits>DECNUMMAXP) {status|=DEC_Invalid_operation; break;}
      } 

    
    
    needbytes=sizeof(decNumber)+(D2U(aset.digits)-1)*sizeof(Unit);
    
    if (needbytes>sizeof(dacbuff)) {
      allocdac=(decNumber *)malloc(needbytes);
      if (allocdac==NULL) {   
        status|=DEC_Insufficient_storage;
        break;}
      dac=allocdac;           
      }
    

    if (!useint) {                           
      
      
      
      decLnOp(dac, lhs, &aset, &status);     
      
      if (ISZERO(dac)) {                     
        
        *dac->lsu=1;                         
        if (!rhsint) {                       
          Int shift=set->digits-1;
          dac->digits=decShiftToMost(dac->lsu, 1, shift);
          dac->exponent=-shift;              
          status|=DEC_Inexact|DEC_Rounded;   
          }
        }
       else {
        decMultiplyOp(dac, dac, rhs, &aset, &status);  
        decExpOp(dac, dac, &aset, &status);            
        }
      
      } 

     else {                             
      uprv_decNumberZero(dac);               
      *dac->lsu=1;                      

      
      
      if (decNumberIsNegative(rhs)) {   
        decNumber *inv=invbuff;         
        uprv_decNumberCopy(&dnOne, dac);     
        #if DECSUBSET
        if (set->extended) {            
        #endif

          decDivideOp(dac, &dnOne, lhs, &aset, DIVIDE, &status);
          
          if (needbytes>sizeof(invbuff)) {
            allocinv=(decNumber *)malloc(needbytes);
            if (allocinv==NULL) {       
              status|=DEC_Insufficient_storage;
              break;}
            inv=allocinv;               
            }
          
          uprv_decNumberCopy(inv, dac);      
          uprv_decNumberCopy(dac, &dnOne);   
          lhs=inv;                      
        #if DECSUBSET
          }
        #endif
        }

      
      seenbit=0;                   
      for (i=1;;i++){              
        
        if (status & (DEC_Overflow|DEC_Underflow)) { 
          if (status&DEC_Overflow || ISZERO(dac)) break;
          }
        
        
        n=n<<1;                    
        if (n<0) {                 
          seenbit=1;               
          decMultiplyOp(dac, dac, lhs, &aset, &status); 
          }
        if (i==31) break;          
        if (!seenbit) continue;    
        decMultiplyOp(dac, dac, dac, &aset, &status); 
        }  

      
      if (status & (DEC_Overflow|DEC_Underflow)) {
        #if DECSUBSET
        
        
        if (!set->extended && decNumberIsNegative(rhs)) {
          if (status & DEC_Overflow)
            status^=DEC_Overflow | DEC_Underflow | DEC_Subnormal;
           else { 
            status&=~(DEC_Underflow | DEC_Subnormal); 
            status|=DEC_Overflow;
            }
          }
        #endif
        dac->bits=(dac->bits & ~DECNEG) | bits; 
        
        
        decFinalize(dac, set, &residue, &status);
        uprv_decNumberCopy(res, dac);   
        break;
        }

      #if DECSUBSET
      if (!set->extended &&                  
          decNumberIsNegative(rhs)) {        
        
        decDivideOp(dac, &dnOne, dac, &aset, DIVIDE, &status);
        }
      #endif
      } 

    
    decCopyFit(res, dac, set, &residue, &status);
    decFinish(res, set, &residue, &status);  
    #if DECSUBSET
    if (!set->extended) decTrim(res, set, 0, 1, &dropped); 
    #endif
    } while(0);                         

  if (allocdac!=NULL) free(allocdac);   
  if (allocinv!=NULL) free(allocinv);   
  #if DECSUBSET
  if (alloclhs!=NULL) free(alloclhs);   
  if (allocrhs!=NULL) free(allocrhs);   
  #endif
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 



















U_CAPI decNumber * U_EXPORT2 uprv_decNumberQuantize(decNumber *res, const decNumber *lhs,
                              const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decQuantizeOp(res, lhs, rhs, set, 1, &status);
  if (status!=0) decStatus(res, status, set);
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberNormalize(decNumber *res, const decNumber *rhs,
                               decContext *set) {
  return uprv_decNumberReduce(res, rhs, set);
  } 

U_CAPI decNumber * U_EXPORT2 uprv_decNumberReduce(decNumber *res, const decNumber *rhs,
                            decContext *set) {
  #if DECSUBSET
  decNumber *allocrhs=NULL;        
  #endif
  uInt status=0;                   
  Int  residue=0;                  
  Int  dropped;                    

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, &status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif


    
    if (decNumberIsNaN(rhs)) {
      decNaNs(res, rhs, NULL, set, &status);
      break;
      }

    
    decCopyFit(res, rhs, set, &residue, &status); 
    decFinish(res, set, &residue, &status);       
    decTrim(res, set, 1, 0, &dropped);            
                                                  
    } while(0);                              

  #if DECSUBSET
  if (allocrhs !=NULL) free(allocrhs);       
  #endif
  if (status!=0) decStatus(res, status, set);
  return res;
  } 



















U_CAPI decNumber * U_EXPORT2 uprv_decNumberRescale(decNumber *res, const decNumber *lhs,
                             const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decQuantizeOp(res, lhs, rhs, set, 0, &status);
  if (status!=0) decStatus(res, status, set);
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberRemainder(decNumber *res, const decNumber *lhs,
                               const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decDivideOp(res, lhs, rhs, set, REMAINDER, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberRemainderNear(decNumber *res, const decNumber *lhs,
                                   const decNumber *rhs, decContext *set) {
  uInt status=0;                        
  decDivideOp(res, lhs, rhs, set, REMNEAR, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 


























U_CAPI decNumber * U_EXPORT2 uprv_decNumberRotate(decNumber *res, const decNumber *lhs,
                           const decNumber *rhs, decContext *set) {
  uInt status=0;              
  Int  rotate;                

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  
  if (decNumberIsNaN(lhs) || decNumberIsNaN(rhs))
    decNaNs(res, lhs, rhs, set, &status);
   
   else if (decNumberIsInfinite(rhs) || rhs->exponent!=0)
    status=DEC_Invalid_operation;
   else { 
    rotate=decGetInt(rhs);                   
    if (rotate==BADINT                       
     || rotate==BIGODD || rotate==BIGEVEN    
     || abs(rotate)>set->digits)             
      status=DEC_Invalid_operation;
     else {                                  
      uprv_decNumberCopy(res, lhs);
      
      if (rotate<0) rotate=set->digits+rotate;
      if (rotate!=0 && rotate!=set->digits   
       && !decNumberIsInfinite(res)) {       
        
        uInt units, shift;                   
        uInt msudigits;                      
        Unit *msu=res->lsu+D2U(res->digits)-1;    
        Unit *msumax=res->lsu+D2U(set->digits)-1; 
        for (msu++; msu<=msumax; msu++) *msu=0;   
        res->digits=set->digits;                  
        msudigits=MSUDIGITS(res->digits);         

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        
        rotate=set->digits-rotate;      
        units=rotate/DECDPUN;           
        shift=rotate%DECDPUN;           
        if (shift>0) {                  
          uInt save=res->lsu[0]%powers[shift];    
          decShiftToLeast(res->lsu, D2U(res->digits), shift);
          if (shift>msudigits) {        
            uInt rem=save%powers[shift-msudigits];
            *msumax=(Unit)(save/powers[shift-msudigits]); 
            *(msumax-1)=*(msumax-1)
                       +(Unit)(rem*powers[DECDPUN-(shift-msudigits)]); 
            }
           else { 
            *msumax=*msumax+(Unit)(save*powers[msudigits-shift]); 
            }
          } 

        
        if (units>0) {                  
          
          
          
          shift=DECDPUN-msudigits;
          if (shift>0) {                
            uInt save=res->lsu[0]%powers[shift];  
            decShiftToLeast(res->lsu, units, shift);
            *msumax=*msumax+(Unit)(save*powers[msudigits]);
            } 

          
          
          decReverse(res->lsu+units, msumax);     
          decReverse(res->lsu, res->lsu+units-1); 
          decReverse(res->lsu, msumax);           
          } 
        
        
        res->digits=decGetDigits(res->lsu, msumax-res->lsu+1);
        } 
      } 
    } 
  if (status!=0) decStatus(res, status, set);
  return res;
  } 










U_CAPI decNumber * U_EXPORT2 uprv_decNumberSameQuantum(decNumber *res, const decNumber *lhs,
                                 const decNumber *rhs) {
  Unit ret=0;                      

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, DECUNCONT)) return res;
  #endif

  if (SPECIALARGS) {
    if (decNumberIsNaN(lhs) && decNumberIsNaN(rhs)) ret=1;
     else if (decNumberIsInfinite(lhs) && decNumberIsInfinite(rhs)) ret=1;
     
    }
   else if (lhs->exponent==rhs->exponent) ret=1;

  uprv_decNumberZero(res);              
  *res->lsu=ret;
  return res;
  } 
















U_CAPI decNumber * U_EXPORT2 uprv_decNumberScaleB(decNumber *res, const decNumber *lhs,
                            const decNumber *rhs, decContext *set) {
  Int  reqexp;                
  uInt status=0;              
  Int  residue;               

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  
  if (decNumberIsNaN(lhs) || decNumberIsNaN(rhs))
    decNaNs(res, lhs, rhs, set, &status);
    
   else if (decNumberIsInfinite(rhs) || rhs->exponent!=0)
    status=DEC_Invalid_operation;
   else {
    
    reqexp=decGetInt(rhs);                   
    if (reqexp==BADINT                       
     || reqexp==BIGODD || reqexp==BIGEVEN    
     || abs(reqexp)>(2*(set->digits+set->emax))) 
      status=DEC_Invalid_operation;
     else {                                  
      uprv_decNumberCopy(res, lhs);               
      if (!decNumberIsInfinite(res)) {       
        res->exponent+=reqexp;               
        residue=0;
        decFinalize(res, set, &residue, &status); 
        } 
      } 
    } 
  if (status!=0) decStatus(res, status, set);
  return res;
  } 






















U_CAPI decNumber * U_EXPORT2 uprv_decNumberShift(decNumber *res, const decNumber *lhs,
                           const decNumber *rhs, decContext *set) {
  uInt status=0;              
  Int  shift;                 

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  
  if (decNumberIsNaN(lhs) || decNumberIsNaN(rhs))
    decNaNs(res, lhs, rhs, set, &status);
   
   else if (decNumberIsInfinite(rhs) || rhs->exponent!=0)
    status=DEC_Invalid_operation;
   else { 
    shift=decGetInt(rhs);                    
    if (shift==BADINT                        
     || shift==BIGODD || shift==BIGEVEN      
     || abs(shift)>set->digits)              
      status=DEC_Invalid_operation;
     else {                                  
      uprv_decNumberCopy(res, lhs);
      if (shift!=0 && !decNumberIsInfinite(res)) { 
        if (shift>0) {                       
          if (shift==set->digits) {          
            *res->lsu=0;                     
            res->digits=1;                   
            }
           else {                            
            
            if (res->digits+shift>set->digits) {
              decDecap(res, res->digits+shift-set->digits);
              
              
              }
            if (res->digits>1 || *res->lsu)  
              res->digits=decShiftToMost(res->lsu, res->digits, shift);
            } 
          } 
         else { 
          if (-shift>=res->digits) {         
            *res->lsu=0;                     
            res->digits=1;                   
            }
           else {
            decShiftToLeast(res->lsu, D2U(res->digits), -shift);
            res->digits-=(-shift);
            }
          } 
        } 
      } 
    } 
  if (status!=0) decStatus(res, status, set);
  return res;
  } 









































































#if defined(__clang__) || U_GCC_MAJOR_MINOR >= 406
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
U_CAPI decNumber * U_EXPORT2 uprv_decNumberSquareRoot(decNumber *res, const decNumber *rhs,
                                decContext *set) {
  decContext workset, approxset;   
  decNumber dzero;                 
  Int  maxp;                       
  Int  workp;                      
  Int  residue=0;                  
  uInt status=0, ignore=0;         
  uInt rstatus;                    
  Int  exp;                        
  Int  ideal;                      
  Int  needbytes;                  
  Int  dropped;                    

  #if DECSUBSET
  decNumber *allocrhs=NULL;        
  #endif
  
  decNumber buff[D2N(DECBUFFER+1)];
  
  decNumber bufa[D2N(DECBUFFER+2)];
  
  decNumber bufb[D2N(DECBUFFER+2)];
  decNumber *allocbuff=NULL;       
  decNumber *allocbufa=NULL;       
  decNumber *allocbufb=NULL;       
  decNumber *f=buff;               
  decNumber *a=bufa;               
  decNumber *b=bufb;               
  
  decNumber buft[D2N(3)];
  decNumber *t=buft;               

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, &status);
        if (allocrhs==NULL) break;
        
        
        rhs=allocrhs;
        }
      }
    #endif
    

    
    if (SPECIALARG) {
      if (decNumberIsInfinite(rhs)) {         
        if (decNumberIsNegative(rhs)) status|=DEC_Invalid_operation;
         else uprv_decNumberCopy(res, rhs);        
        }
       else decNaNs(res, rhs, NULL, set, &status); 
      break;
      }

    
    
    
    ideal=(rhs->exponent&~1)/2;         

    
    if (ISZERO(rhs)) {
      uprv_decNumberCopy(res, rhs);          
      res->exponent=ideal;              
      
      decFinish(res, set, &residue, &status);
      break;
      }

    
    if (decNumberIsNegative(rhs)) {
      status|=DEC_Invalid_operation;
      break;
      }

    
    
    
    
    
    
    
    workp=MAXI(set->digits+1, rhs->digits);  
    workp=MAXI(workp, 7);                    
    maxp=workp+2;                            

    needbytes=sizeof(decNumber)+(D2U(rhs->digits)-1)*sizeof(Unit);
    if (needbytes>(Int)sizeof(buff)) {
      allocbuff=(decNumber *)malloc(needbytes);
      if (allocbuff==NULL) {  
        status|=DEC_Insufficient_storage;
        break;}
      f=allocbuff;            
      }
    
    needbytes=sizeof(decNumber)+(D2U(maxp)-1)*sizeof(Unit);
    if (needbytes>(Int)sizeof(bufa)) {            
      allocbufa=(decNumber *)malloc(needbytes);
      allocbufb=(decNumber *)malloc(needbytes);
      if (allocbufa==NULL || allocbufb==NULL) {   
        status|=DEC_Insufficient_storage;
        break;}
      a=allocbufa;            
      b=allocbufb;            
      }

    
    uprv_decNumberCopy(f, rhs);
    exp=f->exponent+f->digits;               
    f->exponent=-(f->digits);                

    
    uprv_decContextDefault(&workset, DEC_INIT_DECIMAL64);
    workset.emax=DEC_MAX_EMAX;
    workset.emin=DEC_MIN_EMIN;

    
    

    
    workset.digits=workp;                    
    t->bits=0; t->digits=3;
    a->bits=0; a->digits=3;
    if ((exp & 1)==0) {                      
      
      t->exponent=-3;
      a->exponent=-3;
      #if DECDPUN>=3
        t->lsu[0]=259;
        a->lsu[0]=819;
      #elif DECDPUN==2
        t->lsu[0]=59; t->lsu[1]=2;
        a->lsu[0]=19; a->lsu[1]=8;
      #else
        t->lsu[0]=9; t->lsu[1]=5; t->lsu[2]=2;
        a->lsu[0]=9; a->lsu[1]=1; a->lsu[2]=8;
      #endif
      }
     else {                                  
      
      f->exponent--;                         
      exp++;                                 
      t->exponent=-4;
      a->exponent=-2;
      #if DECDPUN>=3
        t->lsu[0]=819;
        a->lsu[0]=259;
      #elif DECDPUN==2
        t->lsu[0]=19; t->lsu[1]=8;
        a->lsu[0]=59; a->lsu[1]=2;
      #else
        t->lsu[0]=9; t->lsu[1]=1; t->lsu[2]=8;
        a->lsu[0]=9; a->lsu[1]=5; a->lsu[2]=2;
      #endif
      }

    decMultiplyOp(a, a, f, &workset, &ignore);    
    decAddOp(a, a, t, &workset, 0, &ignore);      
    
    

    
    uprv_decNumberZero(&dzero);                   
    uprv_decNumberZero(t);                        
    t->lsu[0]=5;                             
    t->exponent=-1;                          
    workset.digits=3;                        
    for (; workset.digits<maxp;) {
      
      workset.digits=MINI(workset.digits*2-2, maxp);
      
      
      decDivideOp(b, f, a, &workset, DIVIDE, &ignore); 
      decAddOp(b, b, a, &workset, 0, &ignore);         
      decMultiplyOp(a, b, t, &workset, &ignore);       
      } 

    
    
    
    
    approxset=*set;                          
    approxset.round=DEC_ROUND_HALF_EVEN;
    a->exponent+=exp/2;                      
    rstatus=0;                               
    residue=0;                               
    decCopyFit(a, a, &approxset, &residue, &rstatus);  
    decFinish(a, &approxset, &residue, &rstatus);      

    
    
    if (rstatus&DEC_Overflow) {
      status=rstatus;                        
      uprv_decNumberCopy(res, a);                 
      break;
      }

    
    status|=(rstatus & ~(DEC_Rounded|DEC_Inexact));

    
    a->exponent-=exp/2;                      

    
    
    
    
    
    workset.digits--;                             
    t->exponent=-a->digits-1;                     
    decAddOp(b, a, t, &workset, DECNEG, &ignore); 
    workset.round=DEC_ROUND_UP;
    decMultiplyOp(b, b, b, &workset, &ignore);    
    decCompareOp(b, f, b, &workset, COMPARE, &ignore); 
    if (decNumberIsNegative(b)) {                 
      
      t->exponent++;                              
      t->lsu[0]=1;                                
      decAddOp(a, a, t, &workset, DECNEG, &ignore); 
      
      approxset.emin-=exp/2;                      
      approxset.emax-=exp/2;
      decAddOp(a, &dzero, a, &approxset, 0, &ignore);
      }
     else {
      decAddOp(b, a, t, &workset, 0, &ignore);    
      workset.round=DEC_ROUND_DOWN;
      decMultiplyOp(b, b, b, &workset, &ignore);  
      decCompareOp(b, b, f, &workset, COMPARE, &ignore);   
      if (decNumberIsNegative(b)) {               
        t->exponent++;                            
        t->lsu[0]=1;                              
        decAddOp(a, a, t, &workset, 0, &ignore);  
        
        approxset.emin-=exp/2;                    
        approxset.emax-=exp/2;
        decAddOp(a, &dzero, a, &approxset, 0, &ignore);
        }
      }
    
    

    
    a->exponent+=exp/2;                      

    
    
    uprv_decNumberCopy(b, a);
    decTrim(b, set, 1, 1, &dropped);         

    
    
    
    
    if (b->digits*2-1 > workp) {             
      status|=DEC_Inexact|DEC_Rounded;
      }
     else {                                  
      uInt mstatus=0;                        
      decMultiplyOp(b, b, b, &workset, &mstatus); 
      if (mstatus&DEC_Overflow) {            
        status|=DEC_Inexact|DEC_Rounded;
        }
       else {                                
        decCompareOp(t, b, rhs, &workset, COMPARE, &mstatus); 
        if (!ISZERO(t)) status|=DEC_Inexact|DEC_Rounded; 
         else {                              
          
          
          Int todrop=ideal-a->exponent;      
          if (todrop<0) status|=DEC_Rounded; 
           else {                            
            
            Int maxexp=set->emax-set->digits+1;
            Int maxdrop=maxexp-a->exponent;
            if (todrop>maxdrop && set->clamp) { 
              todrop=maxdrop;
              status|=DEC_Clamped;
              }
            if (dropped<todrop) {            
              todrop=dropped;
              status|=DEC_Clamped;
              }
            if (todrop>0) {                  
              decShiftToLeast(a->lsu, D2U(a->digits), todrop);
              a->exponent+=todrop;           
              a->digits-=todrop;             
              }
            }
          }
        }
      }

    
    
    if (status&DEC_Underflow) {
      Int ae=rhs->exponent+rhs->digits-1;    
      
      #if DECEXTFLAG                         
        if (ae>=set->emin*2) status&=~(DEC_Subnormal|DEC_Underflow);
      #else
        if (ae>=set->emin*2) status&=~DEC_Underflow;
      #endif
      
      if (!(status&DEC_Inexact)) status&=~DEC_Underflow;
      }

    uprv_decNumberCopy(res, a);                   
    } while(0);                              

  if (allocbuff!=NULL) free(allocbuff);      
  if (allocbufa!=NULL) free(allocbufa);      
  if (allocbufb!=NULL) free(allocbufb);      
  #if DECSUBSET
  if (allocrhs !=NULL) free(allocrhs);       
  #endif
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 
#if defined(__clang__) || U_GCC_MAJOR_MINOR >= 406
#pragma GCC diagnostic pop
#endif













U_CAPI decNumber * U_EXPORT2 uprv_decNumberSubtract(decNumber *res, const decNumber *lhs,
                              const decNumber *rhs, decContext *set) {
  uInt status=0;                        

  decAddOp(res, lhs, rhs, set, DECNEG, &status);
  if (status!=0) decStatus(res, status, set);
  #if DECCHECK
  decCheckInexact(res, set);
  #endif
  return res;
  } 






















U_CAPI decNumber * U_EXPORT2 uprv_decNumberToIntegralExact(decNumber *res, const decNumber *rhs,
                                     decContext *set) {
  decNumber dn;
  decContext workset;              
  uInt status=0;                   

  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  
  if (SPECIALARG) {
    if (decNumberIsInfinite(rhs)) uprv_decNumberCopy(res, rhs); 
     else decNaNs(res, rhs, NULL, set, &status); 
    }
   else { 
    
    if (rhs->exponent>=0) return uprv_decNumberCopy(res, rhs);
    
    workset=*set;                  
    workset.digits=rhs->digits;    
    workset.traps=0;               
    uprv_decNumberZero(&dn);            
    uprv_decNumberQuantize(res, rhs, &dn, &workset);
    status|=workset.status;
    }
  if (status!=0) decStatus(res, status, set);
  return res;
  } 

U_CAPI decNumber * U_EXPORT2 uprv_decNumberToIntegralValue(decNumber *res, const decNumber *rhs,
                                     decContext *set) {
  decContext workset=*set;         
  workset.traps=0;                 
  uprv_decNumberToIntegralExact(res, rhs, &workset);
  
  
  set->status|=workset.status&DEC_Invalid_operation;
  return res;
  } 
















U_CAPI decNumber * U_EXPORT2 uprv_decNumberXor(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set) {
  const Unit *ua, *ub;                  
  const Unit *msua, *msub;              
  Unit  *uc, *msuc;                     
  Int   msudigs;                        
  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  if (lhs->exponent!=0 || decNumberIsSpecial(lhs) || decNumberIsNegative(lhs)
   || rhs->exponent!=0 || decNumberIsSpecial(rhs) || decNumberIsNegative(rhs)) {
    decStatus(res, DEC_Invalid_operation, set);
    return res;
    }
  
  ua=lhs->lsu;                          
  ub=rhs->lsu;                          
  uc=res->lsu;                          
  msua=ua+D2U(lhs->digits)-1;           
  msub=ub+D2U(rhs->digits)-1;           
  msuc=uc+D2U(set->digits)-1;           
  msudigs=MSUDIGITS(set->digits);       
  for (; uc<=msuc; ua++, ub++, uc++) {  
    Unit a, b;                          
    if (ua>msua) a=0;
     else a=*ua;
    if (ub>msub) b=0;
     else b=*ub;
    *uc=0;                              
    if (a|b) {                          
      Int i, j;
      
      for (i=0; i<DECDPUN; i++) {
        if ((a^b)&1) *uc=*uc+(Unit)powers[i];     
        j=a%10;
        a=a/10;
        j|=b%10;
        b=b/10;
        if (j>1) {
          decStatus(res, DEC_Invalid_operation, set);
          return res;
          }
        if (uc==msuc && i==msudigs-1) break;      
        } 
      } 
    } 
  
  res->digits=decGetDigits(res->lsu, uc-res->lsu);
  res->exponent=0;                      
  res->bits=0;                          
  return res;  
  } 












enum decClass uprv_decNumberClass(const decNumber *dn, decContext *set) {
  if (decNumberIsSpecial(dn)) {
    if (decNumberIsQNaN(dn)) return DEC_CLASS_QNAN;
    if (decNumberIsSNaN(dn)) return DEC_CLASS_SNAN;
    
    if (decNumberIsNegative(dn)) return DEC_CLASS_NEG_INF;
    return DEC_CLASS_POS_INF;
    }
  
  if (uprv_decNumberIsNormal(dn, set)) { 
    if (decNumberIsNegative(dn)) return DEC_CLASS_NEG_NORMAL;
    return DEC_CLASS_POS_NORMAL;
    }
  
  if (decNumberIsZero(dn)) {    
    if (decNumberIsNegative(dn)) return DEC_CLASS_NEG_ZERO;
    return DEC_CLASS_POS_ZERO;
    }
  if (decNumberIsNegative(dn)) return DEC_CLASS_NEG_SUBNORMAL;
  return DEC_CLASS_POS_SUBNORMAL;
  } 







const char *uprv_decNumberClassToString(enum decClass eclass) {
  if (eclass==DEC_CLASS_POS_NORMAL)    return DEC_ClassString_PN;
  if (eclass==DEC_CLASS_NEG_NORMAL)    return DEC_ClassString_NN;
  if (eclass==DEC_CLASS_POS_ZERO)      return DEC_ClassString_PZ;
  if (eclass==DEC_CLASS_NEG_ZERO)      return DEC_ClassString_NZ;
  if (eclass==DEC_CLASS_POS_SUBNORMAL) return DEC_ClassString_PS;
  if (eclass==DEC_CLASS_NEG_SUBNORMAL) return DEC_ClassString_NS;
  if (eclass==DEC_CLASS_POS_INF)       return DEC_ClassString_PI;
  if (eclass==DEC_CLASS_NEG_INF)       return DEC_ClassString_NI;
  if (eclass==DEC_CLASS_QNAN)          return DEC_ClassString_QN;
  if (eclass==DEC_CLASS_SNAN)          return DEC_ClassString_SN;
  return DEC_ClassString_UN;           
  } 












U_CAPI decNumber * U_EXPORT2 uprv_decNumberCopy(decNumber *dest, const decNumber *src) {

  #if DECCHECK
  if (src==NULL) return uprv_decNumberZero(dest);
  #endif

  if (dest==src) return dest;                

  
  
  
  
  dest->bits=src->bits;
  dest->exponent=src->exponent;
  dest->digits=src->digits;
  dest->lsu[0]=src->lsu[0];
  if (src->digits>DECDPUN) {                 
    const Unit *smsup, *s;                   
    Unit  *d;                                
    
    
    d=dest->lsu+1;                           
    smsup=src->lsu+D2U(src->digits);         
    for (s=src->lsu+1; s<smsup; s++, d++) *d=*s;
    }
  return dest;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberCopyAbs(decNumber *res, const decNumber *rhs) {
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, DECUNCONT)) return res;
  #endif
  uprv_decNumberCopy(res, rhs);
  res->bits&=~DECNEG;                   
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberCopyNegate(decNumber *res, const decNumber *rhs) {
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, DECUNCONT)) return res;
  #endif
  uprv_decNumberCopy(res, rhs);
  res->bits^=DECNEG;                    
  return res;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberCopySign(decNumber *res, const decNumber *lhs,
                              const decNumber *rhs) {
  uByte sign;                           
  #if DECCHECK
  if (decCheckOperands(res, DECUNUSED, rhs, DECUNCONT)) return res;
  #endif
  sign=rhs->bits & DECNEG;              
  uprv_decNumberCopy(res, lhs);
  res->bits&=~DECNEG;                   
  res->bits|=sign;                      
  return res;
  } 











U_CAPI uByte * U_EXPORT2 uprv_decNumberGetBCD(const decNumber *dn, uByte *bcd) {
  uByte *ub=bcd+dn->digits-1;      
  const Unit *up=dn->lsu;          

  #if DECDPUN==1                   
    for (; ub>=bcd; ub--, up++) *ub=*up;
  #else                            
    uInt u=*up;                    
    uInt cut=DECDPUN;              
    for (; ub>=bcd; ub--) {
      *ub=(uByte)(u%10);           
      u=u/10;
      cut--;
      if (cut>0) continue;         
      up++;
      u=*up;
      cut=DECDPUN;
      }
  #endif
  return bcd;
  } 













U_CAPI decNumber * U_EXPORT2 uprv_decNumberSetBCD(decNumber *dn, const uByte *bcd, uInt n) {
  Unit *up=dn->lsu+D2U(dn->digits)-1;   
  const uByte *ub=bcd;                  

  #if DECDPUN==1                        
    for (; ub<bcd+n; ub++, up--) *up=*ub;
  #else                                 
    
    Int cut=MSUDIGITS(n);               
    for (;up>=dn->lsu; up--) {          
      *up=0;                            
      for (; cut>0; ub++, cut--) *up=X10(*up)+*ub;
      cut=DECDPUN;                      
      }
  #endif
  dn->digits=n;                         
  return dn;
  } 







Int uprv_decNumberIsNormal(const decNumber *dn, decContext *set) {
  Int ae;                               
  #if DECCHECK
  if (decCheckOperands(DECUNRESU, DECUNUSED, dn, set)) return 0;
  #endif

  if (decNumberIsSpecial(dn)) return 0; 
  if (decNumberIsZero(dn)) return 0;    

  ae=dn->exponent+dn->digits-1;         
  if (ae<set->emin) return 0;           
  return 1;
  } 







Int uprv_decNumberIsSubnormal(const decNumber *dn, decContext *set) {
  Int ae;                               
  #if DECCHECK
  if (decCheckOperands(DECUNRESU, DECUNUSED, dn, set)) return 0;
  #endif

  if (decNumberIsSpecial(dn)) return 0; 
  if (decNumberIsZero(dn)) return 0;    

  ae=dn->exponent+dn->digits-1;         
  if (ae<set->emin) return 1;           
  return 0;
  } 











U_CAPI decNumber * U_EXPORT2 uprv_decNumberTrim(decNumber *dn) {
  Int  dropped;                    
  decContext set;                  
  #if DECCHECK
  if (decCheckOperands(DECUNRESU, DECUNUSED, dn, DECUNCONT)) return dn;
  #endif
  uprv_decContextDefault(&set, DEC_INIT_BASE);    
  return decTrim(dn, &set, 0, 1, &dropped);
  } 






const char * uprv_decNumberVersion(void) {
  return DECVERSION;
  } 










U_CAPI decNumber * U_EXPORT2 uprv_decNumberZero(decNumber *dn) {

  #if DECCHECK
  if (decCheckOperands(dn, DECUNUSED, DECUNUSED, DECUNCONT)) return dn;
  #endif

  dn->bits=0;
  dn->exponent=0;
  dn->digits=1;
  dn->lsu[0]=0;
  return dn;
  } 





















static void decToString(const decNumber *dn, char *string, Flag eng) {
  Int exp=dn->exponent;       
  Int e;                      
  Int pre;                    
  Int cut;                    
  char *c=string;             
  const Unit *up=dn->lsu+D2U(dn->digits)-1; 
  uInt u, pow;                

  #if DECCHECK
  if (decCheckOperands(DECUNRESU, dn, DECUNUSED, DECUNCONT)) {
    strcpy(string, "?");
    return;}
  #endif

  if (decNumberIsNegative(dn)) {   
    *c='-';
    c++;
    }
  if (dn->bits&DECSPECIAL) {       
    if (decNumberIsInfinite(dn)) {
      strcpy(c,   "Inf");
      strcpy(c+3, "inity");
      return;}
    
    if (dn->bits&DECSNAN) {        
      *c='s';
      c++;
      }
    strcpy(c, "NaN");
    c+=3;                          
    
    
    if (exp!=0 || (*dn->lsu==0 && dn->digits==1)) return;
    
    }

  
  cut=MSUDIGITS(dn->digits);       
  cut--;                           

  if (exp==0) {                    
    for (;up>=dn->lsu; up--) {     
      u=*up;                       
      for (; cut>=0; c++, cut--) TODIGIT(u, cut, c, pow);
      cut=DECDPUN-1;               
      }
    *c='\0';                       
    return;}

  
  pre=dn->digits+exp;              
  e=0;                             
  if ((exp>0) || (pre<-5)) {       
    e=exp+dn->digits-1;            
    pre=1;                         
    if (eng && (e!=0)) {           
      Int adj;                     
      
      
      if (e<0) {
        adj=(-e)%3;
        if (adj!=0) adj=3-adj;
        }
       else { 
        adj=e%3;
        }
      e=e-adj;
      
      
      
      if (!ISZERO(dn)) pre+=adj;
       else {  
        if (adj!=0) {              
          e=e+3;
          pre=-(2-adj);
          }
        } 
      } 
    } 

  
  u=*up;
  if (pre>0) {                     
    Int n=pre;
    for (; pre>0; pre--, c++, cut--) {
      if (cut<0) {                 
        if (up==dn->lsu) break;    
        up--;
        cut=DECDPUN-1;
        u=*up;
        }
      TODIGIT(u, cut, c, pow);
      }
    if (n<dn->digits) {            
      *c='.'; c++;
      for (;; c++, cut--) {
        if (cut<0) {               
          if (up==dn->lsu) break;  
          up--;
          cut=DECDPUN-1;
          u=*up;
          }
        TODIGIT(u, cut, c, pow);
        }
      }
     else for (; pre>0; pre--, c++) *c='0'; 
    }
   else {                          
    *c='0'; c++;
    *c='.'; c++;
    for (; pre<0; pre++, c++) *c='0';   
    for (; ; c++, cut--) {
      if (cut<0) {                 
        if (up==dn->lsu) break;    
        up--;
        cut=DECDPUN-1;
        u=*up;
        }
      TODIGIT(u, cut, c, pow);
      }
    }

  


  if (e!=0) {
    Flag had=0;               
    *c='E'; c++;
    *c='+'; c++;              
    u=e;                      
    if (e<0) {
      *(c-1)='-';             
      u=-e;                   
      }
    
    for (cut=9; cut>=0; cut--) {
      TODIGIT(u, cut, c, pow);
      if (*c=='0' && !had) continue;    
      had=1;                            
      c++;                              
      } 
    }
  *c='\0';          
  return;
  } 




































static decNumber * decAddOp(decNumber *res, const decNumber *lhs,
                            const decNumber *rhs, decContext *set,
                            uByte negate, uInt *status) {
  #if DECSUBSET
  decNumber *alloclhs=NULL;        
  decNumber *allocrhs=NULL;        
  #endif
  Int   rhsshift;                  
  Int   maxdigits;                 
  Int   mult;                      
  Int   residue;                   
  uByte bits;                      
  Flag  diffsign;                  
  Unit  *acc;                      
  Unit  accbuff[SD2U(DECBUFFER*2+20)]; 
                                   
                                   
  Unit  *allocacc=NULL;            
  Int   reqdigits=set->digits;     
  Int   padding;                   

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (lhs->digits>reqdigits) {
        alloclhs=decRoundOperand(lhs, set, status);
        if (alloclhs==NULL) break;
        lhs=alloclhs;
        }
      if (rhs->digits>reqdigits) {
        allocrhs=decRoundOperand(rhs, set, status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif
    

    
    diffsign=(Flag)((lhs->bits^rhs->bits^negate)&DECNEG);

    
    if (SPECIALARGS) {                  
      if (SPECIALARGS & (DECSNAN | DECNAN))  
        decNaNs(res, lhs, rhs, set, status);
       else { 
        if (decNumberIsInfinite(lhs)) { 
          
          if (decNumberIsInfinite(rhs) && diffsign) {
            *status|=DEC_Invalid_operation;
            break;
            }
          bits=lhs->bits & DECNEG;      
          }
         else bits=(rhs->bits^negate) & DECNEG;
        bits|=DECINF;
        uprv_decNumberZero(res);
        res->bits=bits;                 
        } 
      break;
      }

    
    if (ISZERO(lhs)) {
      Int adjust;                       
      Int lexp=lhs->exponent;           
      bits=lhs->bits;                   
      residue=0;                        
      decCopyFit(res, rhs, set, &residue, status); 
      res->bits^=negate;                
      #if DECSUBSET
      if (set->extended) {              
      #endif

        adjust=lexp-res->exponent;      /* adjustment needed [if -ve]  */
        if (ISZERO(res)) {              
          if (adjust<0) res->exponent=lexp;  
          
          if (diffsign) {
            if (set->round!=DEC_ROUND_FLOOR) res->bits=0;
             else res->bits=DECNEG;     
            }
          }
         else { 
          if (adjust<0) {     
            if ((res->digits-adjust)>set->digits) {
              adjust=res->digits-set->digits;     
              *status|=DEC_Rounded;               
              }
            res->digits=decShiftToMost(res->lsu, res->digits, -adjust);
            res->exponent+=adjust;                
            }
          } 
      #if DECSUBSET
        } 
      #endif
      decFinish(res, set, &residue, status);      
      break;}

    if (ISZERO(rhs)) {                  
      Int adjust;                       
      Int rexp=rhs->exponent;           
      bits=rhs->bits;                   
      residue=0;                        
      decCopyFit(res, lhs, set, &residue, status); 
      #if DECSUBSET
      if (set->extended) {              
      #endif


        adjust=rexp-res->exponent;      /* adjustment needed [if -ve]  */
        if (adjust<0) {     
          if ((res->digits-adjust)>set->digits) {
            adjust=res->digits-set->digits;     
            *status|=DEC_Rounded;               
            }
          res->digits=decShiftToMost(res->lsu, res->digits, -adjust);
          res->exponent+=adjust;                
          }
      #if DECSUBSET
        } 
      #endif
      decFinish(res, set, &residue, status);      
      break;}

    
    

    
    padding=rhs->exponent-lhs->exponent;

    
    
    
    if (padding==0
        && rhs->digits<=DECDPUN
        && rhs->exponent>=set->emin     
        && rhs->exponent<=set->emax-set->digits+1 
        && rhs->digits<=reqdigits
        && lhs->digits<=reqdigits) {
      Int partial=*lhs->lsu;
      if (!diffsign) {                  
        partial+=*rhs->lsu;
        if ((partial<=DECDPUNMAX)       
         && (lhs->digits>=DECDPUN ||    
             partial<(Int)powers[lhs->digits])) { 
          if (res!=lhs) uprv_decNumberCopy(res, lhs);  
          *res->lsu=(Unit)partial;      
          break;
          }
        
        }
       else {                           
        partial-=*rhs->lsu;
        if (partial>0) { 
          if (res!=lhs) uprv_decNumberCopy(res, lhs);  
          *res->lsu=(Unit)partial;
          
          res->digits=decGetDigits(res->lsu, D2U(res->digits));
          break;
          }
        
        }
      }

    
    
    
    
    
    
    
    rhsshift=0;               
    bits=lhs->bits;           
    mult=1;                   

    
    if (padding!=0) {
      
      
      
      Flag swapped=0;
      if (padding<0) {                  
        const decNumber *t;
        padding=-padding;               
        bits=(uByte)(rhs->bits^negate); 
        t=lhs; lhs=rhs; rhs=t;
        swapped=1;
        }

      
      
      
      if (rhs->digits+padding > lhs->digits+reqdigits+1) {
        
        
        Int shift=reqdigits-rhs->digits;     
        residue=1;                           
        if (diffsign) residue=-residue;      
        
        decCopyFit(res, rhs, set, &residue, status);
        
        if (shift>0) {
          res->digits=decShiftToMost(res->lsu, res->digits, shift);
          res->exponent-=shift;              
          }
        
        if (!swapped) res->bits^=negate;
        decFinish(res, set, &residue, status);    
        break;}

      
      rhsshift=D2U(padding+1)-1;        
      mult=powers[padding-(rhsshift*DECDPUN)]; 
      } 

    if (diffsign) mult=-mult;           

    
    maxdigits=rhs->digits+padding;      
    if (lhs->digits>maxdigits) maxdigits=lhs->digits;

    
    
    acc=res->lsu;                       
    
    
    
    if ((maxdigits>=reqdigits)          
     || (res==rhs && rhsshift>0)) {     
      
      
      Int need=D2U(maxdigits)+1;
      acc=accbuff;                      
      if (need*sizeof(Unit)>sizeof(accbuff)) {
        
        allocacc=(Unit *)malloc(need*sizeof(Unit));
        if (allocacc==NULL) {           
          *status|=DEC_Insufficient_storage;
          break;}
        acc=allocacc;
        }
      }

    res->bits=(uByte)(bits&DECNEG);     
    res->exponent=lhs->exponent;        

    #if DECTRACE
      decDumpAr('A', lhs->lsu, D2U(lhs->digits));
      decDumpAr('B', rhs->lsu, D2U(rhs->digits));
      printf("  :h: %ld %ld\n", rhsshift, mult);
    #endif

    
    U_ASSERT(rhs->digits > 0);
    U_ASSERT(lhs->digits > 0);
    res->digits=decUnitAddSub(lhs->lsu, D2U(lhs->digits),
                              rhs->lsu, D2U(rhs->digits),
                              rhsshift, acc, mult)
               *DECDPUN;           
    if (res->digits<0) {           
      res->digits=-res->digits;
      res->bits^=DECNEG;           
      }
    #if DECTRACE
      decDumpAr('+', acc, D2U(res->digits));
    #endif

    
    
    
    residue=0;                     
    if (acc!=res->lsu) {
      #if DECSUBSET
      if (set->extended) {         
      #endif


        if (res->digits>reqdigits)
          res->digits=decGetDigits(acc, D2U(res->digits));
        decSetCoeff(res, set, acc, res->digits, &residue, status);
      #if DECSUBSET
        }
       else { 
        
        
        
        
        
        if (res->digits<maxdigits) {
          *(acc+D2U(res->digits))=0; 
          res->digits=maxdigits;
          }
         else {
          
          
          
          if (res->digits>reqdigits) {
            res->digits=decGetDigits(acc, D2U(res->digits));
            if (res->digits<maxdigits) res->digits=maxdigits;
            }
          }
        decSetCoeff(res, set, acc, res->digits, &residue, status);
        
        
        if (residue!=0) {
          decApplyRound(res, set, residue, status);
          residue=0;                 
          }
        } 
      #endif
      } 

    
    res->digits=decGetDigits(res->lsu, D2U(res->digits));

    
    decFinish(res, set, &residue, status);

    
    
    
    
    if (ISZERO(res) && diffsign
     #if DECSUBSET
     && set->extended
     #endif
     && (*status&DEC_Inexact)==0) {
      if (set->round==DEC_ROUND_FLOOR) res->bits|=DECNEG;   
                                  else res->bits&=~DECNEG;  
      }
    } while(0);                              

  if (allocacc!=NULL) free(allocacc);        
  #if DECSUBSET
  if (allocrhs!=NULL) free(allocrhs);        
  if (alloclhs!=NULL) free(alloclhs);        
  #endif
  return res;
  } 







































































static decNumber * decDivideOp(decNumber *res,
                               const decNumber *lhs, const decNumber *rhs,
                               decContext *set, Flag op, uInt *status) {
  #if DECSUBSET
  decNumber *alloclhs=NULL;        
  decNumber *allocrhs=NULL;        
  #endif
  Unit  accbuff[SD2U(DECBUFFER+DECDPUN+10)]; 
  Unit  *acc=accbuff;              
  Unit  *allocacc=NULL;            
  Unit  *accnext;                  
  Int   acclength;                 
  Int   accunits;                  
  Int   accdigits;                 

  Unit  varbuff[SD2U(DECBUFFER*2+DECDPUN)];  
  Unit  *var1=varbuff;             
  Unit  *varalloc=NULL;            
  Unit  *msu1;                     

  const Unit *var2;                
  const Unit *msu2;                
  Int   msu2plus;                  
  eInt  msu2pair;                  

  Int   var1units, var2units;      
  Int   var2ulen;                  
  Int   var1initpad=0;             
  Int   maxdigits;                 
  Int   mult;                      
  Unit  thisunit;                  
  Int   residue;                   
  Int   reqdigits=set->digits;     
  Int   exponent;                  
  Int   maxexponent=0;             
  uByte bits;                      
  Unit  *target;                   
  const Unit *source;              
  uInt  const *pow;                
  Int   shift, cut;                
  #if DECSUBSET
  Int   dropped;                   
  #endif

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (lhs->digits>reqdigits) {
        alloclhs=decRoundOperand(lhs, set, status);
        if (alloclhs==NULL) break;
        lhs=alloclhs;
        }
      if (rhs->digits>reqdigits) {
        allocrhs=decRoundOperand(rhs, set, status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif
    

    bits=(lhs->bits^rhs->bits)&DECNEG;  

    
    if (SPECIALARGS) {                  
      if (SPECIALARGS & (DECSNAN | DECNAN)) { 
        decNaNs(res, lhs, rhs, set, status);
        break;
        }
      
      if (decNumberIsInfinite(lhs)) {   
        if (decNumberIsInfinite(rhs) || 
            op & (REMAINDER | REMNEAR)) { 
          *status|=DEC_Invalid_operation;
          break;
          }
        
        uprv_decNumberZero(res);
        res->bits=bits|DECINF;          
        break;
        }
       else {                           
        residue=0;
        if (op&(REMAINDER|REMNEAR)) {
          
          decCopyFit(res, lhs, set, &residue, status);
          }
         else {  
          uprv_decNumberZero(res);
          res->bits=bits;               
          
          
          if (op&DIVIDE) {
            res->exponent=set->emin-set->digits+1;
            *status|=DEC_Clamped;
            }
          }
        decFinish(res, set, &residue, status);
        break;
        }
      }

    
    if (ISZERO(rhs)) {                  
      if (ISZERO(lhs)) {
        uprv_decNumberZero(res);             
        *status|=DEC_Division_undefined;
        }
       else {
        uprv_decNumberZero(res);
        if (op&(REMAINDER|REMNEAR)) *status|=DEC_Invalid_operation;
         else {
          *status|=DEC_Division_by_zero; 
          res->bits=bits|DECINF;         
          }
        }
      break;}

    
    if (ISZERO(lhs)) {                  
      #if DECSUBSET
      if (!set->extended) uprv_decNumberZero(res);
       else {
      #endif
        if (op&DIVIDE) {
          residue=0;
          exponent=lhs->exponent-rhs->exponent; 
          uprv_decNumberCopy(res, lhs);      
          res->bits=bits;               
          res->exponent=exponent;       
          decFinalize(res, set, &residue, status);   
          }
         else if (op&DIVIDEINT) {
          uprv_decNumberZero(res);           
          res->bits=bits;               
          }
         else {                         
          exponent=rhs->exponent;       
          uprv_decNumberCopy(res, lhs);      
          if (exponent<res->exponent) res->exponent=exponent; 
          }
      #if DECSUBSET
        }
      #endif
      break;}

    
    
    
    
    exponent=(lhs->exponent+lhs->digits)-(rhs->exponent+rhs->digits);

    
    
    
    if (exponent<0 && !(op==DIVIDE)) {
      if (op&DIVIDEINT) {
        uprv_decNumberZero(res);                  
        #if DECSUBSET
        if (set->extended)
        #endif
          res->bits=bits;                    
        break;}
      
      
      if (lhs->exponent<=rhs->exponent) {
        if (op&REMAINDER || exponent<-1) {
          
          
          residue=0;
          decCopyFit(res, lhs, set, &residue, status);
          decFinish(res, set, &residue, status);
          break;
          }
        
        }
      } 

    

    
    
    acclength=D2U(reqdigits+DECDPUN);   
    if (acclength*sizeof(Unit)>sizeof(accbuff)) {
      
      allocacc=(Unit *)malloc(acclength*sizeof(Unit));
      if (allocacc==NULL) {             
        *status|=DEC_Insufficient_storage;
        break;}
      acc=allocacc;                     
      }

    
    
    
    
    
    
    
    
    
    
    
    maxdigits=rhs->digits+reqdigits-1;
    if (lhs->digits>maxdigits) maxdigits=lhs->digits;
    var1units=D2U(maxdigits)+2;
    
    if (!(op&DIVIDE)) var1units++;
    if ((var1units+1)*sizeof(Unit)>sizeof(varbuff)) {
      
      varalloc=(Unit *)malloc((var1units+1)*sizeof(Unit));
      if (varalloc==NULL) {             
        *status|=DEC_Insufficient_storage;
        break;}
      var1=varalloc;                    
      }

    
    
    
    
    
    
    msu1=var1+var1units-1;              
    source=lhs->lsu+D2U(lhs->digits)-1; 
    for (target=msu1; source>=lhs->lsu; source--, target--) *target=*source;
    for (; target>=var1; target--) *target=0;

    
    var2ulen=var1units;                 
    var2units=D2U(rhs->digits);         
    var2=rhs->lsu;                      
    msu2=var2+var2units-1;              
    
    
    
    msu2plus=*msu2;                     
    if (var2units>1) msu2plus++;        
    msu2pair=(eInt)*msu2*(DECDPUNMAX+1);
    if (var2units>1) {                  
      msu2pair+=*(msu2-1);              
      if (var2units>2) msu2pair++;      
      }

    
    
    
    
    
    
    for (pow=&powers[1]; *msu1>=*pow; pow++) exponent--;
    for (pow=&powers[1]; *msu2>=*pow; pow++) exponent++;

    
    
    
    
    
    if (!(op&DIVIDE)) {
      Unit *u;                          
      
      var1initpad=(var1units-D2U(lhs->digits))*DECDPUN;
      
      if (exponent<0) cut=-exponent;
       else cut=DECDPUN-exponent%DECDPUN;
      decShiftToLeast(var1, var1units, cut);
      exponent+=cut;                    
      var1initpad-=cut;                 
      
      for (u=msu1; cut>=DECDPUN; cut-=DECDPUN, u--) *u=0;
      } 
     else { 
      maxexponent=lhs->exponent-rhs->exponent;    
      
      
      if (*msu1<*msu2) {
        var2ulen--;                     
        exponent-=DECDPUN;              
        }
      }

    
    accunits=0;                         
    accdigits=0;                        
    accnext=acc+acclength-1;            
    for (;;) {                          
      thisunit=0;                       
      
      for (;;) {                        
        
        
        for (; *msu1==0 && msu1>var1; msu1--) var1units--;

        if (var1units<var2ulen) break;       
        if (var1units==var2ulen) {           
          
          const Unit *pv1, *pv2;
          Unit v2;                           
          pv2=msu2;                          
          for (pv1=msu1; ; pv1--, pv2--) {
            
            v2=0;                            
            if (pv2>=var2) v2=*pv2;          
            if (*pv1!=v2) break;             
            if (pv1==var1) break;            
            }
          
          if (*pv1<v2) break;                
          if (*pv1==v2) {                    
            
            
            
            thisunit++;                      
            *var1=0;                         
            var1units=1;                     
            break;  
            } 
          
          
          
          mult=(Int)(((eInt)*msu1*(DECDPUNMAX+1)+*(msu1-1))/msu2pair);
          } 
         else { 
          
          
          mult=(Int)(((eInt)*msu1*(DECDPUNMAX+1)+*(msu1-1))/msu2plus);
          }
        if (mult==0) mult=1;                 
        
        thisunit=(Unit)(thisunit+mult);      
        
        
        shift=var2ulen-var2units;
        #if DECTRACE
          decDumpAr('1', &var1[shift], var1units-shift);
          decDumpAr('2', var2, var2units);
          printf("m=%ld\n", -mult);
        #endif
        decUnitAddSub(&var1[shift], var1units-shift,
                      var2, var2units, 0,
                      &var1[shift], -mult);
        #if DECTRACE
          decDumpAr('#', &var1[shift], var1units-shift);
        #endif
        
        
        } 

      
      
      if (accunits!=0 || thisunit!=0) {      
        *accnext=thisunit;                   
        
        if (accunits==0) {
          accdigits++;                       
          for (pow=&powers[1]; thisunit>=*pow; pow++) accdigits++;
          }
         else accdigits+=DECDPUN;
        accunits++;                          
        accnext--;                           
        if (accdigits>reqdigits) break;      
        }

      
      
      if (*var1==0 && var1units==1) {        
        if (op&(REMAINDER|REMNEAR)) break;
        if ((op&DIVIDE) && (exponent<=maxexponent)) break;
        
        }
      
      
      if (exponent==0 && !(op&DIVIDE)) break;

      
      
      var2ulen--;                            
      exponent-=DECDPUN;                     
      } 

    
    
    
    
    
    if (accunits==0) {             
      accunits=1;                  
      accdigits=1;                 
      *accnext=0;                  
      }
     else accnext++;               
    

    residue=0;                     
    if (op&DIVIDE) {
      
      if (*var1!=0 || var1units>1) residue=1;
       else { 
        
        
        
        
        #if DECDPUN>1
        Unit lsu=*accnext;
        if (!(lsu&0x01) && (lsu!=0)) {
          
          Int drop=0;
          for (;; drop++) {    
            if (exponent>=maxexponent) break;     
            #if DECDPUN<=4
              if ((lsu-QUOT10(lsu, drop+1)
                  *powers[drop+1])!=0) break;     
            #else
              if (lsu%powers[drop+1]!=0) break;   
            #endif
            exponent++;
            }
          if (drop>0) {
            accunits=decShiftToLeast(accnext, accunits, drop);
            accdigits=decGetDigits(accnext, accunits);
            accunits=D2U(accdigits);
            
            }
          } 
        #endif
        } 
      } 
     else  {
      
      if (accdigits+exponent>reqdigits) {
        *status|=DEC_Division_impossible;
        break;
        }
      if (op & (REMAINDER|REMNEAR)) {
        
        
        Int postshift;                       
        Flag wasodd=0;                       
        Unit *quotlsu;                       
        Int  quotdigits;                     

        bits=lhs->bits;                      

        
        
        if (*var1==0 && var1units==1) {      
          Int exp=lhs->exponent;             
          if (rhs->exponent<exp) exp=rhs->exponent;
          uprv_decNumberZero(res);                
          #if DECSUBSET
          if (set->extended)
          #endif
          res->exponent=exp;                 
          res->bits=(uByte)(bits&DECNEG);          
          decFinish(res, set, &residue, status);   
          break;
          }
        
        if (*accnext & 0x01) wasodd=1;       
        quotlsu=accnext;                     
        quotdigits=accdigits;                

        
        
        
        
        postshift=var1initpad+exponent-lhs->exponent+rhs->exponent;
        
        if (var1initpad<postshift) postshift=var1initpad;

        
        var1units=decShiftToLeast(var1, var1units, postshift);
        accnext=var1;
        accdigits=decGetDigits(var1, var1units);
        accunits=D2U(accdigits);

        exponent=lhs->exponent;         
        if (rhs->exponent<exponent) exponent=rhs->exponent;

        
        
        
        if (op&REMNEAR) {
          Int compare, tarunits;        
          Unit *up;                     
          
          
          
          tarunits=decUnitAddSub(accnext, accunits, accnext, accunits,
                                 0, accnext, 1);
          

          
          
          
          compare=decUnitCompare(accnext, tarunits, rhs->lsu, D2U(rhs->digits),
                                 rhs->exponent-exponent);
          if (compare==BADINT) {             
            *status|=DEC_Insufficient_storage;
            break;}

          
          
          for (up=accnext; up<accnext+tarunits; up++) {
            Int half;              
            half=*up & 0x01;
            *up/=2;                
            if (!half) continue;
            *(up-1)+=(DECDPUNMAX+1)/2;
            }
          

          if (compare>0 || (compare==0 && wasodd)) { 
            Int exp, expunits, exprem;       
            
            
            
            
            Flag allnines=0;                 
            if (quotdigits==reqdigits) {     
              for (up=quotlsu; ; up++) {
                if (quotdigits>DECDPUN) {
                  if (*up!=DECDPUNMAX) break;
                  }
                 else {                      
                  if (*up==powers[quotdigits]-1) allnines=1;
                  break;
                  }
                quotdigits-=DECDPUN;         
                } 
              } 
            if (allnines) {
              *status|=DEC_Division_impossible;
              break;}

            
            
            exp=rhs->exponent-exponent;      
            
            expunits=exp/DECDPUN;
            exprem=exp%DECDPUN;
            
            accunits=-decUnitAddSub(accnext, accunits,
                                    rhs->lsu, D2U(rhs->digits),
                                    expunits, accnext, -(Int)powers[exprem]);
            accdigits=decGetDigits(accnext, accunits); 
            accunits=D2U(accdigits);    
            
            bits^=DECNEG;               
            }
          } 
        } 
      } 

    
    res->exponent=exponent;
    res->bits=(uByte)(bits&DECNEG);          

    
    decSetCoeff(res, set, accnext, accdigits, &residue, status);

    decFinish(res, set, &residue, status);   

    #if DECSUBSET
    
    if (!set->extended && (op==DIVIDE)) decTrim(res, set, 0, 1, &dropped);
    #endif
    } while(0);                              

  if (varalloc!=NULL) free(varalloc);   
  if (allocacc!=NULL) free(allocacc);   
  #if DECSUBSET
  if (allocrhs!=NULL) free(allocrhs);   
  if (alloclhs!=NULL) free(alloclhs);   
  #endif
  return res;
  } 





































#define FASTMUL (DECUSE64 && DECDPUN<5)
static decNumber * decMultiplyOp(decNumber *res, const decNumber *lhs,
                                 const decNumber *rhs, decContext *set,
                                 uInt *status) {
  Int    accunits;                 
  Int    exponent;                 
  Int    residue=0;                
  uByte  bits;                     
  Unit  *acc;                      
  Int    needbytes;                
  void  *allocacc=NULL;            
  Unit  accbuff[SD2U(DECBUFFER*4+1)]; 
                                   
  const Unit *mer, *mermsup;       
  Int   madlength;                 
  Int   shift;                     

  #if FASTMUL
    
    
    #if DECDPUN & 1                
      #define FASTBASE 1000000000  
      #define FASTDIGS          9  
      #define FASTLAZY         18  
    #else
      #define FASTBASE  100000000
      #define FASTDIGS          8
      #define FASTLAZY       1844  
    #endif
    
    
    
    uInt   zlhibuff[(DECBUFFER*2+1)/8+1]; 
    uInt  *zlhi=zlhibuff;                 
    uInt  *alloclhi=NULL;                 
    uInt   zrhibuff[(DECBUFFER*2+1)/8+1]; 
    uInt  *zrhi=zrhibuff;                 
    uInt  *allocrhi=NULL;                 
    uLong  zaccbuff[(DECBUFFER*2+1)/4+2]; 
    
    uLong *zacc=zaccbuff;          
    #if DECDPUN==1
    Int    zoff;                   
    #endif
    uInt  *lip, *rip;              
    uInt  *lmsi, *rmsi;            
    Int    ilhs, irhs, iacc;       
    Int    lazy;                   
    uLong  lcarry;                 
    uInt   carry;                  
    Int    count;                  
    const  Unit *cup;              
    Unit  *up;                     
    uLong *lp;                     
    Int    p;                      
  #endif

  #if DECSUBSET
    decNumber *alloclhs=NULL;      
    decNumber *allocrhs=NULL;      
  #endif

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  
  bits=(uByte)((lhs->bits^rhs->bits)&DECNEG);

  
  if (SPECIALARGS) {               
    if (SPECIALARGS & (DECSNAN | DECNAN)) { 
      decNaNs(res, lhs, rhs, set, status);
      return res;}
    
    if (((lhs->bits & DECINF)==0 && ISZERO(lhs))
      ||((rhs->bits & DECINF)==0 && ISZERO(rhs))) {
      *status|=DEC_Invalid_operation;
      return res;}
    uprv_decNumberZero(res);
    res->bits=bits|DECINF;         
    return res;}

  
  
  
  
  if (lhs->digits<rhs->digits) {   
    const decNumber *hold=lhs;
    lhs=rhs;
    rhs=hold;
    }

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (lhs->digits>set->digits) {
        alloclhs=decRoundOperand(lhs, set, status);
        if (alloclhs==NULL) break;
        lhs=alloclhs;
        }
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif
    

    #if FASTMUL                    
    
    
    #define NEEDTWO (DECDPUN*2)    
    if (rhs->digits>NEEDTWO) {     
      
      ilhs=(lhs->digits+FASTDIGS-1)/FASTDIGS; 
      irhs=(rhs->digits+FASTDIGS-1)/FASTDIGS; 
      iacc=ilhs+irhs;

      
      needbytes=ilhs*sizeof(uInt);
      if (needbytes>(Int)sizeof(zlhibuff)) {
        alloclhi=(uInt *)malloc(needbytes);
        zlhi=alloclhi;}
      needbytes=irhs*sizeof(uInt);
      if (needbytes>(Int)sizeof(zrhibuff)) {
        allocrhi=(uInt *)malloc(needbytes);
        zrhi=allocrhi;}

      
      
      
      
      
      
      

      
      
      U_ASSERT(iacc <= INT32_MAX/sizeof(uLong));
      needbytes=iacc*sizeof(uLong);
      #if DECDPUN==1
      zoff=(iacc+7)/8;        
      needbytes+=zoff*8;
      #endif
      if (needbytes>(Int)sizeof(zaccbuff)) {
        allocacc=(uLong *)malloc(needbytes);
        zacc=(uLong *)allocacc;}
      if (zlhi==NULL||zrhi==NULL||zacc==NULL) {
        *status|=DEC_Insufficient_storage;
        break;}

      acc=(Unit *)zacc;       
      #if DECDPUN==1
      zacc+=zoff;             
      #endif

      
      for (count=lhs->digits, cup=lhs->lsu, lip=zlhi; count>0; lip++)
        for (p=0, *lip=0; p<FASTDIGS && count>0;
             p+=DECDPUN, cup++, count-=DECDPUN)
          *lip+=*cup*powers[p];
      lmsi=lip-1;     
      for (count=rhs->digits, cup=rhs->lsu, rip=zrhi; count>0; rip++)
        for (p=0, *rip=0; p<FASTDIGS && count>0;
             p+=DECDPUN, cup++, count-=DECDPUN)
          *rip+=*cup*powers[p];
      rmsi=rip-1;     

      
      for (lp=zacc; lp<zacc+iacc; lp++) *lp=0;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      lazy=FASTLAZY;                         
      for (rip=zrhi; rip<=rmsi; rip++) {     
        lp=zacc+(rip-zrhi);                  
        for (lip=zlhi; lip<=lmsi; lip++, lp++) { 
          *lp+=(uLong)(*lip)*(*rip);         
          } 
        lazy--;
        if (lazy>0 && rip!=rmsi) continue;
        lazy=FASTLAZY;                       
        
        for (lp=zacc; lp<zacc+iacc; lp++) {
          if (*lp<FASTBASE) continue;        
          lcarry=*lp/FASTBASE;               
          
          
          
          
          if (lcarry<FASTBASE) carry=(uInt)lcarry;  
           else { 
            uInt carry2=(uInt)(lcarry/FASTBASE);    
            *(lp+2)+=carry2;                        
            *lp-=((uLong)FASTBASE*FASTBASE*carry2); 
            carry=(uInt)(lcarry-((uLong)FASTBASE*carry2)); 
            }
          *(lp+1)+=carry;                    
          *lp-=((uLong)FASTBASE*carry);      
          } 
        } 

      
      
      
      
      
      
      for (lp=zacc, up=acc; lp<zacc+iacc; lp++) {
        uInt item=(uInt)*lp;                 
        for (p=0; p<FASTDIGS-DECDPUN; p+=DECDPUN, up++) {
          uInt part=item/(DECDPUNMAX+1);
          *up=(Unit)(item-(part*(DECDPUNMAX+1)));
          item=part;
          } 
        *up=(Unit)item; up++;                
        } 
      accunits=up-acc;                       
      }
     else { 
    #endif

      
      acc=accbuff;                 
      needbytes=(D2U(lhs->digits)+D2U(rhs->digits))*sizeof(Unit);
      if (needbytes>(Int)sizeof(accbuff)) {
        allocacc=(Unit *)malloc(needbytes);
        if (allocacc==NULL) {*status|=DEC_Insufficient_storage; break;}
        acc=(Unit *)allocacc;                
        }

      
      
      
      
      
      accunits=1;                  
      *acc=0;                      
      shift=0;                     
      madlength=D2U(lhs->digits);  
      mermsup=rhs->lsu+D2U(rhs->digits); 

      for (mer=rhs->lsu; mer<mermsup; mer++) {
        
        
        if (*mer!=0) accunits=decUnitAddSub(&acc[shift], accunits-shift,
                                            lhs->lsu, madlength, 0,
                                            &acc[shift], *mer)
                                            + shift;
         else { 
          *(acc+accunits)=0;       
          accunits++;
          }
        
        shift++;                   
        } 
    #if FASTMUL
      } 
    #endif
    
    #if DECTRACE
      decDumpAr('*', acc, accunits);         
    #endif

    
    
    
    res->bits=bits;                          
    res->digits=decGetDigits(acc, accunits); 

    
    
    
    
    
    exponent=lhs->exponent+rhs->exponent;    
    if (lhs->exponent<0 && rhs->exponent<0 && exponent>0)
      exponent=-2*DECNUMMAXE;                
    res->exponent=exponent;                  


    
    decSetCoeff(res, set, acc, res->digits, &residue, status);
    decFinish(res, set, &residue, status);   
    } while(0);                         

  if (allocacc!=NULL) free(allocacc);   
  #if DECSUBSET
  if (allocrhs!=NULL) free(allocrhs);   
  if (alloclhs!=NULL) free(alloclhs);   
  #endif
  #if FASTMUL
  if (allocrhi!=NULL) free(allocrhi);   
  if (alloclhi!=NULL) free(alloclhi);   
  #endif
  return res;
  } 
















































































decNumber * decExpOp(decNumber *res, const decNumber *rhs,
                         decContext *set, uInt *status) {
  uInt ignore=0;                   
  Int h;                           
  Int p;                           
  Int residue;                     
  uInt needbytes;                  
  const decNumber *x=rhs;          
  decContext aset, tset, dset;     
  Int comp;                        

  
  
  
  decNumber bufr[D2N(DECBUFFER*2+1)];
  decNumber *allocrhs=NULL;        

  
  
  

  
  decNumber buft[D2N(DECBUFFER*2+9+1)];
  decNumber *allocbuft=NULL;       
  decNumber *t=buft;               
  
  decNumber bufa[D2N(DECBUFFER*4+18+1)];
  decNumber *allocbufa=NULL;       
  decNumber *a=bufa;               
  
  
  decNumber bufd[D2N(16)];
  decNumber *d=bufd;               
  decNumber numone;                

  #if DECCHECK
  Int iterations=0;                
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  do {                                  
    if (SPECIALARG) {                   
      if (decNumberIsInfinite(rhs)) {   
        if (decNumberIsNegative(rhs))   
          uprv_decNumberZero(res);
         else uprv_decNumberCopy(res, rhs);  
        }
       else decNaNs(res, rhs, NULL, set, status); 
      break;}

    if (ISZERO(rhs)) {                  
      uprv_decNumberZero(res);               
      *res->lsu=1;                      
      break;}                           

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    uprv_decNumberZero(d);                   
    *d->lsu=4;                          
    d->exponent=-set->digits;           
    if (decNumberIsNegative(rhs)) d->exponent--;  
    comp=decCompare(d, rhs, 1);         
    if (comp==BADINT) {
      *status|=DEC_Insufficient_storage;
      break;}
    if (comp>=0) {                      
      Int shift=set->digits-1;
      uprv_decNumberZero(res);               
      *res->lsu=1;                      
      res->digits=decShiftToMost(res->lsu, 1, shift);
      res->exponent=-shift;                  
      *status|=DEC_Inexact | DEC_Rounded;    
      break;} 

    
    
    uprv_decContextDefault(&aset, DEC_INIT_DECIMAL64);
    
    aset.emax=set->emax;                
    aset.emin=set->emin;                
    aset.clamp=0;                       

    
    
    h=rhs->exponent+rhs->digits;
    
    
    
    
    
    if (h>8) {                          
      
      
      
      uprv_decNumberZero(a);
      *a->lsu=2;                        
      if (decNumberIsNegative(rhs)) a->exponent=-2; 
      h=8;                              
      p=9;                              
      }
     else {                             
      Int maxlever=(rhs->digits>8?1:0);
      

      
      
      
      
      
      
      
      
      Int lever=MINI(8-h, maxlever);    
      Int use=-rhs->digits-lever;       
      h+=lever;                         
      if (h<0) {                        
        use+=h;                         
        h=0;
        }
      
      if (rhs->exponent!=use) {
        decNumber *newrhs=bufr;         
        needbytes=sizeof(decNumber)+(D2U(rhs->digits)-1)*sizeof(Unit);
        if (needbytes>sizeof(bufr)) {   
          allocrhs=(decNumber *)malloc(needbytes);
          if (allocrhs==NULL) {         
            *status|=DEC_Insufficient_storage;
            break;}
          newrhs=allocrhs;              
          }
        uprv_decNumberCopy(newrhs, rhs);     
        newrhs->exponent=use;           
        x=newrhs;                       
        
        }

      
      
      
      

      
      
      
      
      
      p=MAXI(x->digits, set->digits)+h+2;    

      
      

      
      
      
      needbytes=sizeof(decNumber)+(D2U(p*2)-1)*sizeof(Unit);
      if (needbytes>sizeof(bufa)) {     
        allocbufa=(decNumber *)malloc(needbytes);
        if (allocbufa==NULL) {          
          *status|=DEC_Insufficient_storage;
          break;}
        a=allocbufa;                    
        }
      
      
      
      
      needbytes=sizeof(decNumber)+(D2U(p+2)-1)*sizeof(Unit);
      if (needbytes>sizeof(buft)) {     
        allocbuft=(decNumber *)malloc(needbytes);
        if (allocbuft==NULL) {          
          *status|=DEC_Insufficient_storage;
          break;}
        t=allocbuft;                    
        }

      uprv_decNumberCopy(t, x);              
      uprv_decNumberZero(a); *a->lsu=1;      
      uprv_decNumberZero(d); *d->lsu=2;      
      uprv_decNumberZero(&numone); *numone.lsu=1; 

      
      uprv_decContextDefault(&tset, DEC_INIT_DECIMAL64);
      dset=tset;
      
      aset.digits=p*2;                  
      
      tset.digits=p;
      tset.emin=DEC_MIN_EMIN;           
      

      
      for (;;) {
        #if DECCHECK
        iterations++;
        #endif
        
        
        decAddOp(a, a, t, &aset, 0, status);           
        decMultiplyOp(t, t, x, &tset, &ignore);        
        decDivideOp(t, t, d, &tset, DIVIDE, &ignore);  
        
        
        
        
        if (((a->digits+a->exponent)>=(t->digits+t->exponent+p+1))
            && (a->digits>=p)) break;
        decAddOp(d, d, &numone, &dset, 0, &ignore);    
        } 

      #if DECCHECK
      
      if (iterations>p+3)
        printf("Exp iterations=%ld, status=%08lx, p=%ld, d=%ld\n",
               (LI)iterations, (LI)*status, (LI)p, (LI)x->digits);
      #endif
      } 

    
    
    if (h>0) {
      Int seenbit=0;               
      Int i;                       
      Int n=powers[h];             
      aset.digits=p+2;             
      
      
      
      uprv_decNumberZero(t); *t->lsu=1; 
      for (i=1;;i++){              
        
        if (*status & (DEC_Overflow|DEC_Underflow)) { 
          if (*status&DEC_Overflow || ISZERO(t)) break;}
        n=n<<1;                    
        if (n<0) {                 
          seenbit=1;               
          decMultiplyOp(t, t, a, &aset, status); 
          }
        if (i==31) break;          
        if (!seenbit) continue;    
        decMultiplyOp(t, t, t, &aset, status); 
        }  
      
      a=t;                         
      }

    
    residue=1;                          
    if (ISZERO(a)) residue=0;           
    aset.digits=set->digits;            
    decCopyFit(res, a, &aset, &residue, status); 
    decFinish(res, set, &residue, status);       
    } while(0);                         

  if (allocrhs !=NULL) free(allocrhs);  
  if (allocbufa!=NULL) free(allocbufa); 
  if (allocbuft!=NULL) free(allocbuft); 
  
  return res;
  } 

















static const uShort LNnn[90]={9016,  8652,  8316,  8008,  7724,  7456,  7208,
  6972,  6748,  6540,  6340,  6148,  5968,  5792,  5628,  5464,  5312,
  5164,  5020,  4884,  4748,  4620,  4496,  4376,  4256,  4144,  4032,
 39233, 38181, 37157, 36157, 35181, 34229, 33297, 32389, 31501, 30629,
 29777, 28945, 28129, 27329, 26545, 25777, 25021, 24281, 23553, 22837,
 22137, 21445, 20769, 20101, 19445, 18801, 18165, 17541, 16925, 16321,
 15721, 15133, 14553, 13985, 13421, 12865, 12317, 11777, 11241, 10717,
 10197,  9685,  9177,  8677,  8185,  7697,  7213,  6737,  6269,  5801,
  5341,  4889,  4437, 39930, 35534, 31186, 26886, 22630, 18418, 14254,
 10130,  6046, 20055};






























































#if defined(__clang__) || U_GCC_MAJOR_MINOR >= 406
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
decNumber * decLnOp(decNumber *res, const decNumber *rhs,
                    decContext *set, uInt *status) {
  uInt ignore=0;                   
  uInt needbytes;                  
  Int residue;                     
  Int r;                           
  Int p;                           
  Int pp;                          
  Int t;                           

  
  
  decNumber bufa[D2N(DECBUFFER+12)];
  decNumber *allocbufa=NULL;       
  decNumber *a=bufa;               
  decNumber bufb[D2N(DECBUFFER*2+2)];
  decNumber *allocbufb=NULL;       
  decNumber *b=bufb;               

  decNumber  numone;               
  decNumber  cmp;                  
  decContext aset, bset;           

  #if DECCHECK
  Int iterations=0;                
  if (decCheckOperands(res, DECUNUSED, rhs, set)) return res;
  #endif

  do {                                  
    if (SPECIALARG) {                   
      if (decNumberIsInfinite(rhs)) {   
        if (decNumberIsNegative(rhs))   
          *status|=DEC_Invalid_operation;
         else uprv_decNumberCopy(res, rhs);  
        }
       else decNaNs(res, rhs, NULL, set, status); 
      break;}

    if (ISZERO(rhs)) {                  
      uprv_decNumberZero(res);               
      res->bits=DECINF|DECNEG;          
      break;}                           

    
    if (decNumberIsNegative(rhs)) {     
      *status|=DEC_Invalid_operation;
      break;}

    

    
    if (rhs->exponent==0 && set->digits<=40) {
      #if DECDPUN==1
      if (rhs->lsu[0]==0 && rhs->lsu[1]==1 && rhs->digits==2) { 
      #else
      if (rhs->lsu[0]==10 && rhs->digits==2) {                  /* ln(10)  */
      #endif
        aset=*set; aset.round=DEC_ROUND_HALF_EVEN;
        #define LN10 "2.302585092994045684017991454684364207601"
        uprv_decNumberFromString(res, LN10, &aset);
        *status|=(DEC_Inexact | DEC_Rounded); 
        break;}
      if (rhs->lsu[0]==2 && rhs->digits==1) { 
        aset=*set; aset.round=DEC_ROUND_HALF_EVEN;
        #define LN2 "0.6931471805599453094172321214581765680755"
        uprv_decNumberFromString(res, LN2, &aset);
        *status|=(DEC_Inexact | DEC_Rounded);
        break;}
      } 

    
    
    
    
    
    p=MAXI(rhs->digits, MAXI(set->digits, 7))+2;

    
    
    
    
    
    
    needbytes=sizeof(decNumber)+(D2U(MAXI(p,16))-1)*sizeof(Unit);
    if (needbytes>sizeof(bufa)) {     
      allocbufa=(decNumber *)malloc(needbytes);
      if (allocbufa==NULL) {          
        *status|=DEC_Insufficient_storage;
        break;}
      a=allocbufa;                    
      }
    pp=p+rhs->digits;
    needbytes=sizeof(decNumber)+(D2U(MAXI(pp,16))-1)*sizeof(Unit);
    if (needbytes>sizeof(bufb)) {     
      allocbufb=(decNumber *)malloc(needbytes);
      if (allocbufb==NULL) {          
        *status|=DEC_Insufficient_storage;
        break;}
      b=allocbufb;                    
      }

    
    
    
    
    
    
    
    

    uprv_decContextDefault(&aset, DEC_INIT_DECIMAL64); 
    r=rhs->exponent+rhs->digits;        
    uprv_decNumberFromInt32(a, r);           
    uprv_decNumberFromInt32(b, 2302585);     
    b->exponent=-6;                     
    decMultiplyOp(a, a, b, &aset, &ignore);  
    
    
    residue=0;                          
    aset.digits=2; aset.round=DEC_ROUND_DOWN;
    decCopyFit(b, rhs, &aset, &residue, &ignore); 
    b->exponent=0;                      
    t=decGetInt(b);                     
    if (t<10) t=X10(t);                 
    t=LNnn[t-10];                       
    uprv_decNumberFromInt32(b, t>>2);        
    b->exponent=-(t&3)-3;               
    b->bits=DECNEG;                     
    aset.digits=16; aset.round=DEC_ROUND_HALF_EVEN; 
    decAddOp(a, a, b, &aset, 0, &ignore); 
    
    
    

    uprv_decNumberZero(&numone); *numone.lsu=1;   

    
    
    aset.emax=set->emax;
    aset.emin=set->emin;
    aset.clamp=0;                       
    
    bset=aset;
    bset.emax=DEC_MAX_MATH*2;           
    bset.emin=-DEC_MAX_MATH*2;          
                                        
    
    
    pp=9;                               
    
    
    aset.digits=pp;                     
    bset.digits=pp+rhs->digits;         
    for (;;) {                          
      #if DECCHECK
      iterations++;
      if (iterations>24) break;         
      #endif
      
      
      
      
      
      a->bits^=DECNEG;                  
      decExpOp(b, a, &bset, &ignore);   
      a->bits^=DECNEG;                  
      
      decMultiplyOp(b, b, rhs, &bset, &ignore);        
      decAddOp(b, b, &numone, &bset, DECNEG, &ignore); 

      
      
      
      
      
      
      

      if (decNumberIsZero(b) ||
          (a->digits+a->exponent)>=(b->digits+b->exponent+set->digits+1)) {
        if (a->digits==p) break;
        if (decNumberIsZero(a)) {
          decCompareOp(&cmp, rhs, &numone, &aset, COMPARE, &ignore); 
          if (cmp.lsu[0]==0) a->exponent=0;            
           else *status|=(DEC_Inexact | DEC_Rounded);  
          break;
          }
        
        if (decNumberIsZero(b)) b->exponent=a->exponent-p;
        }

      
      decAddOp(a, a, b, &aset, 0, &ignore);  
      if (pp==p) continue;                   
      
      pp=pp*2;                               
      if (pp>p) pp=p;                        
      aset.digits=pp;                        
      bset.digits=pp+rhs->digits;            
      } 

    #if DECCHECK
    
    if (iterations>24)
      printf("Ln iterations=%ld, status=%08lx, p=%ld, d=%ld\n",
            (LI)iterations, (LI)*status, (LI)p, (LI)rhs->digits);
    #endif

    
    residue=1;                          
    if (ISZERO(a)) residue=0;           
    aset.digits=set->digits;            
    decCopyFit(res, a, &aset, &residue, status); 
    decFinish(res, set, &residue, status);       
    } while(0);                         

  if (allocbufa!=NULL) free(allocbufa); 
  if (allocbufb!=NULL) free(allocbufb); 
  
  return res;
  } 
#if defined(__clang__) || U_GCC_MAJOR_MINOR >= 406
#pragma GCC diagnostic pop
#endif























static decNumber * decQuantizeOp(decNumber *res, const decNumber *lhs,
                                 const decNumber *rhs, decContext *set,
                                 Flag quant, uInt *status) {
  #if DECSUBSET
  decNumber *alloclhs=NULL;        
  decNumber *allocrhs=NULL;        
  #endif
  const decNumber *inrhs=rhs;      
  Int   reqdigits=set->digits;     
  Int   reqexp;                    
  Int   residue=0;                 
  Int   etiny=set->emin-(reqdigits-1);

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (lhs->digits>reqdigits) {
        alloclhs=decRoundOperand(lhs, set, status);
        if (alloclhs==NULL) break;
        lhs=alloclhs;
        }
      if (rhs->digits>reqdigits) { 
        allocrhs=decRoundOperand(rhs, set, status);
        if (allocrhs==NULL) break;
        rhs=allocrhs;
        }
      }
    #endif
    

    
    if (SPECIALARGS) {
      
      if (SPECIALARGS & (DECSNAN | DECNAN))
        decNaNs(res, lhs, rhs, set, status);
      
      else if ((lhs->bits ^ rhs->bits) & DECINF)
        *status|=DEC_Invalid_operation;
      
      else uprv_decNumberCopy(res, lhs);          
      break;
      }

    
    if (quant) reqexp=inrhs->exponent;  
     else {                             
      
      
      
      reqexp=decGetInt(inrhs);               
      }

    #if DECSUBSET
    if (!set->extended) etiny=set->emin;     
    #endif

    if (reqexp==BADINT                       
     || reqexp==BIGODD || reqexp==BIGEVEN    
     || (reqexp<etiny)                       
     || (reqexp>set->emax)) {                
      *status|=DEC_Invalid_operation;
      break;}

    
    if (ISZERO(lhs)) {                       
      uprv_decNumberCopy(res, lhs);               
      res->exponent=reqexp;                  
      #if DECSUBSET
      if (!set->extended) res->bits=0;       
      #endif
      }
     else {                                  
      Int adjust=reqexp-lhs->exponent;       
      
      if ((lhs->digits-adjust)>reqdigits) {
        *status|=DEC_Invalid_operation;
        break;
        }

      if (adjust>0) {                        
        
        
        decContext workset;                  
        workset=*set;                        
        workset.digits=lhs->digits-adjust;   
        
        decCopyFit(res, lhs, &workset, &residue, status); 
        decApplyRound(res, &workset, residue, status);    
        residue=0;                                        
        
        
        if (res->exponent>reqexp) {
          
          
          if (res->digits==reqdigits) {      
            *status&=~(DEC_Inexact | DEC_Rounded); 
            *status|=DEC_Invalid_operation;
            break;
            }
          res->digits=decShiftToMost(res->lsu, res->digits, 1); 
          res->exponent--;                   
          }
        #if DECSUBSET
        if (ISZERO(res) && !set->extended) res->bits=0; 
        #endif
        } 
       else  {                
        
        
        
        uprv_decNumberCopy(res, lhs);             
        
        if (adjust<0) {
          res->digits=decShiftToMost(res->lsu, res->digits, -adjust);
          res->exponent+=adjust;             
          }
        } 
      } 

    
    
    if (res->exponent>set->emax-res->digits+1) {  
      *status|=DEC_Invalid_operation;
      break;
      }
     else {
      decFinalize(res, set, &residue, status);    
      *status&=~DEC_Underflow;          
      }
    } while(0);                         

  #if DECSUBSET
  if (allocrhs!=NULL) free(allocrhs);   
  if (alloclhs!=NULL) free(alloclhs);   
  #endif
  return res;
  } 































static decNumber * decCompareOp(decNumber *res, const decNumber *lhs,
                         const decNumber *rhs, decContext *set,
                         Flag op, uInt *status) {
  #if DECSUBSET
  decNumber *alloclhs=NULL;        
  decNumber *allocrhs=NULL;        
  #endif
  Int   result=0;                  
  uByte merged;                    

  #if DECCHECK
  if (decCheckOperands(res, lhs, rhs, set)) return res;
  #endif

  do {                             
    #if DECSUBSET
    if (!set->extended) {
      
      if (lhs->digits>set->digits) {
        alloclhs=decRoundOperand(lhs, set, status);
        if (alloclhs==NULL) {result=BADINT; break;}
        lhs=alloclhs;
        }
      if (rhs->digits>set->digits) {
        allocrhs=decRoundOperand(rhs, set, status);
        if (allocrhs==NULL) {result=BADINT; break;}
        rhs=allocrhs;
        }
      }
    #endif
    

    
    if (op==COMPTOTAL) {                
      if (decNumberIsNegative(lhs) && !decNumberIsNegative(rhs)) {
        result=-1;
        break;
        }
      if (!decNumberIsNegative(lhs) && decNumberIsNegative(rhs)) {
        result=+1;
        break;
        }
      }

    
    
    merged=(lhs->bits | rhs->bits) & (DECSNAN | DECNAN);
    if (merged) {                       
      if (op==COMPARE);                 
       else if (op==COMPSIG)            
        *status|=DEC_Invalid_operation | DEC_sNaN;
       else if (op==COMPTOTAL) {        
        
        
        if (!decNumberIsNaN(lhs)) result=-1;
         else if (!decNumberIsNaN(rhs)) result=+1;
         
         else if (decNumberIsSNaN(lhs) && decNumberIsQNaN(rhs)) result=-1;
         else if (decNumberIsQNaN(lhs) && decNumberIsSNaN(rhs)) result=+1;
         else { 
          
          result=decUnitCompare(lhs->lsu, D2U(lhs->digits),
                                rhs->lsu, D2U(rhs->digits), 0);
          
          } 
        if (decNumberIsNegative(lhs)) result=-result;
        break;
        } 

       else if (merged & DECSNAN);           
       else { 
        
        if (!decNumberIsNaN(lhs) || !decNumberIsNaN(rhs)) {
          
          op=COMPMAX;
          if (lhs->bits & DECNAN) result=-1; 
                             else result=+1; 
          break;
          }
        } 
      op=COMPNAN;                            
      decNaNs(res, lhs, rhs, set, status);   
      break;
      }
    
    if (op==COMPMAXMAG || op==COMPMINMAG) result=decCompare(lhs, rhs, 1);
     else result=decCompare(lhs, rhs, 0);    
    } while(0);                              

  if (result==BADINT) *status|=DEC_Insufficient_storage; 
   else {
    if (op==COMPARE || op==COMPSIG ||op==COMPTOTAL) { 
      if (op==COMPTOTAL && result==0) {
        
        
        if (lhs->exponent!=rhs->exponent) {
          if (lhs->exponent<rhs->exponent) result=-1;
           else result=+1;
          if (decNumberIsNegative(lhs)) result=-result;
          } 
        } 
      uprv_decNumberZero(res);               
      if (result!=0) {                  
        *res->lsu=1;
        if (result<0) res->bits=DECNEG;
        }
      }
     else if (op==COMPNAN);             
     else {                             
      Int residue=0;                    
      
      const decNumber *choice;
      if (result==0) { 
        
        uByte slhs=(lhs->bits & DECNEG);
        uByte srhs=(rhs->bits & DECNEG);
        #if DECSUBSET
        if (!set->extended) {           
          op=COMPMAX;
          result=+1;
          }
        else
        #endif
        if (slhs!=srhs) {          
          if (slhs) result=-1;     
               else result=+1;     
          }
         else if (slhs && srhs) {  
          if (lhs->exponent<rhs->exponent) result=+1;
                                      else result=-1;
          
          }
         else {                    
          if (lhs->exponent>rhs->exponent) result=+1;
                                      else result=-1;
          
          }
        } 
      
      if (op==COMPMIN || op==COMPMINMAG) result=-result;
      choice=(result>0 ? lhs : rhs);    
      
      decCopyFit(res, choice, set, &residue, status);
      decFinish(res, set, &residue, status);
      }
    }
  #if DECSUBSET
  if (allocrhs!=NULL) free(allocrhs);   
  if (alloclhs!=NULL) free(alloclhs);   
  #endif
  return res;
  } 













static Int decCompare(const decNumber *lhs, const decNumber *rhs,
                      Flag abs_c) {
  Int   result;                    
  Int   sigr;                      
  Int   compare;                   

  result=1;                                  
  if (ISZERO(lhs)) result=0;
  if (abs_c) {
    if (ISZERO(rhs)) return result;          
    
    if (result==0) return -1;                
    
    }
   else {                                    
    if (result && decNumberIsNegative(lhs)) result=-1;
    sigr=1;                                  
    if (ISZERO(rhs)) sigr=0;
     else if (decNumberIsNegative(rhs)) sigr=-1;
    if (result > sigr) return +1;            
    if (result < sigr) return -1;            
    if (result==0) return 0;                   
    }

  
  if ((lhs->bits | rhs->bits) & DECINF) {    
    if (decNumberIsInfinite(rhs)) {
      if (decNumberIsInfinite(lhs)) result=0;
       else result=-result;                  
      }
    return result;
    }
  
  if (lhs->exponent>rhs->exponent) {         
    
    const decNumber *temp=lhs;
    lhs=rhs;
    rhs=temp;
    result=-result;
    }
  compare=decUnitCompare(lhs->lsu, D2U(lhs->digits),
                         rhs->lsu, D2U(rhs->digits),
                         rhs->exponent-lhs->exponent);
  if (compare!=BADINT) compare*=result;      
  return compare;
  } 


















static Int decUnitCompare(const Unit *a, Int alength,
                          const Unit *b, Int blength, Int exp) {
  Unit  *acc;                      
  Unit  accbuff[SD2U(DECBUFFER*2+1)]; 
  Unit  *allocacc=NULL;            
  Int   accunits, need;            
  const Unit *l, *r, *u;           
  Int   expunits, exprem, result;  

  if (exp==0) {                    
    if (alength>blength) return 1;
    if (alength<blength) return -1;
    
    l=a+alength-1;
    r=b+alength-1;
    for (;l>=a; l--, r--) {
      if (*l>*r) return 1;
      if (*l<*r) return -1;
      }
    return 0;                      
    } 

  
  
  if (alength>blength+(Int)D2U(exp)) return 1;
  if (alength+1<blength+(Int)D2U(exp)) return -1;

  
  
  
  need=blength+D2U(exp);                
  if (need<alength) need=alength;
  need+=2;
  acc=accbuff;                          
  if (need*sizeof(Unit)>sizeof(accbuff)) {
    allocacc=(Unit *)malloc(need*sizeof(Unit));
    if (allocacc==NULL) return BADINT;  
    acc=allocacc;
    }
  
  expunits=exp/DECDPUN;
  exprem=exp%DECDPUN;
  
  accunits=decUnitAddSub(a, alength, b, blength, expunits, acc,
                         -(Int)powers[exprem]);
  
  if (accunits<0) result=-1;            
   else {                               
    
    for (u=acc; u<acc+accunits-1 && *u==0;) u++;
    result=(*u==0 ? 0 : +1);
    }
  
  if (allocacc!=NULL) free(allocacc);   
  return result;
  } 
















































static Int decUnitAddSub(const Unit *a, Int alength,
                         const Unit *b, Int blength, Int bshift,
                         Unit *c, Int m) {
  const Unit *alsu=a;              
  Unit *clsu=c;                    
  Unit *minC;                      
  Unit *maxC;                      
  eInt carry=0;                    
  Int  add;                        
  #if DECDPUN<=4                   
  Int  est;                        
  #endif

  #if DECTRACE
  if (alength<1 || blength<1)
    printf("decUnitAddSub: alen blen m %ld %ld [%ld]\n", alength, blength, m);
  #endif

  maxC=c+alength;                  
  minC=c+blength;                  
  if (bshift!=0) {                 
    minC+=bshift;
    
    if (a==c && bshift<=alength) {
      c+=bshift;
      a+=bshift;
      }
     else for (; c<clsu+bshift; a++, c++) {  
      if (a<alsu+alength) *c=*a;
       else *c=0;
      }
    }
  if (minC>maxC) { 
    Unit *hold=minC;
    minC=maxC;
    maxC=hold;
    }

  
  
  
  
  for (; c<minC; c++) {
    carry+=*a;
    a++;
    carry+=((eInt)*b)*m;                
    b++;                                
    
    if ((ueInt)carry<=DECDPUNMAX) {     
      *c=(Unit)carry;
      carry=0;
      continue;
      }
    #if DECDPUN==4                           
      if (carry>=0) {
        est=(((ueInt)carry>>11)*53687)>>18;
        *c=(Unit)(carry-est*(DECDPUNMAX+1)); 
        carry=est;                           
        if (*c<DECDPUNMAX+1) continue;       
        carry++;
        *c-=DECDPUNMAX+1;
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      est=(((ueInt)carry>>11)*53687)>>18;
      *c=(Unit)(carry-est*(DECDPUNMAX+1));
      carry=est-(DECDPUNMAX+1);              
      if (*c<DECDPUNMAX+1) continue;         
      carry++;
      *c-=DECDPUNMAX+1;
    #elif DECDPUN==3
      if (carry>=0) {
        est=(((ueInt)carry>>3)*16777)>>21;
        *c=(Unit)(carry-est*(DECDPUNMAX+1)); 
        carry=est;                           
        if (*c<DECDPUNMAX+1) continue;       
        carry++;
        *c-=DECDPUNMAX+1;
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      est=(((ueInt)carry>>3)*16777)>>21;
      *c=(Unit)(carry-est*(DECDPUNMAX+1));
      carry=est-(DECDPUNMAX+1);              
      if (*c<DECDPUNMAX+1) continue;         
      carry++;
      *c-=DECDPUNMAX+1;
    #elif DECDPUN<=2
      
      if (carry>=0) {
        est=QUOT10(carry, DECDPUN);
        *c=(Unit)(carry-est*(DECDPUNMAX+1)); 
        carry=est;                           
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      est=QUOT10(carry, DECDPUN);
      *c=(Unit)(carry-est*(DECDPUNMAX+1));
      carry=est-(DECDPUNMAX+1);              
    #else
      
      if ((ueInt)carry<(DECDPUNMAX+1)*2) {   
        *c=(Unit)(carry-(DECDPUNMAX+1));     
        carry=1;
        continue;
        }
      if (carry>=0) {
        *c=(Unit)(carry%(DECDPUNMAX+1));
        carry=carry/(DECDPUNMAX+1);
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      *c=(Unit)(carry%(DECDPUNMAX+1));
      carry=carry/(DECDPUNMAX+1)-(DECDPUNMAX+1);
    #endif
    } 

  
  
  if (c<maxC) for (; c<maxC; c++) {
    if (a<alsu+alength) {               
      carry+=*a;
      a++;
      }
     else {                             
      carry+=((eInt)*b)*m;
      b++;
      }
    
    
    if ((ueInt)carry<=DECDPUNMAX) {     
      *c=(Unit)carry;
      carry=0;
      continue;
      }
    
    #if DECDPUN==4                           
      if (carry>=0) {
        est=(((ueInt)carry>>11)*53687)>>18;
        *c=(Unit)(carry-est*(DECDPUNMAX+1)); 
        carry=est;                           
        if (*c<DECDPUNMAX+1) continue;       
        carry++;
        *c-=DECDPUNMAX+1;
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      est=(((ueInt)carry>>11)*53687)>>18;
      *c=(Unit)(carry-est*(DECDPUNMAX+1));
      carry=est-(DECDPUNMAX+1);              
      if (*c<DECDPUNMAX+1) continue;         
      carry++;
      *c-=DECDPUNMAX+1;
    #elif DECDPUN==3
      if (carry>=0) {
        est=(((ueInt)carry>>3)*16777)>>21;
        *c=(Unit)(carry-est*(DECDPUNMAX+1)); 
        carry=est;                           
        if (*c<DECDPUNMAX+1) continue;       
        carry++;
        *c-=DECDPUNMAX+1;
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      est=(((ueInt)carry>>3)*16777)>>21;
      *c=(Unit)(carry-est*(DECDPUNMAX+1));
      carry=est-(DECDPUNMAX+1);              
      if (*c<DECDPUNMAX+1) continue;         
      carry++;
      *c-=DECDPUNMAX+1;
    #elif DECDPUN<=2
      if (carry>=0) {
        est=QUOT10(carry, DECDPUN);
        *c=(Unit)(carry-est*(DECDPUNMAX+1)); 
        carry=est;                           
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      est=QUOT10(carry, DECDPUN);
      *c=(Unit)(carry-est*(DECDPUNMAX+1));
      carry=est-(DECDPUNMAX+1);              
    #else
      if ((ueInt)carry<(DECDPUNMAX+1)*2){    
        *c=(Unit)(carry-(DECDPUNMAX+1));
        carry=1;
        continue;
        }
      
      if (carry>=0) {
        *c=(Unit)(carry%(DECDPUNMAX+1));
        carry=carry/(DECDPUNMAX+1);
        continue;
        }
      
      carry=carry+(eInt)(DECDPUNMAX+1)*(DECDPUNMAX+1); 
      *c=(Unit)(carry%(DECDPUNMAX+1));
      carry=carry/(DECDPUNMAX+1)-(DECDPUNMAX+1);
    #endif
    } 

  
  
  if (carry==0) return c-clsu;     
  if (carry>0) {                   
    *c=(Unit)carry;                
    c++;                           
    return c-clsu;
    }
  
  add=1;                           
  for (c=clsu; c<maxC; c++) {
    add=DECDPUNMAX+add-*c;
    if (add<=DECDPUNMAX) {
      *c=(Unit)add;
      add=0;
      }
     else {
      *c=0;
      add=1;
      }
    }
  
  #if DECTRACE
    printf("UAS borrow: add %ld, carry %ld\n", add, carry);
  #endif
  if ((add-carry-1)!=0) {
    *c=(Unit)(add-carry-1);
    c++;                      
    }
  return clsu-c;              
  } 
















static decNumber * decTrim(decNumber *dn, decContext *set, Flag all,
                           Flag noclamp, Int *dropped) {
  Int   d, exp;                    
  uInt  cut;                       
  Unit  *up;                       

  #if DECCHECK
  if (decCheckOperands(dn, DECUNUSED, DECUNUSED, DECUNCONT)) return dn;
  #endif

  *dropped=0;                           
  if ((dn->bits & DECSPECIAL)           
    || (*dn->lsu & 0x01)) return dn;    
  if (ISZERO(dn)) {                     
    dn->exponent=0;                     
    return dn;
    }

  
  exp=dn->exponent;
  cut=1;                           
  up=dn->lsu;                      
  for (d=0; d<dn->digits-1; d++) { 
    
    #if DECDPUN<=4
      uInt quot=QUOT10(*up, cut);
      if ((*up-quot*powers[cut])!=0) break;  
    #else
      if (*up%powers[cut]!=0) break;         
    #endif
    
    if (!all) {                    
      
      if (exp<=0) {                
        if (exp==0) break;         
        exp++;                     
        }
      }
    cut++;                         
    if (cut>DECDPUN) {             
      up++;
      cut=1;
      }
    } 
  if (d==0) return dn;             

  
  if (set->clamp && !noclamp) {
    Int maxd=set->emax-set->digits+1-dn->exponent;
    if (maxd<=0) return dn;        
    if (d>maxd) d=maxd;
    }

  
  decShiftToLeast(dn->lsu, D2U(dn->digits), d);
  dn->exponent+=d;                 
  dn->digits-=d;                   
  *dropped=d;                      
  return dn;
  } 











static void decReverse(Unit *ulo, Unit *uhi) {
  Unit temp;
  for (; ulo<uhi; ulo++, uhi--) {
    temp=*ulo;
    *ulo=*uhi;
    *uhi=temp;
    }
  return;
  } 














static Int decShiftToMost(Unit *uar, Int digits, Int shift) {
  Unit  *target, *source, *first;  
  Int   cut;                       
  uInt  next;                      

  if (shift==0) return digits;     
  if ((digits+shift)<=DECDPUN) {   
    *uar=(Unit)(*uar*powers[shift]);
    return digits+shift;
    }

  next=0;                          
  source=uar+D2U(digits)-1;        
  target=source+D2U(shift);        
  cut=DECDPUN-MSUDIGITS(shift);    
  if (cut==0) {                    
    for (; source>=uar; source--, target--) *target=*source;
    }
   else {
    first=uar+D2U(digits+shift)-1; 
    for (; source>=uar; source--, target--) {
      
      #if DECDPUN<=4
        uInt quot=QUOT10(*source, cut);
        uInt rem=*source-quot*powers[cut];
        next+=quot;
      #else
        uInt rem=*source%powers[cut];
        next+=*source/powers[cut];
      #endif
      if (target<=first) *target=(Unit)next;   
      next=rem*powers[DECDPUN-cut];            
      }
    } 

  
  for (; target>=uar; target--) {
    *target=(Unit)next;
    next=0;
    }
  return digits+shift;
  } 














static Int decShiftToLeast(Unit *uar, Int units, Int shift) {
  Unit  *target, *up;              
  Int   cut, count;                
  Int   quot, rem;                 

  if (shift==0) return units;      
  if (shift==units*DECDPUN) {      
    *uar=0;                        
    return 1;                      
    }

  target=uar;                      
  cut=MSUDIGITS(shift);
  if (cut==DECDPUN) {              
    up=uar+D2U(shift);
    for (; up<uar+units; target++, up++) *target=*up;
    return target-uar;
    }

  
  up=uar+D2U(shift-cut);           
  count=units*DECDPUN-shift;       
  #if DECDPUN<=4
    quot=QUOT10(*up, cut);
  #else
    quot=*up/powers[cut];
  #endif
  for (; ; target++) {
    *target=(Unit)quot;
    count-=(DECDPUN-cut);
    if (count<=0) break;
    up++;
    quot=*up;
    #if DECDPUN<=4
      quot=QUOT10(quot, cut);
      rem=*up-quot*powers[cut];
    #else
      rem=quot%powers[cut];
      quot=quot/powers[cut];
    #endif
    *target=(Unit)(*target+rem*powers[DECDPUN-cut]);
    count-=cut;
    if (count<=0) break;
    }
  return target-uar+1;
  } 

#if DECSUBSET


















static decNumber *decRoundOperand(const decNumber *dn, decContext *set,
                                  uInt *status) {
  decNumber *res;                       
  uInt newstatus=0;                     
  Int  residue=0;                       

  
  
  res=(decNumber *)malloc(sizeof(decNumber)
                          +(D2U(set->digits)-1)*sizeof(Unit));
  if (res==NULL) {
    *status|=DEC_Insufficient_storage;
    return NULL;
    }
  decCopyFit(res, dn, set, &residue, &newstatus);
  decApplyRound(res, set, residue, &newstatus);

  
  if (newstatus & DEC_Inexact) newstatus|=DEC_Lost_digits;
  *status|=newstatus;
  return res;
  } 
#endif













static void decCopyFit(decNumber *dest, const decNumber *src,
                       decContext *set, Int *residue, uInt *status) {
  dest->bits=src->bits;
  dest->exponent=src->exponent;
  decSetCoeff(dest, set, src->lsu, src->digits, residue, status);
  } 







































static const uByte resmap[10]={0, 3, 3, 3, 3, 5, 7, 7, 7, 7};
static void decSetCoeff(decNumber *dn, decContext *set, const Unit *lsu,
                        Int len, Int *residue, uInt *status) {
  Int   discard;              
  uInt  cut;                  
  const Unit *up;             
  Unit  *target;              
  Int   count;                
  #if DECDPUN<=4
  uInt  temp;                 
  #endif

  discard=len-set->digits;    
  if (discard<=0) {           
    if (dn->lsu!=lsu) {       
      
      count=len;              
      up=lsu;
      for (target=dn->lsu; count>0; target++, up++, count-=DECDPUN)
        *target=*up;
      dn->digits=len;         
      }
    
    if (*residue!=0) *status|=(DEC_Inexact | DEC_Rounded);
    return;
    }

  
  dn->exponent+=discard;      
  *status|=DEC_Rounded;       
  if (*residue>1) *residue=1; 

  if (discard>len) {          
    
    
    if (*residue<=0) {        
      count=len;              
      for (up=lsu; count>0; up++, count-=DECDPUN) if (*up!=0) { 
        *residue=1;
        break;                
        }
      }
    if (*residue!=0) *status|=DEC_Inexact; 
    *dn->lsu=0;               
    dn->digits=1;             
    return;
    } 

  
  

  
  
  
  count=0;
  for (up=lsu;; up++) {
    count+=DECDPUN;
    if (count>=discard) break; 
    if (*up!=0) *residue=1;
    } 

  
  cut=discard-(count-DECDPUN)-1;
  if (cut==DECDPUN-1) {       
    Unit half=(Unit)powers[DECDPUN]>>1;
    
    if (*up>=half) {
      if (*up>half) *residue=7;
      else *residue+=5;       
      }
     else { 
      if (*up!=0) *residue=3; 
      }
    if (set->digits<=0) {     
      *dn->lsu=0;             
      dn->digits=1;           
      }
     else {                   
      count=set->digits;      
      dn->digits=count;       
      up++;                   
      
      for (target=dn->lsu; count>0; target++, up++, count-=DECDPUN)
        *target=*up;
      }
    } 

   else { 
    uInt  discard1;                
    uInt  quot, rem;               
    if (cut==0) quot=*up;          
     else  {            
      #if DECDPUN<=4
        U_ASSERT(cut >= 0 && cut <= 4);
        quot=QUOT10(*up, cut);
        rem=*up-quot*powers[cut];
      #else
        rem=*up%powers[cut];
        quot=*up/powers[cut];
      #endif
      if (rem!=0) *residue=1;
      }
    
    #if DECDPUN<=4
      temp=(quot*6554)>>16;        
      
      discard1=quot-X10(temp);
      quot=temp;
    #else
      discard1=quot%10;
      quot=quot/10;
    #endif
    
    
    *residue+=resmap[discard1];
    cut++;                         
    
    
    
    if (set->digits<=0) {          
      *dn->lsu=0;                  
      dn->digits=1;                
      }
     else {                        
      count=set->digits;           
      dn->digits=count;            
      
      for (target=dn->lsu; ; target++) {
        *target=(Unit)quot;
        count-=(DECDPUN-cut);
        if (count<=0) break;
        up++;
        quot=*up;
        #if DECDPUN<=4
          quot=QUOT10(quot, cut);
          rem=*up-quot*powers[cut];
        #else
          rem=quot%powers[cut];
          quot=quot/powers[cut];
        #endif
        *target=(Unit)(*target+rem*powers[DECDPUN-cut]);
        count-=cut;
        if (count<=0) break;
        } 
      } 
    } 

  if (*residue!=0) *status|=DEC_Inexact; 
  return;
  } 

































static void decApplyRound(decNumber *dn, decContext *set, Int residue,
                          uInt *status) {
  Int  bump;                  
                              

  if (residue==0) return;     

  bump=0;                     

  
  switch (set->round) {
    case DEC_ROUND_05UP: {    
      
      
      
      
      
      Int lsd5=*dn->lsu%5;     
      if (residue<0 && lsd5!=1) bump=-1;
       else if (residue>0 && lsd5==0) bump=1;
      
      break;} 

    case DEC_ROUND_DOWN: {
      
      if (residue<0) bump=-1;
      break;} 

    case DEC_ROUND_HALF_DOWN: {
      if (residue>5) bump=1;
      break;} 

    case DEC_ROUND_HALF_EVEN: {
      if (residue>5) bump=1;            
       else if (residue==5) {           
        
        if (*dn->lsu & 0x01) bump=1;
        }
      break;} 

    case DEC_ROUND_HALF_UP: {
      if (residue>=5) bump=1;
      break;} 

    case DEC_ROUND_UP: {
      if (residue>0) bump=1;
      break;} 

    case DEC_ROUND_CEILING: {
      
      
      if (decNumberIsNegative(dn)) {
        if (residue<0) bump=-1;
        }
       else {
        if (residue>0) bump=1;
        }
      break;} 

    case DEC_ROUND_FLOOR: {
      
      
      if (!decNumberIsNegative(dn)) {
        if (residue<0) bump=-1;
        }
       else {
        if (residue>0) bump=1;
        }
      break;} 

    default: {      
      *status|=DEC_Invalid_context;
      #if DECTRACE || (DECCHECK && DECVERB)
      printf("Unknown rounding mode: %d\n", set->round);
      #endif
      break;}
    } 

  
  if (bump==0) return;                       

  
  
  
  
  
  if (bump>0) {
    Unit *up;                                
    uInt count=dn->digits;                   
    for (up=dn->lsu; ; up++) {
      if (count<=DECDPUN) {
        
        if (*up!=powers[count]-1) break;     
        
        *up=(Unit)powers[count-1];           
        for (up=up-1; up>=dn->lsu; up--) *up=0; 
        dn->exponent++;                      
        
        if ((dn->exponent+dn->digits)>set->emax+1) {
          decSetOverflow(dn, set, status);
          }
        return;                              
        }
      
      if (*up!=DECDPUNMAX) break;            
      count-=DECDPUN;
      } 
    } 
   else {                                    
    
    
    Unit *up, *sup;                          
    uInt count=dn->digits;                   
    for (up=dn->lsu; ; up++) {
      if (count<=DECDPUN) {
        
        if (*up!=powers[count-1]) break;     
        
        sup=up;                              
        *up=(Unit)powers[count]-1;           
        
        for (up=up-1; up>=dn->lsu; up--) *up=(Unit)powers[DECDPUN]-1;
        dn->exponent--;                      

        
        
        
        
        
        if (dn->exponent+1==set->emin-set->digits+1) {
          if (count==1 && dn->digits==1) *sup=0;  
           else {
            *sup=(Unit)powers[count-1]-1;    
            dn->digits--;
            }
          dn->exponent++;
          *status|=DEC_Underflow | DEC_Subnormal | DEC_Inexact | DEC_Rounded;
          }
        return;                              
        }

      
      if (*up!=0) break;                     
      count-=DECDPUN;
      } 

    } 

  
  decUnitAddSub(dn->lsu, D2U(dn->digits), uarrone, 1, 0, dn->lsu, bump);
  } 

#if DECSUBSET
















static void decFinish(decNumber *dn, decContext *set, Int *residue,
                      uInt *status) {
  if (!set->extended) {
    if ISZERO(dn) {                
      dn->exponent=0;              
      dn->bits=0;                  
      return;                      
      }
    if (dn->exponent>=0) {         
      
      if (set->digits >= (dn->exponent+dn->digits)) {
        dn->digits=decShiftToMost(dn->lsu, dn->digits, dn->exponent);
        dn->exponent=0;
        }
      }
    } 

  decFinalize(dn, set, residue, status);
  } 
#endif















static void decFinalize(decNumber *dn, decContext *set, Int *residue,
                        uInt *status) {
  Int shift;                            
  Int tinyexp=set->emin-dn->digits+1;   

  
  
  

  
  
  if (dn->exponent<=tinyexp) {          
    Int comp;
    decNumber nmin;
    
    if (dn->exponent<tinyexp) {
      
      decSetSubnormal(dn, set, residue, status);
      return;
      }
    
    uprv_decNumberZero(&nmin);
    nmin.lsu[0]=1;
    nmin.exponent=set->emin;
    comp=decCompare(dn, &nmin, 1);                
    if (comp==BADINT) {                           
      *status|=DEC_Insufficient_storage;          
      return;
      }
    if (*residue<0 && comp==0) {                  
      decApplyRound(dn, set, *residue, status);   
      decSetSubnormal(dn, set, residue, status);
      return;
      }
    }

  
  if (*residue!=0) decApplyRound(dn, set, *residue, status);

  
  if (dn->exponent<=set->emax-set->digits+1) return;   


  
  if (dn->exponent>set->emax-dn->digits+1) {           
    decSetOverflow(dn, set, status);
    return;
    }
  
  if (!set->clamp) return;

  
  shift=dn->exponent-(set->emax-set->digits+1);

  
  if (!ISZERO(dn)) {
    dn->digits=decShiftToMost(dn->lsu, dn->digits, shift);
    }
  dn->exponent-=shift;   
  *status|=DEC_Clamped;  
  return;
  } 












static void decSetOverflow(decNumber *dn, decContext *set, uInt *status) {
  Flag needmax=0;                  
  uByte sign=dn->bits&DECNEG;      

  if (ISZERO(dn)) {                
    Int emax=set->emax;                      
    if (set->clamp) emax-=set->digits-1;     
    if (dn->exponent>emax) {                 
      dn->exponent=emax;
      *status|=DEC_Clamped;
      }
    return;
    }

  uprv_decNumberZero(dn);
  switch (set->round) {
    case DEC_ROUND_DOWN: {
      needmax=1;                   
      break;} 
    case DEC_ROUND_05UP: {
      needmax=1;                   
      break;} 
    case DEC_ROUND_CEILING: {
      if (sign) needmax=1;         
      break;} 
    case DEC_ROUND_FLOOR: {
      if (!sign) needmax=1;        
      break;} 
    default: break;                
    }
  if (needmax) {
    decSetMaxValue(dn, set);
    dn->bits=sign;                 
    }
   else dn->bits=sign|DECINF;      
  *status|=DEC_Overflow | DEC_Inexact | DEC_Rounded;
  } 









static void decSetMaxValue(decNumber *dn, decContext *set) {
  Unit *up;                        
  Int count=set->digits;           
  dn->digits=count;
  
  for (up=dn->lsu; ; up++) {
    if (count>DECDPUN) *up=DECDPUNMAX;  
     else {                             
      *up=(Unit)(powers[count]-1);
      break;
      }
    count-=DECDPUN;                
    } 
  dn->bits=0;                      
  dn->exponent=set->emax-set->digits+1;
  } 


















static void decSetSubnormal(decNumber *dn, decContext *set, Int *residue,
                            uInt *status) {
  decContext workset;         
  Int        etiny, adjust;   

  #if DECSUBSET
  
  if (!set->extended) {
    uprv_decNumberZero(dn);
    
    *status|=DEC_Underflow | DEC_Subnormal | DEC_Inexact | DEC_Rounded;
    return;
    }
  #endif

  
  
  etiny=set->emin-(set->digits-1);      

  if ISZERO(dn) {                       
    
    #if DECCHECK
      if (*residue!=0) {
        printf("++ Subnormal 0 residue %ld\n", (LI)*residue);
        *status|=DEC_Invalid_operation;
        }
    #endif
    if (dn->exponent<etiny) {           
      dn->exponent=etiny;
      *status|=DEC_Clamped;
      }
    return;
    }

  *status|=DEC_Subnormal;               
  adjust=etiny-dn->exponent;            
  if (adjust<=0) {                      
    
    
    
    if (*status&DEC_Inexact) *status|=DEC_Underflow;
    return;
    }

  
  
  workset=*set;                         
  workset.digits=dn->digits-adjust;     
  workset.emin-=adjust;                 
  
  decSetCoeff(dn, &workset, dn->lsu, dn->digits, residue, status);
  decApplyRound(dn, &workset, *residue, status);

  
  
  if (*status&DEC_Inexact) *status|=DEC_Underflow;

  
  
  if (dn->exponent>etiny) {
    dn->digits=decShiftToMost(dn->lsu, dn->digits, 1);
    dn->exponent--;                     
    }

  
  if (ISZERO(dn)) *status|=DEC_Clamped;
  } 



















static uInt decCheckMath(const decNumber *rhs, decContext *set,
                         uInt *status) {
  uInt save=*status;                         
  if (set->digits>DEC_MAX_MATH
   || set->emax>DEC_MAX_MATH
   || -set->emin>DEC_MAX_MATH) *status|=DEC_Invalid_context;
   else if ((rhs->digits>DEC_MAX_MATH
     || rhs->exponent+rhs->digits>DEC_MAX_MATH+1
     || rhs->exponent+rhs->digits<2*(1-DEC_MAX_MATH))
     && !ISZERO(rhs)) *status|=DEC_Invalid_operation;
  return (*status!=save);
  } 
















static Int decGetInt(const decNumber *dn) {
  Int  theInt;                          
  const Unit *up;                       
  Int  got;                             
  Int  ilength=dn->digits+dn->exponent; 
  Flag neg=decNumberIsNegative(dn);     

  
  
  #if DEC_MAX_EMAX > 999999999
    #error GetInt may need updating [for Emax]
  #endif
  #if DEC_MIN_EMIN < -999999999
    #error GetInt may need updating [for Emin]
  #endif
  if (ISZERO(dn)) return 0;             

  up=dn->lsu;                           
  theInt=0;                             
  if (dn->exponent>=0) {                
    
    got=dn->exponent;
    }
   else { 
    Int count=-dn->exponent;            
    
    for (; count>=DECDPUN; up++) {
      if (*up!=0) return BADINT;        
      count-=DECDPUN;
      }
    if (count==0) got=0;                
     else {                             
      Int rem;                          
      
      #if DECDPUN<=4
        theInt=QUOT10(*up, count);
        rem=*up-theInt*powers[count];
      #else
        rem=*up%powers[count];          
        theInt=*up/powers[count];
      #endif
      if (rem!=0) return BADINT;        
      
      got=DECDPUN-count;                
      up++;                             
      }
    }
  

  
  if (got==0) {theInt=*up; got+=DECDPUN; up++;} 

  if (ilength<11) {
    Int save=theInt;
    
    for (; got<ilength; up++) {
      theInt+=*up*powers[got];
      got+=DECDPUN;
      }
    if (ilength==10) {                  
      if (theInt/(Int)powers[got-DECDPUN]!=(Int)*(up-1)) ilength=11;
         
       else if (neg && theInt>1999999997) ilength=11;
       else if (!neg && theInt>999999999) ilength=11;
      if (ilength==11) theInt=save;     
      }
    }

  if (ilength>10) {                     
    if (theInt&1) return BIGODD;        
    return BIGEVEN;                     
    }

  if (neg) theInt=-theInt;              
  return theInt;
  } 













static decNumber *decDecap(decNumber *dn, Int drop) {
  Unit *msu;                            
  Int cut;                              
  if (drop>=dn->digits) {               
    #if DECCHECK
    if (drop>dn->digits)
      printf("decDecap called with drop>digits [%ld>%ld]\n",
             (LI)drop, (LI)dn->digits);
    #endif
    dn->lsu[0]=0;
    dn->digits=1;
    return dn;
    }
  msu=dn->lsu+D2U(dn->digits-drop)-1;   
  cut=MSUDIGITS(dn->digits-drop);       
  if (cut!=DECDPUN) *msu%=powers[cut];  
  
  dn->digits=decGetDigits(dn->lsu, msu-dn->lsu+1);
  return dn;
  } 
















static Flag decBiStr(const char *targ, const char *str1, const char *str2) {
  for (;;targ++, str1++, str2++) {
    if (*targ!=*str1 && *targ!=*str2) return 0;
    
    if (*targ=='\0') break;
    } 
  return 1;
  } 















static decNumber * decNaNs(decNumber *res, const decNumber *lhs,
                           const decNumber *rhs, decContext *set,
                           uInt *status) {
  
  
  if (lhs->bits & DECSNAN)
    *status|=DEC_Invalid_operation | DEC_sNaN;
   else if (rhs==NULL);
   else if (rhs->bits & DECSNAN) {
    lhs=rhs;
    *status|=DEC_Invalid_operation | DEC_sNaN;
    }
   else if (lhs->bits & DECNAN);
   else lhs=rhs;

  
  if (lhs->digits<=set->digits) uprv_decNumberCopy(res, lhs); 
   else { 
    const Unit *ul;
    Unit *ur, *uresp1;
    
    res->bits=lhs->bits;                
    uresp1=res->lsu+D2U(set->digits);
    for (ur=res->lsu, ul=lhs->lsu; ur<uresp1; ur++, ul++) *ur=*ul;
    res->digits=D2U(set->digits)*DECDPUN;
    
    if (res->digits>set->digits) decDecap(res, res->digits-set->digits);
    }

  res->bits&=~DECSNAN;        
  res->bits|=DECNAN;          
  res->exponent=0;            
                              
  return res;
  } 
















static void decStatus(decNumber *dn, uInt status, decContext *set) {
  if (status & DEC_NaNs) {              
    
    if (status & DEC_sNaN) status&=~DEC_sNaN;
     else {
      uprv_decNumberZero(dn);                
      dn->bits=DECNAN;                  
      }
    }
  uprv_decContextSetStatus(set, status);     
  return;
  } 














static Int decGetDigits(Unit *uar, Int len) {
  Unit *up=uar+(len-1);            
  Int  digits=(len-1)*DECDPUN+1;   
  #if DECDPUN>4
  uInt const *pow;                 
  #endif
                                   
  #if DECCHECK
  if (len<1) printf("decGetDigits called with len<1 [%ld]\n", (LI)len);
  #endif

  for (; up>=uar; up--) {
    if (*up==0) {                  
      if (digits==1) break;        
      digits-=DECDPUN;             
      continue;}
    
    #if DECDPUN>1                  
    if (*up<10) break;             
    digits++;
    #if DECDPUN>2                  
    if (*up<100) break;            
    digits++;
    #if DECDPUN>3                  
    if (*up<1000) break;           
    digits++;
    #if DECDPUN>4                  
    for (pow=&powers[4]; *up>=*pow; pow++) digits++;
    #endif
    #endif
    #endif
    #endif
    break;
    } 
  return digits;
  } 

#if DECTRACE | DECCHECK








void uprv_decNumberShow(const decNumber *dn) {
  const Unit *up;                  
  uInt u, d;                       
  Int cut;                         
  char isign='+';                  
  if (dn==NULL) {
    printf("NULL\n");
    return;}
  if (decNumberIsNegative(dn)) isign='-';
  printf(" >> %c ", isign);
  if (dn->bits&DECSPECIAL) {       
    if (decNumberIsInfinite(dn)) printf("Infinity");
     else {                                  
      if (dn->bits&DECSNAN) printf("sNaN");  
       else printf("NaN");
      }
    
    if (dn->exponent==0 && dn->digits==1 && *dn->lsu==0) {
      printf("\n");
      return;}
    
    printf(" ");
    }

  
  up=dn->lsu+D2U(dn->digits)-1;         
  printf("%ld", (LI)*up);
  for (up=up-1; up>=dn->lsu; up--) {
    u=*up;
    printf(":");
    for (cut=DECDPUN-1; cut>=0; cut--) {
      d=u/powers[cut];
      u-=d*powers[cut];
      printf("%ld", (LI)d);
      } 
    } 
  if (dn->exponent!=0) {
    char esign='+';
    if (dn->exponent<0) esign='-';
    printf(" E%c%ld", esign, (LI)abs(dn->exponent));
    }
  printf(" [%ld]\n", (LI)dn->digits);
  } 
#endif

#if DECTRACE || DECCHECK






static void decDumpAr(char name, const Unit *ar, Int len) {
  Int i;
  const char *spec;
  #if DECDPUN==9
    spec="%09d ";
  #elif DECDPUN==8
    spec="%08d ";
  #elif DECDPUN==7
    spec="%07d ";
  #elif DECDPUN==6
    spec="%06d ";
  #elif DECDPUN==5
    spec="%05d ";
  #elif DECDPUN==4
    spec="%04d ";
  #elif DECDPUN==3
    spec="%03d ";
  #elif DECDPUN==2
    spec="%02d ";
  #else
    spec="%d ";
  #endif
  printf("  :%c: ", name);
  for (i=len-1; i>=0; i--) {
    if (i==len-1) printf("%ld ", (LI)ar[i]);
     else printf(spec, ar[i]);
    }
  printf("\n");
  return;}
#endif

#if DECCHECK













static Flag decCheckOperands(decNumber *res, const decNumber *lhs,
                             const decNumber *rhs, decContext *set) {
  Flag bad=0;
  if (set==NULL) {                 
    #if DECTRACE || DECVERB
    printf("Reference to context is NULL.\n");
    #endif
    bad=1;
    return 1;}
   else if (set!=DECUNCONT
     && (set->digits<1 || set->round>=DEC_ROUND_MAX)) {
    bad=1;
    #if DECTRACE || DECVERB
    printf("Bad context [digits=%ld round=%ld].\n",
           (LI)set->digits, (LI)set->round);
    #endif
    }
   else {
    if (res==NULL) {
      bad=1;
      #if DECTRACE
      
      printf("Reference to result is NULL.\n");
      #endif
      }
    if (!bad && lhs!=DECUNUSED) bad=(decCheckNumber(lhs));
    if (!bad && rhs!=DECUNUSED) bad=(decCheckNumber(rhs));
    }
  if (bad) {
    if (set!=DECUNCONT) uprv_decContextSetStatus(set, DEC_Invalid_operation);
    if (res!=DECUNRESU && res!=NULL) {
      uprv_decNumberZero(res);
      res->bits=DECNAN;       
      }
    }
  return bad;
  } 









static Flag decCheckNumber(const decNumber *dn) {
  const Unit *up;             
  uInt maxuint;               
  Int ae, d, digits;          
  Int emin, emax;             

  if (dn==NULL) {             
    #if DECTRACE
    
    printf("Reference to decNumber is NULL.\n");
    #endif
    return 1;}

  
  if (dn->bits & DECSPECIAL) {
    if (dn->exponent!=0) {
      #if DECTRACE || DECVERB
      printf("Exponent %ld (not 0) for a special value [%02x].\n",
             (LI)dn->exponent, dn->bits);
      #endif
      return 1;}

    
    if (decNumberIsInfinite(dn)) {
      if (dn->digits!=1) {
        #if DECTRACE || DECVERB
        printf("Digits %ld (not 1) for an infinity.\n", (LI)dn->digits);
        #endif
        return 1;}
      if (*dn->lsu!=0) {
        #if DECTRACE || DECVERB
        printf("LSU %ld (not 0) for an infinity.\n", (LI)*dn->lsu);
        #endif
        decDumpAr('I', dn->lsu, D2U(dn->digits));
        return 1;}
      } 
    
    
    return 0;
    }

  
  if (dn->digits<1 || dn->digits>DECNUMMAXP) {
    #if DECTRACE || DECVERB
    printf("Digits %ld in number.\n", (LI)dn->digits);
    #endif
    return 1;}

  d=dn->digits;

  for (up=dn->lsu; d>0; up++) {
    if (d>DECDPUN) maxuint=DECDPUNMAX;
     else {                   
      maxuint=powers[d]-1;
      if (dn->digits>1 && *up<powers[d-1]) {
        #if DECTRACE || DECVERB
        printf("Leading 0 in number.\n");
        uprv_decNumberShow(dn);
        #endif
        return 1;}
      }
    if (*up>maxuint) {
      #if DECTRACE || DECVERB
      printf("Bad Unit [%08lx] in %ld-digit number at offset %ld [maxuint %ld].\n",
              (LI)*up, (LI)dn->digits, (LI)(up-dn->lsu), (LI)maxuint);
      #endif
      return 1;}
    d-=DECDPUN;
    }

  
  
  
  ae=dn->exponent+dn->digits-1;    
  emax=DECNUMMAXE;
  emin=DECNUMMINE;
  digits=DECNUMMAXP;
  if (ae<emin-(digits-1)) {
    #if DECTRACE || DECVERB
    printf("Adjusted exponent underflow [%ld].\n", (LI)ae);
    uprv_decNumberShow(dn);
    #endif
    return 1;}
  if (ae>+emax) {
    #if DECTRACE || DECVERB
    printf("Adjusted exponent overflow [%ld].\n", (LI)ae);
    uprv_decNumberShow(dn);
    #endif
    return 1;}

  return 0;              
  } 









static void decCheckInexact(const decNumber *dn, decContext *set) {
  #if !DECSUBSET && DECEXTFLAG
    if ((set->status & (DEC_Inexact|DEC_Subnormal))==DEC_Inexact
     && (set->digits!=dn->digits) && !(dn->bits & DECSPECIAL)) {
      #if DECTRACE || DECVERB
      printf("Insufficient digits [%ld] on normal Inexact result.\n",
             (LI)dn->digits);
      uprv_decNumberShow(dn);
      #endif
      uprv_decContextSetStatus(set, DEC_Invalid_operation);
      }
  #else
    
    if (dn!=NULL && dn->digits==0) set->status|=DEC_Invalid_operation;
  #endif
  return;
  } 
#endif

#if DECALLOC
#undef malloc
#undef free














static void *decMalloc(size_t n) {
  uInt  size=n+12;                 
  void  *alloc;                    
  uByte *b, *b0;                   
  uInt  uiwork;                    

  alloc=malloc(size);              
  if (alloc==NULL) return NULL;    
  b0=(uByte *)alloc;               
  decAllocBytes+=n;                
  UBFROMUI(alloc, n);              
  
  for (b=b0+4; b<b0+8; b++) *b=DECFENCE;
  for (b=b0+n+8; b<b0+n+12; b++) *b=DECFENCE;
  return b0+8;                     
  } 













static void decFree(void *alloc) {
  uInt  n;                         
  uByte *b, *b0;                   
  uInt  uiwork;                    

  if (alloc==NULL) return;         
  b0=(uByte *)alloc;               
  b0-=8;                           
  n=UBTOUI(b0);                    
  for (b=b0+4; b<b0+8; b++) if (*b!=DECFENCE)
    printf("=== Corrupt byte [%02x] at offset %d from %ld ===\n", *b,
           b-b0-8, (LI)b0);
  for (b=b0+n+8; b<b0+n+12; b++) if (*b!=DECFENCE)
    printf("=== Corrupt byte [%02x] at offset +%d from %ld, n=%ld ===\n", *b,
           b-b0-8, (LI)b0, (LI)n);
  free(b0);                        
  decAllocBytes-=n;                
  
  } 
#define malloc(a) decMalloc(a)
#define free(a) decFree(a)
#endif
