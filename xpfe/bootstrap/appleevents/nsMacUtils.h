






































#ifndef nsMacUtils_h_
#define nsMacUtils_h_

#include <MacTypes.h>
#include <Memory.h>
#include <Errors.h>
#include <Resources.h>

#define Min(a, b)   ((a) < (b) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif

OSErr MyNewHandle(long length, Handle *outHandle);
void MyDisposeHandle(Handle theHandle);

void MyHLock(Handle theHandle);
void MyHUnlock(Handle theHandle);



OSErr MyNewBlockClear(long length, void* *outBlock);
void MyDisposeBlock(void *dataBlock);


void StrCopySafe(char *dst, const char *src, long destLen);
void CopyPascalString (StringPtr to, const StringPtr from);
void CopyCToPascalString(const char *src, StringPtr dest, long maxLen);
void CopyPascalToCString(const StringPtr src, char *dest, long maxLen);

OSErr GetShortVersionString(short rID, StringPtr version);

#ifdef __cplusplus
}
#endif

#endif
