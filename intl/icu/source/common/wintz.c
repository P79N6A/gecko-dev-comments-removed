










#include "unicode/utypes.h"

#if U_PLATFORM_HAS_WIN32_API

#include "wintz.h"
#include "cmemory.h"
#include "cstring.h"

#include "unicode/ustring.h"
#include "unicode/ures.h"

#   define WIN32_LEAN_AND_MEAN
#   define VC_EXTRALEAN
#   define NOUSER
#   define NOSERVICE
#   define NOIME
#   define NOMCX
#include <windows.h>

#define MAX_LENGTH_ID 40


typedef struct
{
    int32_t bias;
    int32_t standardBias;
    int32_t daylightBias;
    SYSTEMTIME standardDate;
    SYSTEMTIME daylightDate;
} TZI;




static const char CURRENT_ZONE_REGKEY[] = "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation\\";
static const char STANDARD_NAME_REGKEY[] = "StandardName";
static const char STANDARD_TIME_REGKEY[] = " Standard Time";
static const char TZI_REGKEY[] = "TZI";
static const char STD_REGKEY[] = "Std";







static const char* const WIN_TYPE_PROBE_REGKEY[] = {
    
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones",

    
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\GMT"

    
};





static const char* const TZ_REGKEY[] = {
    
    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Time Zones\\",

    
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\"
};






enum {
    WIN_9X_ME_TYPE = 1,
    WIN_NT_TYPE = 2,
    WIN_2K_XP_TYPE = 3
};

static int32_t gWinType = 0;

static int32_t detectWindowsType()
{
    int32_t winType;
    LONG result;
    HKEY hkey;

    




    for (winType = 0; winType < 2; winType++) {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                              WIN_TYPE_PROBE_REGKEY[winType],
                              0,
                              KEY_QUERY_VALUE,
                              &hkey);
        RegCloseKey(hkey);

        if (result == ERROR_SUCCESS) {
            break;
        }
    }

    return winType+1; 
}

static LONG openTZRegKey(HKEY *hkey, const char *winid)
{
    char subKeyName[110]; 
    char *name;
    LONG result;

    
    if (gWinType <= 0) {
        gWinType = detectWindowsType();
    }

    uprv_strcpy(subKeyName, TZ_REGKEY[(gWinType != WIN_9X_ME_TYPE)]);
    name = &subKeyName[strlen(subKeyName)];
    uprv_strcat(subKeyName, winid);

    if (gWinType == WIN_9X_ME_TYPE) {
        
        char *pStd = uprv_strstr(subKeyName, STANDARD_TIME_REGKEY);
        if (pStd) {
            *pStd = 0;
        }
    }

    result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                            subKeyName,
                            0,
                            KEY_QUERY_VALUE,
                            hkey);
    return result;
}

static LONG getTZI(const char *winid, TZI *tzi)
{
    DWORD cbData = sizeof(TZI);
    LONG result;
    HKEY hkey;

    result = openTZRegKey(&hkey, winid);

    if (result == ERROR_SUCCESS) {
        result = RegQueryValueExA(hkey,
                                    TZI_REGKEY,
                                    NULL,
                                    NULL,
                                    (LPBYTE)tzi,
                                    &cbData);

    }

    RegCloseKey(hkey);

    return result;
}

static LONG getSTDName(const char *winid, char *regStdName, int32_t length) {
    DWORD cbData = length;
    LONG result;
    HKEY hkey;

    result = openTZRegKey(&hkey, winid);

    if (result == ERROR_SUCCESS) {
        result = RegQueryValueExA(hkey,
                                    STD_REGKEY,
                                    NULL,
                                    NULL,
                                    (LPBYTE)regStdName,
                                    &cbData);

    }

    RegCloseKey(hkey);

    return result;
}





















































U_CFUNC const char* U_EXPORT2
uprv_detectWindowsTimeZone() {
    UErrorCode status = U_ZERO_ERROR;
    UResourceBundle* bundle = NULL;
    char* icuid = NULL;
    UChar apiStd[MAX_LENGTH_ID];
    char apiStdName[MAX_LENGTH_ID];
    char regStdName[MAX_LENGTH_ID];
    char tmpid[MAX_LENGTH_ID];
    int32_t apiStdLength = 0;
    int32_t len;
    int id;
    int errorCode;
    char ISOcode[3]; 

    LONG result;
    TZI tziKey;
    TZI tziReg;
    TIME_ZONE_INFORMATION apiTZI;

    


    uprv_memset(&apiTZI, 0, sizeof(apiTZI));
    uprv_memset(&tziKey, 0, sizeof(tziKey));
    uprv_memset(&tziReg, 0, sizeof(tziReg));
    GetTimeZoneInformation(&apiTZI);
    tziKey.bias = apiTZI.Bias;
    uprv_memcpy((char *)&tziKey.standardDate, (char*)&apiTZI.StandardDate,
           sizeof(apiTZI.StandardDate));
    uprv_memcpy((char *)&tziKey.daylightDate, (char*)&apiTZI.DaylightDate,
           sizeof(apiTZI.DaylightDate));

    
    uprv_memset(apiStdName, 0, sizeof(apiStdName));
    u_strFromWCS(apiStd, MAX_LENGTH_ID, &apiStdLength, apiTZI.StandardName, -1, &status);
    u_austrncpy(apiStdName, apiStd, apiStdLength);

    tmpid[0] = 0;

    id = GetUserGeoID(GEOCLASS_NATION);
    errorCode = GetGeoInfo(id,GEO_ISO2,ISOcode,3,0);

    bundle = ures_openDirect(NULL, "windowsZones", &status);
    ures_getByKey(bundle, "mapTimezones", bundle, &status);

    
    while (U_SUCCESS(status) && ures_hasNext(bundle)) {
        UBool idFound = FALSE;
        const char* winid;
        UResourceBundle* winTZ = ures_getNextResource(bundle, NULL, &status);
        if (U_FAILURE(status)) {
            break;
        }
        winid = ures_getKey(winTZ);
        result = getTZI(winid, &tziReg);

        if (result == ERROR_SUCCESS) {
            


            tziKey.standardBias = tziReg.standardBias;
            tziKey.daylightBias = tziReg.daylightBias;

            if (uprv_memcmp((char *)&tziKey, (char*)&tziReg, sizeof(tziKey)) == 0) {
                const UChar* icuTZ = NULL;
                if (errorCode != 0) {
                    icuTZ = ures_getStringByKey(winTZ, ISOcode, &len, &status);
                }
                if (errorCode==0 || icuTZ==NULL) {
                    
                    status = U_ZERO_ERROR;
                    icuTZ = ures_getStringByKey(winTZ, "001", &len, &status);
                }

                if (U_SUCCESS(status)) {
                    

                    uprv_memset(regStdName, 0, sizeof(regStdName));
                    result = getSTDName(winid, regStdName, sizeof(regStdName));
                    if (result == ERROR_SUCCESS) {
                        if (uprv_strcmp(apiStdName, regStdName) == 0) {
                            idFound = TRUE;
                        }
                    }

                    



                    if (idFound || tmpid[0] == 0) {
                        
                        int index=0;
                        while (! (*icuTZ == '\0' || *icuTZ ==' ')) {
                            tmpid[index++]=(char)(*icuTZ++);  
                        }
                        tmpid[index]='\0';
                    }
                }
            }
        }
        ures_close(winTZ);
        if (idFound) {
            break;
        }
    }

    


    if (tmpid[0] != 0) {
        len = uprv_strlen(tmpid);
        icuid = (char*)uprv_calloc(len + 1, sizeof(char));
        if (icuid != NULL) {
            uprv_strcpy(icuid, tmpid);
        }
    }

    ures_close(bundle);
    
    return icuid;
}

#endif 
