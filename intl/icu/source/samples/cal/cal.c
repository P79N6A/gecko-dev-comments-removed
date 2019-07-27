














#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unicode/uloc.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/ustring.h"
#include "unicode/uclean.h"

#include "uprint.h"

#if UCONFIG_NO_FORMATTING

int main(int argc, char **argv)
{
  printf("%s: Sorry, UCONFIG_NO_FORMATTING was turned on (see uconfig.h). No formatting can be done. \n", argv[0]);
  return 0;
}
#else



static void usage(void);

static void version(void);

static void cal(int32_t month, int32_t year,
                UBool useLongNames, UErrorCode *status);

static void get_symbols(const UDateFormat *fmt,
                        UDateFormatSymbolType type,
                        UChar *array[],
                        int32_t arrayLength,
                        int32_t lowestIndex,
                        int32_t firstIndex,
                        UErrorCode *status);

static void free_symbols(UChar *array[],
                         int32_t arrayLength);

static void get_days(const UDateFormat *fmt,
                     UChar *days [], UBool useLongNames, 
                     int32_t fdow, UErrorCode *status);

static void free_days(UChar *days[]);

static void get_months(const UDateFormat *fmt,
                       UChar *months [], UBool useLongNames,
                       UErrorCode *status);

static void free_months(UChar *months[]);

static void indent(int32_t count, FILE *f);

static void print_days(UChar *days [], FILE *f, UErrorCode *status);

static void  print_month(UCalendar *c, 
                         UChar *days [], 
                         UBool useLongNames, int32_t fdow, 
                         UErrorCode *status);

static void  print_year(UCalendar *c, 
                        UChar *days [], UChar *months [],
                        UBool useLongNames, int32_t fdow, 
                        UErrorCode *status);


#define CAL_VERSION "1.0"


#define DAY_COUNT 7


#define MONTH_COUNT 13


#define MARGIN_WIDTH 4


#define BUF_SIZE 64


static const UChar sShortPat [] = { 0x004D, 0x004D, 0x004D, 0x0020, 
0x0079, 0x0079, 0x0079, 0x0079 };

static const UChar sLongPat [] = { 0x004D, 0x004D, 0x004D, 0x004D, 0x0020, 
0x0079, 0x0079, 0x0079, 0x0079 };


int
main(int argc,
     char **argv)
{
    int printUsage = 0;
    int printVersion = 0;
    UBool useLongNames = 0;
    int optInd = 1;
    char *arg;
    int32_t month = -1, year = -1;
    UErrorCode status = U_ZERO_ERROR;
    
    
    
    for(optInd = 1; optInd < argc; ++optInd) {
        arg = argv[optInd];
        
        
        if(strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
            printVersion = 1;
        }
        
        else if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
            printUsage = 1;
        }
        
        else if(strcmp(arg, "-l") == 0 || strcmp(arg, "--long") == 0) {
            useLongNames = 1;
        }
        
        else if(strcmp(arg, "--") == 0) {
            
            ++optInd;
            break;
        }
        
        else if(strncmp(arg, "-", strlen("-")) == 0) {
            printf("cal: invalid option -- %s\n", arg+1);
            printUsage = 1;
        }
        
        else {
            break;
        }
    }
    
    
    if(optInd != argc) {
        
        
        if(argc - optInd == 2) {
            sscanf(argv[optInd], "%d", (int*)&month);
            sscanf(argv[optInd + 1], "%d", (int*)&year);
            
            
            if(month < 0 || month > 12) {
                printf("icucal: Bad value for month -- %d\n", (int)month);
                
                
                printUsage = 1;
            }
            
            
            --month;
        }
        
        else {
            sscanf(argv[optInd], "%d", (int*)&year);
        }
    }
    
    
    if(printUsage) {
        usage();
        return 0;
    }
    
    
    if(printVersion) {
        version();
        return 0;
    }
    
    
    cal(month, year, useLongNames, &status);
    
    
    u_cleanup();

    return (U_FAILURE(status) ? 1 : 0);
}


static void
usage()
{  
    puts("Usage: icucal [OPTIONS] [[MONTH] YEAR]");
    puts("");
    puts("Options:");
    puts("  -h, --help        Print this message and exit.");
    puts("  -v, --version     Print the version number of cal and exit.");
    puts("  -l, --long        Use long names.");
    puts("");
    puts("Arguments (optional):");
    puts("  MONTH             An integer (1-12) indicating the month to display");
    puts("  YEAR              An integer indicating the year to display");
    puts("");
    puts("For an interesting calendar, look at October 1582");
}


static void
version()
{
    printf("icucal version %s (ICU version %s), created by Stephen F. Booth.\n", 
        CAL_VERSION, U_ICU_VERSION); 
    puts(U_COPYRIGHT_STRING);
}

static void
cal(int32_t month,
    int32_t year,
    UBool useLongNames,
    UErrorCode *status)
{
    UCalendar *c;
    UChar *days [DAY_COUNT];
    UChar *months [MONTH_COUNT];
    int32_t fdow;
    
    if(U_FAILURE(*status)) return;
    
    
    c = ucal_open(0, -1, uloc_getDefault(), UCAL_TRADITIONAL, status);
    
    
    
    
    if(month == -1 && year != -1) {
        
        
        ucal_set(c, UCAL_YEAR, year);
        
        
        fdow = ucal_getAttribute(c, UCAL_FIRST_DAY_OF_WEEK);
        
        
        print_year(c, days, months, useLongNames, fdow, status);
    }
    
    
    else {
        
        
        if(month != -1)
            ucal_set(c, UCAL_MONTH, month);
        if(year != -1)
            ucal_set(c, UCAL_YEAR, year);
        
        
        fdow = ucal_getAttribute(c, UCAL_FIRST_DAY_OF_WEEK);
        
        
        print_month(c, days, useLongNames, fdow, status);
    }
    
    
    ucal_close(c);
}











static void get_symbols(const UDateFormat *fmt,
                        UDateFormatSymbolType type,
                        UChar *array[],
                        int32_t arrayLength,
                        int32_t lowestIndex,
                        int32_t firstIndex,
                        UErrorCode *status)
{
    int32_t count, i;
    
    if (U_FAILURE(*status)) {
        return;
    }

    count = udat_countSymbols(fmt, type);

    if(count != arrayLength + lowestIndex) {
        return;
    }

    for(i = 0; i < arrayLength; i++) {
        int32_t idx = (i + firstIndex) % arrayLength;
        int32_t size = 1 + udat_getSymbols(fmt, type, idx + lowestIndex, NULL, 0, status);
        
        array[idx] = (UChar *) malloc(sizeof(UChar) * size);

        *status = U_ZERO_ERROR;
        udat_getSymbols(fmt, type, idx + lowestIndex, array[idx], size, status);
    }
}


static void free_symbols(UChar *array[],
                         int32_t arrayLength)
{
    int32_t i;

    for(i = 0; i < arrayLength; i++) {
        free(array[i]);
    }
}





static void
get_days(const UDateFormat *fmt,
         UChar *days [],
         UBool useLongNames,
         int32_t fdow,
         UErrorCode *status)
{
    UDateFormatSymbolType dayType = (useLongNames ? UDAT_WEEKDAYS : UDAT_SHORT_WEEKDAYS);
    
    if(U_FAILURE(*status))
        return;
    
    
    --fdow;

    get_symbols(fmt, dayType, days, DAY_COUNT, 1, fdow, status);
}

static void free_days(UChar *days[])
{
    free_symbols(days, DAY_COUNT);
}



static void
get_months(const UDateFormat *fmt,
           UChar *months [],
           UBool useLongNames,
           UErrorCode *status)
{
    UDateFormatSymbolType monthType = (useLongNames ? UDAT_MONTHS : UDAT_SHORT_MONTHS);
    
    if(U_FAILURE(*status))
        return;
    
    get_symbols(fmt, monthType, months, MONTH_COUNT - 1, 0, 0, status); 
}

static void free_months(UChar *months[])
{
    free_symbols(months, MONTH_COUNT - 1);
}


static void
indent(int32_t count,
       FILE *f)
{
    char c [BUF_SIZE];

    if(count <= 0)
    {
        return;
    }
    
    if(count < BUF_SIZE) {
        memset(c, (int)' ', count);
        fwrite(c, sizeof(char), count, f);
    }
    else {
        int32_t i;
        for(i = 0; i < count; ++i)
            putc(' ', f);
    }
}


static void
print_days(UChar *days [],
           FILE *f,
           UErrorCode *status)
{
    int32_t i;
    
    if(U_FAILURE(*status)) return;
    
    
    for(i = 0; i < DAY_COUNT; ++i) {
        uprint(days[i], f, status);
        putc(' ', f);
    }
}


static void
print_month(UCalendar *c, 
            UChar *days [], 
            UBool useLongNames,
            int32_t fdow,
            UErrorCode *status)
{
    int32_t width, pad, i, day;
    int32_t lens [DAY_COUNT];
    int32_t firstday, current;
    UNumberFormat *nfmt;
    UDateFormat *dfmt;
    UChar s [BUF_SIZE];
    const UChar *pat = (useLongNames ? sLongPat : sShortPat);
    int32_t len = (useLongNames ? 9 : 8);
    
    if(U_FAILURE(*status)) return;
    
    
    
    
    
    dfmt = udat_open(UDAT_PATTERN,UDAT_PATTERN,NULL,NULL,0,pat, len,status);
    
    
    udat_format(dfmt, ucal_getMillis(c, status), s, BUF_SIZE, 0, status);
    
    
    
    get_days(dfmt, days, useLongNames, fdow, status);

    
    
    
    width = 6; 
    for(i = 0; i < DAY_COUNT; ++i) {
        lens[i] = u_strlen(days[i]);
        width += lens[i];
    }
    
    
    pad = width - u_strlen(s);
    indent(pad / 2, stdout);
    uprint(s, stdout, status);
    putc('\n', stdout);
    
    
    
    
    print_days(days, stdout, status);
    putc('\n', stdout);
    
    
    
    
    
    ucal_set(c, UCAL_DATE, 1);
    firstday = ucal_get(c, UCAL_DAY_OF_WEEK, status);
    
    



    firstday -= fdow;
    
    
    nfmt = unum_open(UNUM_DECIMAL, NULL,0,NULL,NULL, status);
    
    
    current = firstday;
    if(current < 0)
    {
       current += 7;
    }
    for(i = 0; i < current; ++i)
        indent(lens[i] + 1, stdout);
    
    
    day = ucal_get(c, UCAL_DATE, status);
    do {
        
        
        unum_format(nfmt, day, s, BUF_SIZE, 0, status);
        
        
        pad = lens[current] - u_strlen(s);
        indent(pad, stdout);
        
        
        uprint(s, stdout, status);
        putc(' ', stdout);
        
        
        ++current;
        current %= DAY_COUNT;
        
        
        if(current == 0) {
            putc('\n', stdout);
        }
        
        
        ucal_add(c, UCAL_DATE, 1, status);
        day = ucal_get(c, UCAL_DATE, status);
        
    } while(day != 1);
    
    
    putc('\n', stdout);
    
    
    free_days(days);
    unum_close(nfmt);
    udat_close(dfmt);
}


static void
print_year(UCalendar *c, 
           UChar *days [], 
           UChar *months [],
           UBool useLongNames, 
           int32_t fdow, 
           UErrorCode *status)
{
    int32_t width, pad, i, j;
    int32_t lens [DAY_COUNT];
    UNumberFormat *nfmt;
    UDateFormat *dfmt;
    UChar s [BUF_SIZE];
    const UChar pat [] = { 0x0079, 0x0079, 0x0079, 0x0079 };
    int32_t len = 4;
    UCalendar  *left_cal, *right_cal;
    int32_t left_day, right_day;
    int32_t left_firstday, right_firstday, left_current, right_current;
    int32_t left_month, right_month;
    
    if(U_FAILURE(*status)) return;
    
    
    left_cal = c;
    
    
    
    
    dfmt = udat_open(UDAT_PATTERN,UDAT_PATTERN,NULL,NULL,0,pat, len, status);
    
    
    udat_format(dfmt, ucal_getMillis(left_cal, status), s, BUF_SIZE, 0, status);
    
    
    get_days(dfmt, days, useLongNames, fdow, status);
    get_months(dfmt, months, useLongNames, status);

    
    
    
    width = 6; 
    for(i = 0; i < DAY_COUNT; ++i) {
        lens[i] = u_strlen(days[i]);
        width += lens[i];
    }
    
    

    
    
    pad = 2 * width + MARGIN_WIDTH - u_strlen(s);
    indent(pad / 2, stdout);
    uprint(s, stdout, status);
    putc('\n', stdout);
    putc('\n', stdout);
    
    
    right_cal = ucal_open(0, -1, uloc_getDefault(), UCAL_TRADITIONAL, status);
    ucal_setMillis(right_cal, ucal_getMillis(left_cal, status), status);
    
    
    nfmt = unum_open(UNUM_DECIMAL,NULL, 0,NULL,NULL, status);
    
    
    for(i = 0; i < MONTH_COUNT - 1; i += 2) {
        
        
        pad = width - u_strlen(months[i]);
        indent(pad / 2, stdout);
        uprint(months[i], stdout, status);
        indent(pad / 2 + MARGIN_WIDTH, stdout);
        pad = width - u_strlen(months[i + 1]);
        indent(pad / 2, stdout);
        uprint(months[i + 1], stdout, status);
        putc('\n', stdout);
        
        
        print_days(days, stdout, status);
        indent(MARGIN_WIDTH, stdout);
        print_days(days, stdout, status);
        putc('\n', stdout);
        
        
        ucal_set(left_cal, UCAL_MONTH, i);
        ucal_set(left_cal, UCAL_DATE, 1);
        ucal_set(right_cal, UCAL_MONTH, i + 1);
        ucal_set(right_cal, UCAL_DATE, 1);
        
        left_firstday = ucal_get(left_cal, UCAL_DAY_OF_WEEK, status);
        right_firstday = ucal_get(right_cal, UCAL_DAY_OF_WEEK, status);
        
        




        
        


        left_firstday += (DAY_COUNT - fdow);
        left_firstday %= DAY_COUNT;
        
        right_firstday += (DAY_COUNT - fdow);
        right_firstday %= DAY_COUNT;
        
        left_current = left_firstday;
        right_current = right_firstday;
        
        left_day = ucal_get(left_cal, UCAL_DATE, status);
        right_day = ucal_get(right_cal, UCAL_DATE, status);
        
        left_month = ucal_get(left_cal, UCAL_MONTH, status);
        right_month = ucal_get(right_cal, UCAL_MONTH, status);
        
        
        while(left_month == i || right_month == i + 1) {
            
        


            if(left_month != i && right_month == i + 1) {
                indent(width + 1, stdout);
                left_current = 0;
            }
            
            while(left_month == i) {
                
            

                if(left_day == 1) {
                    for(j = 0; j < left_current; ++j)
                        indent(lens[j] + 1, stdout);
                }
                
                
                unum_format(nfmt, left_day, s, BUF_SIZE, 0, status);
                
                
                pad = lens[left_current] - u_strlen(s);
                indent(pad, stdout);
                
                
                uprint(s, stdout, status);
                putc(' ', stdout);
                
                
                ++left_current;
                left_current %= DAY_COUNT;
                
                
                ucal_add(left_cal, UCAL_DATE, 1, status);
                left_day = ucal_get(left_cal, UCAL_DATE, status);
                
                
                left_month = ucal_get(left_cal, UCAL_MONTH, status);
                
                

                if(left_current == 0) {
                    break;
                }
            };
            
            

            if(left_current != 0) {
                for(j = left_current; j < DAY_COUNT; ++j)
                    indent(lens[j] + 1, stdout);
            }
            
            
            indent(MARGIN_WIDTH, stdout);
            
            while(right_month == i + 1) {
                
            

                if(right_day == 1) {
                    for(j = 0; j < right_current; ++j)
                        indent(lens[j] + 1, stdout);
                }
                
                
                unum_format(nfmt, right_day, s, BUF_SIZE, 0, status);
                
                
                pad = lens[right_current] - u_strlen(s);
                indent(pad, stdout);
                
                
                uprint(s, stdout, status);
                putc(' ', stdout);
                
                
                ++right_current;
                right_current %= DAY_COUNT;
                
                
                ucal_add(right_cal, UCAL_DATE, 1, status);
                right_day = ucal_get(right_cal, UCAL_DATE, status);
                
                
                right_month = ucal_get(right_cal, UCAL_MONTH, status);
                
                
                if(right_current == 0) {
                    break;
                }
                
            };
            
            
            putc('\n', stdout);
        }
        
        
        putc('\n', stdout);
  }
  
  
  free_months(months);
  free_days(days);
  udat_close(dfmt);
  unum_close(nfmt);
  ucal_close(right_cal);
}

#endif
