














































#ifndef prtime_h___
#define prtime_h___

#include "prlong.h"

PR_BEGIN_EXTERN_C





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
PR_Now(void);











NSPR_API(void) PR_ExplodeTime(
    PRTime usecs, PRTimeParamFn params, PRExplodedTime *exploded);


NSPR_API(PRTime)
PR_ImplodeTime(const PRExplodedTime *exploded);













NSPR_API(void) PR_NormalizeTime(
    PRExplodedTime *exploded, PRTimeParamFn params);






NSPR_API(PRTimeParameters) PR_LocalTimeParameters(const PRExplodedTime *gmt);


NSPR_API(PRTimeParameters) PR_GMTParameters(const PRExplodedTime *gmt);





NSPR_API(PRTimeParameters) PR_USPacificTimeParameters(const PRExplodedTime *gmt);




































NSPR_API(PRStatus) PR_ParseTimeStringToExplodedTime (
        const char *string,
        PRBool default_to_gmt,
        PRExplodedTime *result);









NSPR_API(PRStatus) PR_ParseTimeString (
	const char *string,
	PRBool default_to_gmt,
	PRTime *result);










#ifndef NO_NSPR_10_SUPPORT


NSPR_API(PRUint32) PR_FormatTime(char *buf, int buflen, const char *fmt,
                                           const PRExplodedTime *tm);




NSPR_API(PRUint32)
PR_FormatTimeUSEnglish( char* buf, PRUint32 bufSize,
                        const char* format, const PRExplodedTime* tm );

#endif 

PR_END_EXTERN_C

#endif 
