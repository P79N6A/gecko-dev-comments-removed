



































#include "prlong.h"
#include "prtime.h"
#include "secder.h"
#include "secitem.h"
#include "secerr.h"

static const PRTime January1st2050  = LL_INIT(0x0008f81e, 0x1b098000);

static char *DecodeUTCTime2FormattedAscii (SECItem *utcTimeDER, char *format);
static char *DecodeGeneralizedTime2FormattedAscii (SECItem *generalizedTimeDER, char *format);


char *
DER_UTCTimeToAscii(SECItem *utcTime)
{
    return (DecodeUTCTime2FormattedAscii (utcTime, "%a %b %d %H:%M:%S %Y"));
}


char *
DER_UTCDayToAscii(SECItem *utctime)
{
    return (DecodeUTCTime2FormattedAscii (utctime, "%a %b %d, %Y"));
}



char *
DER_GeneralizedDayToAscii(SECItem *gentime)
{
    return (DecodeGeneralizedTime2FormattedAscii (gentime, "%a %b %d, %Y"));
}



char *
DER_TimeChoiceDayToAscii(SECItem *timechoice)
{
    switch (timechoice->type) {

    case siUTCTime:
        return DER_UTCDayToAscii(timechoice);

    case siGeneralizedTime:
        return DER_GeneralizedDayToAscii(timechoice);

    default:
        PORT_Assert(0);
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return NULL;
    }
}

char *
CERT_UTCTime2FormattedAscii (int64 utcTime, char *format)
{
    PRExplodedTime printableTime; 
    char *timeString;
   
    
    PR_ExplodeTime(utcTime, PR_LocalTimeParameters, &printableTime);
    
    timeString = (char *)PORT_Alloc(256);

    if ( timeString ) {
        if ( ! PR_FormatTime( timeString, 256, format, &printableTime )) {
            PORT_Free(timeString);
            timeString = NULL;
        }
    }
    
    return (timeString);
}

char *CERT_GenTime2FormattedAscii (int64 genTime, char *format)
{
    PRExplodedTime printableTime; 
    char *timeString;
   
    
    PR_ExplodeTime(genTime, PR_GMTParameters, &printableTime);
    
    timeString = (char *)PORT_Alloc(256);

    if ( timeString ) {
        if ( ! PR_FormatTime( timeString, 256, format, &printableTime )) {
            PORT_Free(timeString);
            timeString = NULL;
            PORT_SetError(SEC_ERROR_OUTPUT_LEN);
        }
    }
    
    return (timeString);
}





static char *
DecodeUTCTime2FormattedAscii (SECItem *utcTimeDER,  char *format)
{
    int64 utcTime;
    int rv;
   
    rv = DER_UTCTimeToTime(&utcTime, utcTimeDER);
    if (rv) {
        return(NULL);
    }
    return (CERT_UTCTime2FormattedAscii (utcTime, format));
}




static char *
DecodeGeneralizedTime2FormattedAscii (SECItem *generalizedTimeDER,  char *format)
{
    PRTime generalizedTime;
    int rv;
   
    rv = DER_GeneralizedTimeToTime(&generalizedTime, generalizedTimeDER);
    if (rv) {
        return(NULL);
    }
    return (CERT_GeneralizedTime2FormattedAscii (generalizedTime, format));
}




SECStatus DER_DecodeTimeChoice(PRTime* output, const SECItem* input)
{
    switch (input->type) {
        case siGeneralizedTime:
            return DER_GeneralizedTimeToTime(output, input);

        case siUTCTime:
            return DER_UTCTimeToTime(output, input);

        default:
            PORT_SetError(SEC_ERROR_INVALID_ARGS);
            PORT_Assert(0);
            return SECFailure;
    }
}




SECStatus DER_EncodeTimeChoice(PRArenaPool* arena, SECItem* output, PRTime input)
{
    if (LL_CMP(input, >, January1st2050)) {
        return DER_TimeToGeneralizedTimeArena(arena, output, input);
    } else {
        return DER_TimeToUTCTimeArena(arena, output, input);
    }
}
