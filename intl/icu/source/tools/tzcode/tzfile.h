#ifndef TZFILE_H

#define TZFILE_H


















#ifndef TZDIR
#define TZDIR	"/usr/local/etc/zoneinfo" /* Time zone object file directory */
#endif 

#ifndef TZDEFAULT
#define TZDEFAULT	"localtime"
#endif 

#ifndef TZDEFRULES
#define TZDEFRULES	"posixrules"
#endif 





#define	TZ_MAGIC	"TZif"

struct tzhead {
	char	tzh_magic[4];		
	char	tzh_version[1];		
	char	tzh_reserved[15];	
	char	tzh_ttisgmtcnt[4];	
	char	tzh_ttisstdcnt[4];	
	char	tzh_leapcnt[4];		
	char	tzh_timecnt[4];		
	char	tzh_typecnt[4];		
	char	tzh_charcnt[4];		
};
















































#ifndef TZ_MAX_TIMES
#define TZ_MAX_TIMES	2000
#endif 

#ifndef TZ_MAX_TYPES

#define TZ_MAX_TYPES	256 /* Limited by what (unsigned char)'s can hold */
#endif 

#ifndef TZ_MAX_CHARS
#define TZ_MAX_CHARS	50	/* Maximum number of abbreviation characters */
				
#endif 

#ifndef TZ_MAX_LEAPS
#define TZ_MAX_LEAPS	50	/* Maximum number of leap second corrections */
#endif 

#define SECSPERMIN	60
#define MINSPERHOUR	60
#define HOURSPERDAY	24
#define DAYSPERWEEK	7
#define DAYSPERNYEAR	365
#define DAYSPERLYEAR	366
#define SECSPERHOUR	(SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	((int_fast32_t) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR	12

#define TM_SUNDAY	0
#define TM_MONDAY	1
#define TM_TUESDAY	2
#define TM_WEDNESDAY	3
#define TM_THURSDAY	4
#define TM_FRIDAY	5
#define TM_SATURDAY	6

#define TM_JANUARY	0
#define TM_FEBRUARY	1
#define TM_MARCH	2
#define TM_APRIL	3
#define TM_MAY		4
#define TM_JUNE		5
#define TM_JULY		6
#define TM_AUGUST	7
#define TM_SEPTEMBER	8
#define TM_OCTOBER	9
#define TM_NOVEMBER	10
#define TM_DECEMBER	11

#define TM_YEAR_BASE	1900

#define EPOCH_YEAR	1970
#define EPOCH_WDAY	TM_THURSDAY

#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))













#define isleap_sum(a, b)	isleap((a) % 400 + (b) % 400)

#endif 
