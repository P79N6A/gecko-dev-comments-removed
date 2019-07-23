


















































#ifndef BASE_PRTIME_H__
#define BASE_PRTIME_H__

#include "base/logging.h"
#include "base/third_party/nspr/prtypes.h"

#ifdef CHROMIUM_MOZILLA_BUILD
PR_BEGIN_EXTERN_C
#endif

#define PR_ASSERT DCHECK

#define LL_I2L(l, i)    ((l) = (PRInt64)(i))
#define LL_MUL(r, a, b) ((r) = (a) * (b))





#define PR_MSEC_PER_SEC		1000UL
#define PR_USEC_PER_SEC		1000000UL
#define PR_NSEC_PER_SEC		1000000000UL
#define PR_USEC_PER_MSEC	1000UL
#define PR_NSEC_PER_MSEC	1000000UL
















typedef PRInt64 PRTime;






typedef struct PRTimeParameters {
    PRInt32 tp_gmt_offset;     
    PRInt32 tp_dst_offset;     
} PRTimeParameters;
























typedef struct PRExplodedTime {
    PRInt32 tm_usec;		    
    PRInt32 tm_sec;             
	
    PRInt32 tm_min;             
    PRInt32 tm_hour;            
    PRInt32 tm_mday;            

    PRInt32 tm_month;           
    PRInt16 tm_year;            


    PRInt8 tm_wday;		        

    PRInt16 tm_yday;            


    PRTimeParameters tm_params;  
} PRExplodedTime;
























typedef PRTimeParameters (PR_CALLBACK *PRTimeParamFn)(const PRExplodedTime *gmt);





NSPR_API(PRTime)
PR_ImplodeTime(const PRExplodedTime *exploded);













NSPR_API(void) PR_NormalizeTime(
    PRExplodedTime *exploded, PRTimeParamFn params);






NSPR_API(PRTimeParameters) PR_GMTParameters(const PRExplodedTime *gmt);
































NSPR_API(PRStatus) PR_ParseTimeString (
	const char *string,
	PRBool default_to_gmt,
	PRTime *result);

#ifdef CHROMIUM_MOZILLA_BUILD
PR_END_EXTERN_C
#endif

#endif  
