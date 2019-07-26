





#include "udbgutil.h"
#include <string.h>
#include "ustr_imp.h"
#include "cstring.h"
#include "putilimp.h"
#include "unicode/ulocdata.h"
#include "unicode/ucnv.h"








































struct Field {
    int32_t prefix;   
	const char *str;  
	int32_t num;      
};




#define DBG_ARRAY_COUNT(x) (sizeof(x)/sizeof(x[0]))







#define FIELD_NAME_STR(y,x)  { y, #x, x }



#if !UCONFIG_NO_FORMATTING


#include "unicode/ucal.h"


#define LEN_UCAL 5 /* UCAL_ */
static const int32_t count_UCalendarDateFields = UCAL_FIELD_COUNT;
static const Field names_UCalendarDateFields[] = 
{
    FIELD_NAME_STR( LEN_UCAL, UCAL_ERA ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MONTH ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_WEEK_OF_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_WEEK_OF_MONTH ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DATE ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DAY_OF_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DAY_OF_WEEK ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DAY_OF_WEEK_IN_MONTH ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_AM_PM ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_HOUR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_HOUR_OF_DAY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MINUTE ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_SECOND ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MILLISECOND ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_ZONE_OFFSET ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DST_OFFSET ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_YEAR_WOY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_DOW_LOCAL ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_EXTENDED_YEAR ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_JULIAN_DAY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_MILLISECONDS_IN_DAY ),
    FIELD_NAME_STR( LEN_UCAL, UCAL_IS_LEAP_MONTH ),
};


static const int32_t count_UCalendarMonths = UCAL_UNDECIMBER+1;
static const Field names_UCalendarMonths[] = 
{
  FIELD_NAME_STR( LEN_UCAL, UCAL_JANUARY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_FEBRUARY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_MARCH ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_APRIL ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_MAY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_JUNE ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_JULY ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_AUGUST ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_SEPTEMBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_OCTOBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_NOVEMBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_DECEMBER ),
  FIELD_NAME_STR( LEN_UCAL, UCAL_UNDECIMBER)
};

#include "unicode/udat.h"

#define LEN_UDAT 5 /* "UDAT_" */
static const int32_t count_UDateFormatStyle = UDAT_SHORT+1;
static const Field names_UDateFormatStyle[] = 
{
        FIELD_NAME_STR( LEN_UDAT, UDAT_FULL ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_LONG ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_MEDIUM ),
        FIELD_NAME_STR( LEN_UDAT, UDAT_SHORT ),
        
    




};

#endif
 
#include "unicode/uloc.h"

#define LEN_UAR 12 /* "ULOC_ACCEPT_" */
static const int32_t count_UAcceptResult = 3;
static const Field names_UAcceptResult[] = 
{
        FIELD_NAME_STR( LEN_UAR, ULOC_ACCEPT_FAILED ),
        FIELD_NAME_STR( LEN_UAR, ULOC_ACCEPT_VALID ),
        FIELD_NAME_STR( LEN_UAR, ULOC_ACCEPT_FALLBACK ),
};

#if !UCONFIG_NO_COLLATION
#include "unicode/ucol.h"
#define LEN_UCOL 5 /* UCOL_ */
static const int32_t count_UColAttributeValue = UCOL_ATTRIBUTE_VALUE_COUNT;
static const Field names_UColAttributeValue[]  = {
   FIELD_NAME_STR( LEN_UCOL, UCOL_PRIMARY ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_SECONDARY ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_TERTIARY ),

   FIELD_NAME_STR( LEN_UCOL, UCOL_QUATERNARY ),
   
   FIELD_NAME_STR( LEN_UCOL, UCOL_IDENTICAL ),

   FIELD_NAME_STR( LEN_UCOL, UCOL_OFF ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_ON ),
   
   FIELD_NAME_STR( LEN_UCOL, UCOL_SHIFTED ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_NON_IGNORABLE ),
   
   FIELD_NAME_STR( LEN_UCOL, UCOL_LOWER_FIRST ),
   FIELD_NAME_STR( LEN_UCOL, UCOL_UPPER_FIRST ),
};

#endif


#include "unicode/icuplug.h"

#define LEN_UPLUG_REASON 13 /* UPLUG_REASON_ */
static const int32_t count_UPlugReason = UPLUG_REASON_COUNT;
static const Field names_UPlugReason[]  = {
   FIELD_NAME_STR( LEN_UPLUG_REASON, UPLUG_REASON_QUERY ),
   FIELD_NAME_STR( LEN_UPLUG_REASON, UPLUG_REASON_LOAD ),
   FIELD_NAME_STR( LEN_UPLUG_REASON, UPLUG_REASON_UNLOAD ),
};

#define LEN_UPLUG_LEVEL 12 /* UPLUG_LEVEL_ */
static const int32_t count_UPlugLevel = UPLUG_LEVEL_COUNT;
static const Field names_UPlugLevel[]  = {
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_INVALID ),
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_UNKNOWN ),
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_LOW ),
   FIELD_NAME_STR( LEN_UPLUG_LEVEL, UPLUG_LEVEL_HIGH ),
};

#define LEN_UDBG 5 /* "UDBG_" */
static const int32_t count_UDebugEnumType = UDBG_ENUM_COUNT;
static const Field names_UDebugEnumType[] = 
{
    FIELD_NAME_STR( LEN_UDBG, UDBG_UDebugEnumType ),
#if !UCONFIG_NO_FORMATTING
    FIELD_NAME_STR( LEN_UDBG, UDBG_UCalendarDateFields ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UCalendarMonths ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UDateFormatStyle ),
#endif
    FIELD_NAME_STR( LEN_UDBG, UDBG_UPlugReason ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UPlugLevel ),
    FIELD_NAME_STR( LEN_UDBG, UDBG_UAcceptResult ),
#if !UCONFIG_NO_COLLATION
    FIELD_NAME_STR( LEN_UDBG, UDBG_UColAttributeValue ),
#endif
};




#define COUNT_CASE(x)  case UDBG_##x: return (actual?count_##x:DBG_ARRAY_COUNT(names_##x));
#define COUNT_FAIL_CASE(x) case UDBG_##x: return -1;

#define FIELD_CASE(x)  case UDBG_##x: return names_##x;
#define FIELD_FAIL_CASE(x) case UDBG_##x: return NULL;







static int32_t _udbg_enumCount(UDebugEnumType type, UBool actual) {
	switch(type) {
		COUNT_CASE(UDebugEnumType)
#if !UCONFIG_NO_FORMATTING
		COUNT_CASE(UCalendarDateFields)
		COUNT_CASE(UCalendarMonths)
		COUNT_CASE(UDateFormatStyle)
#endif
        COUNT_CASE(UPlugReason)
        COUNT_CASE(UPlugLevel)
        COUNT_CASE(UAcceptResult)
#if !UCONFIG_NO_COLLATION
        COUNT_CASE(UColAttributeValue)
#endif
		
	default:
		return -1;
	}
}

static const Field* _udbg_enumFields(UDebugEnumType type) {
	switch(type) {
		FIELD_CASE(UDebugEnumType)
#if !UCONFIG_NO_FORMATTING
		FIELD_CASE(UCalendarDateFields)
		FIELD_CASE(UCalendarMonths)
		FIELD_CASE(UDateFormatStyle)
#endif
        FIELD_CASE(UPlugReason)
        FIELD_CASE(UPlugLevel)
        FIELD_CASE(UAcceptResult)
		
#if !UCONFIG_NO_COLLATION
        FIELD_CASE(UColAttributeValue)
#endif
	default:
		return NULL;
	}
}



int32_t  udbg_enumCount(UDebugEnumType type) {
	return _udbg_enumCount(type, FALSE);
}

int32_t  udbg_enumExpectedCount(UDebugEnumType type) {
	return _udbg_enumCount(type, TRUE);
}

const char *  udbg_enumName(UDebugEnumType type, int32_t field) {
	if(field<0 || 
				field>=_udbg_enumCount(type,FALSE)) { 
		return NULL;
	} else {
		const Field *fields = _udbg_enumFields(type);
		if(fields == NULL) {
			return NULL;
		} else {
			return fields[field].str + fields[field].prefix;
		}
	}
}

int32_t  udbg_enumArrayValue(UDebugEnumType type, int32_t field) {
	if(field<0 || 
				field>=_udbg_enumCount(type,FALSE)) { 
		return -1;
	} else {
		const Field *fields = _udbg_enumFields(type);
		if(fields == NULL) {
			return -1;
		} else {
			return fields[field].num;
		}
	}    
}

int32_t udbg_enumByName(UDebugEnumType type, const char *value) {
    if(type<0||type>=_udbg_enumCount(UDBG_UDebugEnumType, TRUE)) {
        return -1; 
    }
	const Field *fields = _udbg_enumFields(type);
    for(int32_t field = 0;field<_udbg_enumCount(type, FALSE);field++) {
        if(!strcmp(value, fields[field].str + fields[field].prefix)) {
            return fields[field].num;
        }        
    }
    
    for(int32_t field = 0;field<_udbg_enumCount(type, FALSE);field++) {
        if(!strcmp(value, fields[field].str)) {
            return fields[field].num;
        }        
    }
    
    return -1;
}





U_CAPI const char *udbg_getPlatform(void)
{
#if U_PLATFORM_HAS_WIN32_API
    return "Windows";
#elif U_PLATFORM == U_PF_UNKNOWN
    return "unknown";
#elif U_PLATFORM == U_PF_DARWIN
    return "Darwin";
#elif U_PLATFORM == U_PF_QNX
    return "QNX";
#elif U_PLATFORM == U_PF_LINUX
    return "Linux";
#elif U_PLATFORM == U_PF_ANDROID
    return "Android";
#elif U_PLATFORM == U_PF_CLASSIC_MACOS
    return "MacOS (Classic)";
#elif U_PLATFORM == U_PF_OS390
    return "IBM z";
#elif U_PLATFORM == U_PF_OS400
    return "IBM i";
#else
    return "Other (POSIX-like)";
#endif
}

struct USystemParams;

typedef int32_t U_CALLCONV USystemParameterCallback(const USystemParams *param, char *target, int32_t targetCapacity, UErrorCode *status);

struct USystemParams {
  const char *paramName;
  USystemParameterCallback *paramFunction;
  const char *paramStr;
  int32_t paramInt;
};


U_CAPI  int32_t
paramEmpty(const USystemParams * , char *target, int32_t targetCapacity, UErrorCode *status) {
  if(U_FAILURE(*status))return 0;
  return u_terminateChars(target, targetCapacity, 0, status);
}

U_CAPI  int32_t
paramStatic(const USystemParams *param, char *target, int32_t targetCapacity, UErrorCode *status) {
  if(param->paramStr==NULL) return paramEmpty(param,target,targetCapacity,status);
  if(U_FAILURE(*status))return 0;
  int32_t len = uprv_strlen(param->paramStr);
  if(target!=NULL) {
    uprv_strncpy(target,param->paramStr,uprv_min(len,targetCapacity));
  }
  return u_terminateChars(target, targetCapacity, len, status);
}

static const char *nullString = "(null)";

static int32_t stringToStringBuffer(char *target, int32_t targetCapacity, const char *str, UErrorCode *status) {
  if(str==NULL) str=nullString; 

  int32_t len = uprv_strlen(str);
  if (U_SUCCESS(*status)) {
    if(target!=NULL) {
      uprv_strncpy(target,str,uprv_min(len,targetCapacity));
    }
  } else {
    const char *s = u_errorName(*status);
    len = uprv_strlen(s);
    if(target!=NULL) {
      uprv_strncpy(target,s,uprv_min(len,targetCapacity));
    }
  }
  return u_terminateChars(target, targetCapacity, len, status);
}

static int32_t integerToStringBuffer(char *target, int32_t targetCapacity, int32_t n, int32_t radix, UErrorCode *status) {
  if(U_FAILURE(*status)) return 0;
  char str[300];
  T_CString_integerToString(str,n,radix);
  return stringToStringBuffer(target,targetCapacity,str,status);
}

U_CAPI  int32_t
paramInteger(const USystemParams *param, char *target, int32_t targetCapacity, UErrorCode *status) {
  if(U_FAILURE(*status))return 0;
  if(param->paramStr==NULL || param->paramStr[0]=='d') {
    return integerToStringBuffer(target,targetCapacity,param->paramInt, 10,status);
  } else if(param->paramStr[0]=='x') {
    return integerToStringBuffer(target,targetCapacity,param->paramInt, 16,status);
  } else if(param->paramStr[0]=='o') {
    return integerToStringBuffer(target,targetCapacity,param->paramInt, 8,status);
  } else if(param->paramStr[0]=='b') {
    return integerToStringBuffer(target,targetCapacity,param->paramInt, 2,status);
  } else {
    *status = U_INTERNAL_PROGRAM_ERROR;
    return 0;
  }
}


U_CAPI  int32_t
paramCldrVersion(const USystemParams * , char *target, int32_t targetCapacity, UErrorCode *status) {
  if(U_FAILURE(*status))return 0;
  char str[200]="";
  UVersionInfo icu;

  ulocdata_getCLDRVersion(icu, status);
  if(U_SUCCESS(*status)) {
    u_versionToString(icu, str);
    return stringToStringBuffer(target,targetCapacity,str,status);
  } else {
    return 0;
  }
}


#if !UCONFIG_NO_FORMATTING
U_CAPI  int32_t
paramTimezoneDefault(const USystemParams * , char *target, int32_t targetCapacity, UErrorCode *status) {
  if(U_FAILURE(*status))return 0;
  UChar buf[100];
  char buf2[100];
  int32_t len;
  
  len = ucal_getDefaultTimeZone(buf, 100, status);
  if(U_SUCCESS(*status)&&len>0) {
    u_UCharsToChars(buf, buf2, len+1);
    return stringToStringBuffer(target,targetCapacity, buf2,status);
  } else {
    return 0;
  }
}
#endif

U_CAPI  int32_t
paramLocaleDefaultBcp47(const USystemParams * , char *target, int32_t targetCapacity, UErrorCode *status) {
  if(U_FAILURE(*status))return 0;
  const char *def = uloc_getDefault();
  return uloc_toLanguageTag(def,target,targetCapacity,FALSE,status);
}



#define STRING_PARAM(func, str) U_CAPI  int32_t \
  func(const USystemParams *, char *target, int32_t targetCapacity, UErrorCode *status) \
  {  return stringToStringBuffer(target,targetCapacity,(str),status); }

STRING_PARAM(paramIcudataPath, u_getDataDirectory())
STRING_PARAM(paramPlatform, udbg_getPlatform())
STRING_PARAM(paramLocaleDefault, uloc_getDefault())
#if !UCONFIG_NO_CONVERSION
STRING_PARAM(paramConverterDefault, ucnv_getDefaultName())
#endif

#if !UCONFIG_NO_FORMATTING
STRING_PARAM(paramTimezoneVersion, ucal_getTZDataVersion(status))
#endif

static const USystemParams systemParams[] = {
  { "copyright",    paramStatic, U_COPYRIGHT_STRING,0 },
  { "product",      paramStatic, "icu4c",0 },
  { "product.full", paramStatic, "International Components for Unicode for C/C++",0 },
  { "version",      paramStatic, U_ICU_VERSION,0 },
  { "version.unicode", paramStatic, U_UNICODE_VERSION,0 },
  { "platform.number", paramInteger, "d",U_PLATFORM},
  { "platform.type", paramPlatform, NULL ,0},
  { "locale.default", paramLocaleDefault, NULL, 0},
  { "locale.default.bcp47", paramLocaleDefaultBcp47, NULL, 0},
#if !UCONFIG_NO_CONVERSION
  { "converter.default", paramConverterDefault, NULL, 0},
#endif
  { "icudata.name", paramStatic, U_ICUDATA_NAME, 0},
  { "icudata.path", paramIcudataPath, NULL, 0},

  { "cldr.version", paramCldrVersion, NULL, 0},
  
#if !UCONFIG_NO_FORMATTING
  { "tz.version", paramTimezoneVersion, NULL, 0},
  { "tz.default", paramTimezoneDefault, NULL, 0},
#endif
  
  { "cpu.bits",       paramInteger, "d", (sizeof(void*))*8},
  { "cpu.big_endian", paramInteger, "b", U_IS_BIG_ENDIAN},
  { "os.wchar_width", paramInteger, "d", U_SIZEOF_WCHAR_T},
  { "os.charset_family", paramInteger, "d", U_CHARSET_FAMILY},
#if defined (U_HOST)
  { "os.host", paramStatic, U_HOST, 0},
#endif
#if defined (U_BUILD)
  { "build.build", paramStatic, U_BUILD, 0},
#endif
#if defined (U_CC)
  { "build.cc", paramStatic, U_CC, 0},
#endif
#if defined (U_CXX)
  { "build.cxx", paramStatic, U_CXX, 0},
#endif
#if defined (CYGWINMSVC)
  { "build.cygwinmsvc", paramInteger, "b", 1},
#endif
  { "uconfig.internal_digitlist", paramInteger, "b", 1}, 
  { "uconfig.have_parseallinput", paramInteger, "b", UCONFIG_HAVE_PARSEALLINPUT},
  { "uconfig.format_fastpaths_49",paramInteger, "b", UCONFIG_FORMAT_FASTPATHS_49},


};

#define U_SYSPARAM_COUNT (sizeof(systemParams)/sizeof(systemParams[0]))

U_CAPI const char *udbg_getSystemParameterNameByIndex(int32_t i) {
  if(i>=0 && i < (int32_t)U_SYSPARAM_COUNT) {
    return systemParams[i].paramName;
  } else {
    return NULL;
  }
}


U_CAPI int32_t udbg_getSystemParameterValueByIndex(int32_t i, char *buffer, int32_t bufferCapacity, UErrorCode *status) {
  if(i>=0 && i< (int32_t)U_SYSPARAM_COUNT) {
    return systemParams[i].paramFunction(&(systemParams[i]),buffer,bufferCapacity,status);
  } else {
    return 0;
  }
}

U_CAPI void udbg_writeIcuInfo(FILE *out) {
  char str[2000];
  
  fprintf(out, " <icuSystemParams type=\"icu4c\">\n");
  const char *paramName;
  for(int32_t i=0;(paramName=udbg_getSystemParameterNameByIndex(i))!=NULL;i++) {
    UErrorCode status2 = U_ZERO_ERROR;
    udbg_getSystemParameterValueByIndex(i, str,2000,&status2);
    if(U_SUCCESS(status2)) {
      fprintf(out,"    <param name=\"%s\">%s</param>\n", paramName,str);
    } else {
      fprintf(out,"  <!-- n=\"%s\" ERROR: %s -->\n", paramName, u_errorName(status2));
    }
  }
  fprintf(out, " </icuSystemParams>\n");
}
