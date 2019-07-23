




































#ifndef macio_h__
#define macio_h__


PR_BEGIN_EXTERN_C

OSErr ConvertUnixPathToMacPath(const char *, char **);
OSErr ConvertUnixPathToFSSpec(const char *unixPath, FSSpec *fileSpec);

PR_END_EXTERN_C


#endif 

