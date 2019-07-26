










#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)asctime.c	8.2";
#endif 
#endif 



#include "private.h"
#include "tzfile.h"



















#ifdef __GNUC__
#define ASCTIME_FMT	"%.3s %.3s%3d %2.2d:%2.2d:%2.2d %-4s\n"
#else 
#define ASCTIME_FMT	"%.3s %.3s%3d %02.2d:%02.2d:%02.2d %-4s\n"
#endif 






#ifdef __GNUC__
#define ASCTIME_FMT_B	"%.3s %.3s%3d %2.2d:%2.2d:%2.2d     %s\n"
#else 
#define ASCTIME_FMT_B	"%.3s %.3s%3d %02.2d:%02.2d:%02.2d     %s\n"
#endif 

#define STD_ASCTIME_BUF_SIZE	26










#define MAX_ASCTIME_BUF_SIZE	(2*3+5*INT_STRLEN_MAXIMUM(int)+7+2+1+1)

static char	buf_asctime[MAX_ASCTIME_BUF_SIZE];





char *
asctime_r(timeptr, buf)
register const struct tm *	timeptr;
char *				buf;
{
	static const char	wday_name[][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static const char	mon_name[][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	register const char *	wn;
	register const char *	mn;
	char			year[INT_STRLEN_MAXIMUM(int) + 2];
	char			result[MAX_ASCTIME_BUF_SIZE];

	if (timeptr->tm_wday < 0 || timeptr->tm_wday >= DAYSPERWEEK)
		wn = "???";
	else	wn = wday_name[timeptr->tm_wday];
	if (timeptr->tm_mon < 0 || timeptr->tm_mon >= MONSPERYEAR)
		mn = "???";
	else	mn = mon_name[timeptr->tm_mon];
	





	(void) strftime(year, sizeof year, "%Y", timeptr);
	


	(void) sprintf(result,
		((strlen(year) <= 4) ? ASCTIME_FMT : ASCTIME_FMT_B),
		wn, mn,
		timeptr->tm_mday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec,
		year);
	if (strlen(result) < STD_ASCTIME_BUF_SIZE || buf == buf_asctime) {
		(void) strcpy(buf, result);
		return buf;
	} else {
#ifdef EOVERFLOW
		errno = EOVERFLOW;
#else 
		errno = EINVAL;
#endif 
		return NULL;
	}
}





char *
asctime(timeptr)
register const struct tm *	timeptr;
{
	return asctime_r(timeptr, buf_asctime);
}
