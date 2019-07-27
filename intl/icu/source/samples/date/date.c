
















#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/ustring.h"
#include "unicode/uclean.h"

#include "unicode/ucnv.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"

#include "uprint.h"

int main(int argc, char **argv);

#if UCONFIG_NO_FORMATTING || UCONFIG_NO_CONVERSION

int main(int argc, char **argv)
{
  printf("%s: Sorry, UCONFIG_NO_FORMATTING or UCONFIG_NO_CONVERSION was turned on (see uconfig.h). No formatting can be done. \n", argv[0]);
  return 0;
}
#else



static void usage(void);
static void version(void);
static void date(UDate when, const UChar *tz, UDateFormatStyle style, const char *format, UErrorCode *status);
static UDate getWhen(const char *millis, const char *seconds, const char *format, UDateFormatStyle style, const char *parse, const UChar *tz, UErrorCode *status);

UConverter *cnv = NULL;


#define DATE_VERSION "1.0"


static const UChar GMT_ID [] = { 0x0047, 0x004d, 0x0054, 0x0000 };

#define FORMAT_MILLIS "%"
#define FORMAT_SECONDS "%%"

int
main(int argc,
     char **argv)
{
  int printUsage = 0;
  int printVersion = 0;
  int optInd = 1;
  char *arg;
  const UChar *tz = 0;
  UDateFormatStyle style = UDAT_DEFAULT;
  UErrorCode status = U_ZERO_ERROR;
  const char *format = NULL;
  char *parse = NULL;
  char *seconds = NULL;
  char *millis = NULL;
  UDate when;

  
  for(optInd = 1; optInd < argc; ++optInd) {
    arg = argv[optInd];
    
    
    if(strcmp(arg, "-v") == 0 || strcmp(arg, "--version") == 0) {
      printVersion = 1;
    }
    
    else if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
      printUsage = 1;
    }
    
    else if(strcmp(arg, "-u") == 0 || strcmp(arg, "--gmt") == 0) {
      tz = GMT_ID;
    }
    
    else if(strcmp(arg, "-f") == 0 || strcmp(arg, "--full") == 0) {
      style = UDAT_FULL;
    }
    
    else if(strcmp(arg, "-l") == 0 || strcmp(arg, "--long") == 0) {
      style = UDAT_LONG;
    }
    
    else if(strcmp(arg, "-m") == 0 || strcmp(arg, "--medium") == 0) {
      style = UDAT_MEDIUM;
    }
    
    else if(strcmp(arg, "-s") == 0 || strcmp(arg, "--short") == 0) {
      style = UDAT_SHORT;
    }
    else if(strcmp(arg, "-F") == 0 || strcmp(arg, "--format") == 0) {
      if ( optInd + 1 < argc ) { 
         optInd++;
         format = argv[optInd];
      }
    } else if(strcmp(arg, "-r") == 0) {
      if ( optInd + 1 < argc ) { 
         optInd++;
         seconds = argv[optInd];
      }
    } else if(strcmp(arg, "-R") == 0) {
      if ( optInd + 1 < argc ) { 
         optInd++;
         millis = argv[optInd];
      }
    } else if(strcmp(arg, "-P") == 0) {
      if ( optInd + 1 < argc ) { 
         optInd++;
         parse = argv[optInd];
      }
    }
    
    else if(strcmp(arg, "--") == 0) {
      
      ++optInd;
      break;
    }
    
    else if(strncmp(arg, "-", strlen("-")) == 0) {
      printf("icudate: invalid option -- %s\n", arg+1);
      printUsage = 1;
    }
    
    else {
      break;
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

  
  when = getWhen(millis, seconds, format, style, parse, tz, &status);
  if(parse != NULL) {
    format = FORMAT_MILLIS; 
  }

  
  date(when, tz, style, format, &status);

  ucnv_close(cnv);

  u_cleanup();
  return (U_FAILURE(status) ? 1 : 0);
}


static void
usage()
{  
  puts("Usage: icudate [OPTIONS]");
  puts("Options:");
  puts("  -h, --help        Print this message and exit.");
  puts("  -v, --version     Print the version number of date and exit.");
  puts("  -u, --gmt         Display the date in Greenwich Mean Time.");
  puts("  -f, --full        Use full display format.");
  puts("  -l, --long        Use long display format.");
  puts("  -m, --medium      Use medium display format.");
  puts("  -s, --short       Use short display format.");
  puts("  -F <format>, --format <format>       Use <format> as the display format.");
  puts("                    (Special formats: \"%\" alone is Millis since 1970, \"%%\" alone is Seconds since 1970)");
  puts("  -r <seconds>      Use <seconds> as the time (Epoch 1970) rather than now.");
  puts("  -R <millis>       Use <millis> as the time (Epoch 1970) rather than now.");
  puts("  -P <string>       Parse <string> as the time, output in millis format.");
}


static void
version()
{
  UErrorCode status = U_ZERO_ERROR;
  const char *tzVer;
  int len = 256;
  UChar tzName[256];
  printf("icudate version %s, created by Stephen F. Booth.\n", 
	 DATE_VERSION);
  puts(U_COPYRIGHT_STRING);
  tzVer = ucal_getTZDataVersion(&status);
  if(U_FAILURE(status)) {  
      tzVer = u_errorName(status);  
  }
  printf("\n");
  printf("ICU Version:               %s\n", U_ICU_VERSION);
  printf("ICU Data (major+min):      %s\n", U_ICUDATA_NAME);
  printf("Default Locale:            %s\n", uloc_getDefault());
  printf("Time Zone Data Version:    %s\n", tzVer);
  printf("Default Time Zone:         ");
  status = U_ZERO_ERROR;
  u_init(&status);
  len = ucal_getDefaultTimeZone(tzName, len, &status);
  if(U_FAILURE(status)) {
    fprintf(stderr, " ** Error getting default zone: %s\n", u_errorName(status));
  }
  uprint(tzName, stdout, &status);
  printf("\n\n");
}

static int32_t charsToUCharsDefault(UChar *uchars, int32_t ucharsSize, const char*chars, int32_t charsSize, UErrorCode *status) {
  int32_t len=-1;
  if(U_FAILURE(*status)) return len;
  if(cnv==NULL) {
    cnv = ucnv_open(NULL, status);
  }
  if(cnv&&U_SUCCESS(*status)) {
    len = ucnv_toUChars(cnv, uchars, ucharsSize, chars,charsSize, status);
  }
  return len;
}


static void
date(UDate when,
     const UChar *tz,
     UDateFormatStyle style,
     const char *format,
     UErrorCode *status )
{
  UChar *s = 0;
  int32_t len = 0;
  UDateFormat *fmt;
  UChar uFormat[100];

  if(U_FAILURE(*status)) return;

  if( format != NULL ) {
    if(!strcmp(format,FORMAT_MILLIS)) {
      printf("%.0f\n", when);
      return;
    } else if(!strcmp(format, FORMAT_SECONDS)) {
      printf("%.3f\n", when/1000.0);
      return;
    }
  }

  fmt = udat_open(style, style, 0, tz, -1,NULL,0, status);
  if ( format != NULL ) {
    charsToUCharsDefault(uFormat,sizeof(uFormat)/sizeof(uFormat[0]),format,-1,status);
    udat_applyPattern(fmt,FALSE,uFormat,-1);
  }
  len = udat_format(fmt, when, 0, len, 0, status);
  if(*status == U_BUFFER_OVERFLOW_ERROR) {
    *status = U_ZERO_ERROR;
    s = (UChar*) malloc(sizeof(UChar) * (len+1));
    if(s == 0) goto finish;
    udat_format(fmt, when, s, len + 1, 0, status);
  }
  if(U_FAILURE(*status)) goto finish;

  
  uprint(s, stdout, status);

  
  printf("\n");

 finish:
  if(U_FAILURE(*status)) {
    fprintf(stderr, "Error in Print: %s\n", u_errorName(*status));
  }
  udat_close(fmt);
  free(s);
}

static UDate getWhen(const char *millis, const char *seconds, const char *format, 
                     UDateFormatStyle style, const char *parse, const UChar *tz, UErrorCode *status) {
  UDateFormat *fmt = NULL; 
  UChar uFormat[100];
  UChar uParse[256];
  UDate when=0;
  int32_t parsepos = 0;

  if(millis != NULL) {
    sscanf(millis, "%lf", &when);
    return when;
  } else if(seconds != NULL) {
    sscanf(seconds, "%lf", &when);
    return when*1000.0;
  }

  if(parse!=NULL) {
    if( format != NULL ) {
      if(!strcmp(format,FORMAT_MILLIS)) {
        sscanf(parse, "%lf", &when);
        return when;
      } else if(!strcmp(format, FORMAT_SECONDS)) {
        sscanf(parse, "%lf", &when);
        return when*1000.0;
      }
    }

    fmt = udat_open(style, style, 0, tz, -1,NULL,0, status);
    if ( format != NULL ) {
      charsToUCharsDefault(uFormat,sizeof(uFormat)/sizeof(uFormat[0]), format,-1,status);
      udat_applyPattern(fmt,FALSE,uFormat,-1);
    }
    
    charsToUCharsDefault(uParse,sizeof(uParse)/sizeof(uParse[0]), parse,-1,status);
    when = udat_parse(fmt, uParse, -1, &parsepos, status);
    if(U_FAILURE(*status)) {
      fprintf(stderr, "Error in Parse: %s\n", u_errorName(*status));
      if(parsepos > 0 && parsepos <= (int32_t)strlen(parse)) {
        fprintf(stderr, "ERR>\"%s\" @%d\n"
                        "ERR> %*s^\n",
                parse,parsepos,parsepos,"");
                
      }
    }

    udat_close(fmt);
    return when;
  } else {
    return ucal_getNow();
  }
}

#endif
