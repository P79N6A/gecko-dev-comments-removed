







#ifdef __OBJC__
#	define DATA_TYPE NSData *
#	define DICTIONARY_TYPE NSDictionary *
#	define STRING_TYPE NSString *
#	define ARRAY_TYPE NSArray *
#	define URL_TYPE NSURL *
#	define PLIST_TYPE NSObject *
#else
#	define DATA_TYPE CFDataRef
#	define DICTIONARY_TYPE CFDictionaryRef
#	define STRING_TYPE CFStringRef
#	define ARRAY_TYPE CFArrayRef
#	define URL_TYPE CFURLRef
#	define PLIST_TYPE CFPropertyListRef
#endif

STRING_TYPE copyCurrentProcessName(void);
URL_TYPE    copyCurrentProcessURL(void);
STRING_TYPE copyCurrentProcessPath(void);

URL_TYPE    copyTemporaryFolderURL(void);
STRING_TYPE copyTemporaryFolderPath(void);

STRING_TYPE createStringWithAddressData(DATA_TYPE aAddressData);
STRING_TYPE createHostNameForAddressData(DATA_TYPE aAddressData);

DICTIONARY_TYPE createDockDescriptionForURL(URL_TYPE url);





DATA_TYPE copyIconDataForPath(STRING_TYPE path);




DATA_TYPE copyIconDataForURL(URL_TYPE URL);














URL_TYPE createURLByMakingDirectoryAtURLWithName(URL_TYPE parent, STRING_TYPE name);






URL_TYPE createURLByCopyingFileFromURLToDirectoryURL(URL_TYPE file, URL_TYPE dest);











PLIST_TYPE createPropertyListFromURL(URL_TYPE file, u_int32_t mutability, CFPropertyListFormat *outFormat, STRING_TYPE *outErrorString);
