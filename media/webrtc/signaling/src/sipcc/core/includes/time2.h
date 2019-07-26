



#ifndef TIME2_H
#define TIME2_H

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>





#define EPOCH_YEAR                  (1900)
#define EPOCH_SECONDS               (0x80000000)
#define DAYS_PER_LEAP_YEAR          (366)
#define DAYS_PER_YEAR               (365)
#define DAYS_PER_WEEK               (7)
#define LEAP_FOUR_CENTURY           (400)
#define LEAP_CENTURY                (100)
#define LEAP_YEAR                   (4)

#define SECONDS_PER_GREGORIAN_YEAR  (31556952)
#define SECONDS_PER_YEAR            (31536000)
#define SECONDS_PER_LEAP_YEAR       (31622400)
#define SECONDS_PER_DAY             (86400)
#define SECONDS_PER_HOUR            (3600)
#define SECONDS_PER_MINUTE          (60)
#define MINUTES_PER_HOUR            (60)
#define HOURS_PER_DAY               (24)
#define MAX_WEEKS_PER_MONTH         (6)
#define MONTHS_PER_YEAR             (12)

#define STARTING_TIME (101 * SECONDS_PER_GREGORIAN_YEAR)


#define SNTP_MAX_TIME_SINCE_UPDATE 30*60

typedef enum {
    SUNDAY = 0, MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY
} DAYS_NAMES_TO_NUMBER;

typedef enum {
    JANUARY = 0,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER
} MONTH_NAMES_TO_NUMBER;

typedef struct {
    time_t         time;
    unsigned short millitm;
    short          timezone;
    short          dstflag;
} timeb;

typedef struct {
    
    unsigned long year;         
    unsigned long month;        
    unsigned long day;          

    
    unsigned long hour;         
    unsigned long minute;       
    unsigned long seconds;      

    
    unsigned long day_of_year;  
    unsigned long day_of_january_first; 
    unsigned long day_of_week;  
    unsigned long week_of_year; 

    
    unsigned long leap_years;   
    unsigned long leap_flag;    

    
    long time_zone;                                   
    unsigned long daylight_saving_offset;             
                                                      
                                                      
                                                      
    unsigned long daylight_saving_start_month;        
    unsigned long daylight_saving_start_day;          
    unsigned long daylight_saving_start_day_of_week;  
    unsigned long daylight_saving_start_week_of_month;
    unsigned long daylight_saving_start_time;         
    unsigned long daylight_saving_stop_month;         
    unsigned long daylight_saving_stop_day;           
    unsigned long daylight_saving_stop_day_of_week;   
    unsigned long daylight_saving_stop_week_of_month; 
    unsigned long daylight_saving_stop_time;          
    unsigned long daylight_saving_year_calc;          
    unsigned long daylight_saving_auto_adjust;        

    
    unsigned long is_daylight_saving;           
    unsigned long daylight_saving_start_second; 
    unsigned long daylight_saving_stop_second;  
} time2_s;

typedef struct {
    const char *TZ_NAME;
    const long offset;
} TZ_STRUCT;


extern const char *const   MONTH_NAMES_LONG[];
extern const char *const   MONTH_NAMES_SHORT[];
extern const unsigned long DAYS_IN_MONTH[];
extern const char *const   DAY_NAMES_LONG[];
extern const char *const   DAY_NAMES_SHORT[];
extern const char *const   DAY_NAMES_ABBREV[];
extern const TZ_STRUCT     TZ_TABLE[];

long make_date(unsigned long secondsPastEpoch, time2_s *timeStruct);
const char *get_month_name_long(unsigned long month);
const char *get_month_name_short(unsigned long month);
const char *get_day_name_long(unsigned long day);
const char *get_day_name_short(unsigned long day);
const char *get_day_name_abrrev(unsigned long day);
long get_leap_years(unsigned long year);
unsigned long get_seconds_past_epoch(unsigned long nth, unsigned long day_name, unsigned long month, unsigned long year);
long is_leap_year(unsigned long year);
long range_check_time_struct(time2_s *ts);
unsigned long date_to_seconds(const time2_s *ts);
unsigned long time_to_seconds(const time2_s *ts);
unsigned long date_time_to_seconds(const time2_s *ts);
unsigned long get_local_time(void);
void get_precise_local_time(unsigned long *local_time);
long get_local_timezone(void);
long get_local_dst_active(void);
unsigned long time_to_short_string(const time2_s *ts, char *time_string);
unsigned long gmt_string_to_seconds(char *gmt_string, unsigned long *seconds);
unsigned long seconds_to_gmt_string(unsigned long seconds, char *gmt_string);
unsigned long diff_time(unsigned long t1, unsigned long t2);






long cmp_time(unsigned long t1, unsigned long t2);

long diff_current_time(unsigned long t1, unsigned long *difference);
long parse_month(char *pString);
long parse_day_of_week(char *pString);
long parse_timezone(char *pString);
long parse_time(char *pString);






void ascTimeDuration(unsigned long ulMilliseconds, char *ptimStr, int length);
void ascFormatDate(time2_s *ts, char * pDateStr, int length);
void ascFormatHrMinTime(time2_s *ts, char * pTimeStr, int length, char bFlashColon);
void FormatDateTime(char * ptimeStr, int length);
void FormatDebugTime(char *timeStr, int length,long ts);
char *GetDateTimeString(void);






int TimeOfDayUpdate(void);


void SetTimeOfDay(time2_s *time);




void ClearTimeDisplay(void);

#endif
