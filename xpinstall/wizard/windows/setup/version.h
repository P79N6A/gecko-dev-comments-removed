




































#ifndef _version_h
#define _version_h

typedef struct structVer
{
  ULONGLONG ullMajor;
  ULONGLONG ullMinor;
  ULONGLONG ullRelease;
  ULONGLONG ullBuild;
} verBlock;

void  TranslateVersionStr(LPSTR szVersion, verBlock *vbVersion);
BOOL  GetFileVersion(LPSTR szFile, verBlock *vbVersion);
int   CompareVersion(verBlock vbVersionOld, verBlock vbVersionNew);

#endif 

