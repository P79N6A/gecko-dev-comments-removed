




































#include <OSUtils.h>
#include <Timer.h>

#include "primpl.h"

#include "mactime.h"

unsigned long gJanuaryFirst1970Seconds;















static void MyReadLocation(MachineLocation *loc)
{
    static MachineLocation storedLoc;
    static Boolean didReadLocation = false;
    
    if (!didReadLocation) {
        ReadLocation(&storedLoc);
        didReadLocation = true;
    }
    *loc = storedLoc;
}

static long GMTDelta(void)
{
    MachineLocation loc;
    long gmtDelta;

    MyReadLocation(&loc);
    gmtDelta = loc.u.gmtDelta & 0x00ffffff;
    if (gmtDelta & 0x00800000) {    
        gmtDelta |= 0xff000000;
    }
    return gmtDelta;
}

void MacintoshInitializeTime(void)
{
    












    gJanuaryFirst1970Seconds = 2082844800 + GMTDelta();
}















PRTime PR_Now(void)
{
    unsigned long currentTime;    


    PRTime retVal;
    int64  usecPerSec;

    




    GetDateTime(&currentTime);

    













    currentTime = currentTime - 2082844800 - GMTDelta();

    
    LL_I2L(usecPerSec, PR_USEC_PER_SEC);
    LL_I2L(retVal, currentTime);
    LL_MUL(retVal, retVal, usecPerSec);

    return retVal;
}


















PRTimeParameters PR_LocalTimeParameters(const PRExplodedTime *gmt)
{
#pragma unused (gmt)

    PRTimeParameters retVal;
    MachineLocation loc;

    MyReadLocation(&loc);

    






    retVal.tp_gmt_offset = loc.u.gmtDelta & 0x00ffffff;
    if (retVal.tp_gmt_offset & 0x00800000) {    
	retVal.tp_gmt_offset |= 0xff000000;
    }

    





    if (loc.u.dlsDelta) {
    	retVal.tp_gmt_offset -= 3600;
    	retVal.tp_dst_offset = 3600;
    } else {
    	retVal.tp_dst_offset = 0;
    }
    return retVal;
}

PRIntervalTime _MD_GetInterval(void)
{
    PRIntervalTime retVal;
    PRUint64 upTime, microtomilli;	

    



    Microseconds((UnsignedWide *)&upTime);
    LL_I2L(microtomilli, PR_USEC_PER_MSEC);
    LL_DIV(upTime, upTime, microtomilli);
    LL_L2I(retVal, upTime);
	
    return retVal;
}

struct tm *Maclocaltime(const time_t * t)
{
	DateTimeRec dtr;
	MachineLocation loc;
	time_t macLocal = *t + gJanuaryFirst1970Seconds; 
	static struct tm statictime;
	static const short monthday[12] =
		{0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

	SecondsToDate(macLocal, &dtr);
	statictime.tm_sec = dtr.second;
	statictime.tm_min = dtr.minute;
	statictime.tm_hour = dtr.hour;
	statictime.tm_mday = dtr.day;
	statictime.tm_mon = dtr.month - 1;
	statictime.tm_year = dtr.year - 1900;
	statictime.tm_wday = dtr.dayOfWeek - 1;
	statictime.tm_yday = monthday[statictime.tm_mon]
		+ statictime.tm_mday - 1;
	if (2 < statictime.tm_mon && !(statictime.tm_year & 3))
		++statictime.tm_yday;
	MyReadLocation(&loc);
	statictime.tm_isdst = loc.u.dlsDelta;
	return(&statictime);
}


