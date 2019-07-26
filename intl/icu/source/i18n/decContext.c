





















#include <string.h>           
#include <stdio.h>            
#include "decContext.h"       
#include "decNumberLocal.h"   

#if 0  

static  const  Int mfcone=1;                 
static  const  Flag *mfctop=(Flag *)&mfcone; 
#define LITEND *mfctop             /* named flag; 1=little-endian  */
#endif











U_CAPI decContext * U_EXPORT2 uprv_decContextClearStatus(decContext *context, uInt mask) {
  context->status&=~mask;
  return context;
  } 














U_CAPI decContext *  U_EXPORT2 uprv_decContextDefault(decContext *context, Int kind) {
  
  context->digits=9;                         
  context->emax=DEC_MAX_EMAX;                
  context->emin=DEC_MIN_EMIN;                
  context->round=DEC_ROUND_HALF_UP;          
  context->traps=DEC_Errors;                 
  context->status=0;                         
  context->clamp=0;                          
  #if DECSUBSET
  context->extended=0;                       
  #endif
  switch (kind) {
    case DEC_INIT_BASE:
      
      break;
    case DEC_INIT_DECIMAL32:
      context->digits=7;                     
      context->emax=96;                      
      context->emin=-95;                     
      context->round=DEC_ROUND_HALF_EVEN;    
      context->traps=0;                      
      context->clamp=1;                      
      #if DECSUBSET
      context->extended=1;                   
      #endif
      break;
    case DEC_INIT_DECIMAL64:
      context->digits=16;                    
      context->emax=384;                     
      context->emin=-383;                    
      context->round=DEC_ROUND_HALF_EVEN;    
      context->traps=0;                      
      context->clamp=1;                      
      #if DECSUBSET
      context->extended=1;                   
      #endif
      break;
    case DEC_INIT_DECIMAL128:
      context->digits=34;                    
      context->emax=6144;                    
      context->emin=-6143;                   
      context->round=DEC_ROUND_HALF_EVEN;    
      context->traps=0;                      
      context->clamp=1;                      
      #if DECSUBSET
      context->extended=1;                   
      #endif
      break;

    default:                                 
      
      uprv_decContextSetStatus(context, DEC_Invalid_operation); 
    }

  return context;} 









U_CAPI enum rounding  U_EXPORT2 uprv_decContextGetRounding(decContext *context) {
  return context->round;
  } 









U_CAPI uInt  U_EXPORT2 uprv_decContextGetStatus(decContext *context) {
  return context->status;
  } 













U_CAPI decContext * U_EXPORT2 uprv_decContextRestoreStatus(decContext *context,
                                    uInt newstatus, uInt mask) {
  context->status&=~mask;               
  context->status|=(mask&newstatus);    
  return context;
  } 











U_CAPI uInt  U_EXPORT2 uprv_decContextSaveStatus(decContext *context, uInt mask) {
  return context->status&mask;
  } 










U_CAPI decContext * U_EXPORT2 uprv_decContextSetRounding(decContext *context,
                                  enum rounding newround) {
  context->round=newround;
  return context;
  } 











U_CAPI decContext *  U_EXPORT2 uprv_decContextSetStatus(decContext *context, uInt status) {
  context->status|=status;
#if 0  
  if (status & context->traps) raise(SIGFPE);
#endif
  return context;} 















U_CAPI decContext *  U_EXPORT2 uprv_decContextSetStatusFromString(decContext *context,
                                           const char *string) {
  if (strcmp(string, DEC_Condition_CS)==0)
    return uprv_decContextSetStatus(context, DEC_Conversion_syntax);
  if (strcmp(string, DEC_Condition_DZ)==0)
    return uprv_decContextSetStatus(context, DEC_Division_by_zero);
  if (strcmp(string, DEC_Condition_DI)==0)
    return uprv_decContextSetStatus(context, DEC_Division_impossible);
  if (strcmp(string, DEC_Condition_DU)==0)
    return uprv_decContextSetStatus(context, DEC_Division_undefined);
  if (strcmp(string, DEC_Condition_IE)==0)
    return uprv_decContextSetStatus(context, DEC_Inexact);
  if (strcmp(string, DEC_Condition_IS)==0)
    return uprv_decContextSetStatus(context, DEC_Insufficient_storage);
  if (strcmp(string, DEC_Condition_IC)==0)
    return uprv_decContextSetStatus(context, DEC_Invalid_context);
  if (strcmp(string, DEC_Condition_IO)==0)
    return uprv_decContextSetStatus(context, DEC_Invalid_operation);
  #if DECSUBSET
  if (strcmp(string, DEC_Condition_LD)==0)
    return uprv_decContextSetStatus(context, DEC_Lost_digits);
  #endif
  if (strcmp(string, DEC_Condition_OV)==0)
    return uprv_decContextSetStatus(context, DEC_Overflow);
  if (strcmp(string, DEC_Condition_PA)==0)
    return uprv_decContextSetStatus(context, DEC_Clamped);
  if (strcmp(string, DEC_Condition_RO)==0)
    return uprv_decContextSetStatus(context, DEC_Rounded);
  if (strcmp(string, DEC_Condition_SU)==0)
    return uprv_decContextSetStatus(context, DEC_Subnormal);
  if (strcmp(string, DEC_Condition_UN)==0)
    return uprv_decContextSetStatus(context, DEC_Underflow);
  if (strcmp(string, DEC_Condition_ZE)==0)
    return context;
  return NULL;  
  } 















U_CAPI decContext *  U_EXPORT2 uprv_decContextSetStatusFromStringQuiet(decContext *context,
                                                const char *string) {
  if (strcmp(string, DEC_Condition_CS)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Conversion_syntax);
  if (strcmp(string, DEC_Condition_DZ)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Division_by_zero);
  if (strcmp(string, DEC_Condition_DI)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Division_impossible);
  if (strcmp(string, DEC_Condition_DU)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Division_undefined);
  if (strcmp(string, DEC_Condition_IE)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Inexact);
  if (strcmp(string, DEC_Condition_IS)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Insufficient_storage);
  if (strcmp(string, DEC_Condition_IC)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Invalid_context);
  if (strcmp(string, DEC_Condition_IO)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Invalid_operation);
  #if DECSUBSET
  if (strcmp(string, DEC_Condition_LD)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Lost_digits);
  #endif
  if (strcmp(string, DEC_Condition_OV)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Overflow);
  if (strcmp(string, DEC_Condition_PA)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Clamped);
  if (strcmp(string, DEC_Condition_RO)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Rounded);
  if (strcmp(string, DEC_Condition_SU)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Subnormal);
  if (strcmp(string, DEC_Condition_UN)==0)
    return uprv_decContextSetStatusQuiet(context, DEC_Underflow);
  if (strcmp(string, DEC_Condition_ZE)==0)
    return context;
  return NULL;  
  } 










U_CAPI decContext *  U_EXPORT2 uprv_decContextSetStatusQuiet(decContext *context, uInt status) {
  context->status|=status;
  return context;} 









U_CAPI const char * U_EXPORT2 uprv_decContextStatusToString(const decContext *context) {
  Int status=context->status;

  
  
  if (status==DEC_Invalid_operation    ) return DEC_Condition_IO;
  if (status==DEC_Division_by_zero     ) return DEC_Condition_DZ;
  if (status==DEC_Overflow             ) return DEC_Condition_OV;
  if (status==DEC_Underflow            ) return DEC_Condition_UN;
  if (status==DEC_Inexact              ) return DEC_Condition_IE;

  if (status==DEC_Division_impossible  ) return DEC_Condition_DI;
  if (status==DEC_Division_undefined   ) return DEC_Condition_DU;
  if (status==DEC_Rounded              ) return DEC_Condition_RO;
  if (status==DEC_Clamped              ) return DEC_Condition_PA;
  if (status==DEC_Subnormal            ) return DEC_Condition_SU;
  if (status==DEC_Conversion_syntax    ) return DEC_Condition_CS;
  if (status==DEC_Insufficient_storage ) return DEC_Condition_IS;
  if (status==DEC_Invalid_context      ) return DEC_Condition_IC;
  #if DECSUBSET
  if (status==DEC_Lost_digits          ) return DEC_Condition_LD;
  #endif
  if (status==0                        ) return DEC_Condition_ZE;
  return DEC_Condition_MU;  
  } 













#if 0  
U_CAPI Int  U_EXPORT2 uprv_decContextTestEndian(Flag quiet) {
  Int res=0;                  
  uInt dle=(uInt)DECLITEND;   
  if (dle>1) dle=1;           

  if (LITEND!=DECLITEND) {
    const char *adj;
    if (!quiet) {
      if (LITEND) adj="little";
             else adj="big";
      printf("Warning: DECLITEND is set to %d, but this computer appears to be %s-endian\n",
             DECLITEND, adj);
      }
    res=(Int)LITEND-dle;
    }
  return res;
  } 
#endif











U_CAPI  uInt U_EXPORT2 uprv_decContextTestSavedStatus(uInt oldstatus, uInt mask) {
  return (oldstatus&mask)!=0;
  } 











U_CAPI uInt  U_EXPORT2 uprv_decContextTestStatus(decContext *context, uInt mask) {
  return (context->status&mask)!=0;
  } 









U_CAPI decContext * U_EXPORT2 uprv_decContextZeroStatus(decContext *context) {
  context->status=0;
  return context;
  } 

