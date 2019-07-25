








#ifndef HAVE_CFGROWLADDITIONS_H
#define HAVE_CFGROWLADDITIONS_H

#include "CFGrowlDefines.h"


extern void NSLog(STRING_TYPE format, ...);

char *createFileSystemRepresentationOfString(STRING_TYPE str);
STRING_TYPE createStringWithDate(DATE_TYPE date);

STRING_TYPE createStringWithContentsOfFile(STRING_TYPE filename, CFStringEncoding encoding);


STRING_TYPE createStringWithStringAndCharacterAndString(STRING_TYPE str0, UniChar ch, STRING_TYPE str1);

char *copyCString(STRING_TYPE str, CFStringEncoding encoding);

STRING_TYPE copyCurrentProcessName(void);
URL_TYPE    copyCurrentProcessURL(void);
STRING_TYPE copyCurrentProcessPath(void);

URL_TYPE    copyTemporaryFolderURL(void);
STRING_TYPE copyTemporaryFolderPath(void);

STRING_TYPE createStringWithAddressData(DATA_TYPE aAddressData);
STRING_TYPE createHostNameForAddressData(DATA_TYPE aAddressData);

DATA_TYPE readFile(const char *filename);
URL_TYPE  copyURLForApplication(STRING_TYPE appName);





DATA_TYPE copyIconDataForPath(STRING_TYPE path);




DATA_TYPE copyIconDataForURL(URL_TYPE URL);














URL_TYPE createURLByMakingDirectoryAtURLWithName(URL_TYPE parent, STRING_TYPE name);






URL_TYPE createURLByCopyingFileFromURLToDirectoryURL(URL_TYPE file, URL_TYPE dest);











PLIST_TYPE createPropertyListFromURL(URL_TYPE file, u_int32_t mutability, CFPropertyListFormat *outFormat, STRING_TYPE *outErrorString);

#endif
