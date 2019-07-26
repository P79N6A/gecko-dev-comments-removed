






















#ifndef LOCMAP_H
#define LOCMAP_H

#include "unicode/utypes.h"

#define LANGUAGE_LCID(hostID) (uint16_t)(0x03FF & hostID)

U_CAPI int32_t uprv_convertToPosix(uint32_t hostid, char* posixID, int32_t posixIDCapacity, UErrorCode* status);


U_CAPI uint32_t uprv_convertToLCID(const char *langID, const char* posixID, UErrorCode* status);

#endif 

