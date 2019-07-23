







































#include <stdlib.h>

#include "nsMacUtils.h"


OSErr MyNewHandle(long length, Handle *outHandle)
{
  Handle    tempHandle = NewHandle(length);
  if (!tempHandle) return memFullErr;
  *outHandle = tempHandle;
  return noErr;
}


void MyDisposeHandle(Handle theHandle)
{
  if (theHandle)
    DisposeHandle(theHandle);
}


void MyHLock(Handle theHandle)
{
  HLock(theHandle);
}

void MyHUnlock(Handle theHandle)
{
  HUnlock(theHandle);
}

#pragma mark -

OSErr MyNewBlockClear(long length, void* *outBlock)
{
  void* newData = calloc(length, 1);
  if (!newData) return memFullErr;
  *outBlock = newData;
  return noErr;
}

void MyDisposeBlock(void *dataBlock)
{
  if (dataBlock)
    free(dataBlock);
}

#pragma mark -







void StrCopySafe(char *dst, const char *src, long destLen)
{
	const unsigned char	*p = (unsigned char *) src - 1;
	unsigned char 		*q = (unsigned char *) dst - 1;
	unsigned char		*end = (unsigned char *)dst + destLen;
	
	while ( q < end && (*++q = (*++p)) != '\0' ) {;}
	if (q == end)
		*q = '\0';
}










void CopyPascalString (StringPtr to, const StringPtr from)
{
	BlockMoveData(from, to, *from+1);
}








void CopyCToPascalString(const char *src, StringPtr dest, long maxLen)
{
	char		*s = (char *)src;
	char		*d = (char *)(&dest[1]);
	long		count = 0;
		
	while (count < maxLen && *s) {
		*d ++ = *s ++;
		count ++;
	}
	
	dest[0] = (unsigned char)(count);
}








void CopyPascalToCString(const StringPtr src, char *dest, long maxLen)
{
	short	len = Min(maxLen, src[0]);
	
	BlockMoveData(&src[1], dest, len);
	dest[len] = '\0';
}














OSErr GetShortVersionString(short rID, StringPtr version)
{
	VersRecHndl		versionH;
	OSErr			error = resNotFound;
	
	versionH = (VersRecHndl)Get1Resource('vers', rID);		
	
	if (versionH)
	{
		CopyPascalString(version, (**versionH).shortVersion);
		ReleaseResource((Handle) versionH);
		error = noErr;
	}
	else
 		CopyPascalString(version, (StringPtr)"\p<unknown>");

	return error;
}


