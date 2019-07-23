



































#ifndef _SECDER_H_
#define _SECDER_H_

#include "utilrename.h"








#if defined(_WIN32_WCE)
#else
#include <time.h>
#endif

#include "plarena.h"
#include "prlong.h"

#include "seccomon.h"
#include "secdert.h"
#include "prtime.h"

SEC_BEGIN_PROTOS









extern SECStatus DER_Encode(PLArenaPool *arena, SECItem *dest, DERTemplate *t,
			   void *src);

extern SECStatus DER_Lengths(SECItem *item, int *header_len_p,
                             PRUint32 *contents_len_p);










extern unsigned char *DER_StoreHeader(unsigned char *to, unsigned int code,
				      PRUint32 encodingLen);




extern int DER_LengthLength(PRUint32 len);





extern SECStatus DER_SetInteger(PLArenaPool *arena, SECItem *dst, PRInt32 src);





extern SECStatus DER_SetUInteger(PLArenaPool *arena, SECItem *dst, PRUint32 src);






extern long DER_GetInteger(SECItem *src);






extern unsigned long DER_GetUInteger(SECItem *src);










extern SECStatus DER_TimeToUTCTime(SECItem *result, PRTime time);
extern SECStatus DER_TimeToUTCTimeArena(PLArenaPool* arenaOpt,
                                        SECItem *dst, PRTime gmttime);








extern SECStatus DER_AsciiToTime(PRTime *result, const char *string);




extern SECStatus DER_UTCTimeToTime(PRTime *result, const SECItem *time);






extern char *DER_UTCTimeToAscii(SECItem *utcTime);







extern char *DER_UTCDayToAscii(SECItem *utctime);

extern char *DER_GeneralizedDayToAscii(SECItem *gentime);

extern char *DER_TimeChoiceDayToAscii(SECItem *timechoice);






extern SECStatus DER_TimeToGeneralizedTime(SECItem *dst, PRTime gmttime);
extern SECStatus DER_TimeToGeneralizedTimeArena(PLArenaPool* arenaOpt,
                                                SECItem *dst, PRTime gmttime);






extern SECStatus DER_GeneralizedTimeToTime(PRTime *dst, const SECItem *time);





extern char *CERT_UTCTime2FormattedAscii (PRTime utcTime, char *format);
#define CERT_GeneralizedTime2FormattedAscii CERT_UTCTime2FormattedAscii





extern char *CERT_GenTime2FormattedAscii (PRTime genTime, char *format);






extern SECStatus DER_DecodeTimeChoice(PRTime* output, const SECItem* input);




extern SECStatus DER_EncodeTimeChoice(PLArenaPool* arena, SECItem* output,
                                       PRTime input);

SEC_END_PROTOS

#endif 

