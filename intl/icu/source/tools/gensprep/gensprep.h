















#ifndef __GENIDN_H__
#define __GENIDN_H__

#include "unicode/utypes.h"
#include "sprpimpl.h"


#define DATA_NAME "sprep"
#define DATA_TYPE "spp"






 

extern UBool beVerbose, haveCopyright;



extern void
setUnicodeVersion(const char *v);

extern void
setUnicodeVersionNC(UVersionInfo version);

extern void
init(void);

#if !UCONFIG_NO_IDNA
extern void
storeMapping(uint32_t codepoint, uint32_t* mapping,int32_t length, UStringPrepType type, UErrorCode* status);
extern void
storeRange(uint32_t start, uint32_t end, UStringPrepType type,UErrorCode* status);
#endif

extern void
generateData(const char *dataDir, const char* bundleName);

extern void
setOptions(int32_t options);

extern void
cleanUpData(void);










#endif









