




























#ifndef COMMON_LINUX_GUID_CREATOR_H__
#define COMMON_LINUX_GUID_CREATOR_H__

#include "google_breakpad/common/minidump_format.h"

typedef MDGUID GUID;


#define kGUIDFormatString "%08x-%04x-%04x-%08x-%08x"

#define kGUIDStringLength 36


bool CreateGUID(GUID *guid);


bool GUIDToString(const GUID *guid, char *buf, int buf_len);

#endif
