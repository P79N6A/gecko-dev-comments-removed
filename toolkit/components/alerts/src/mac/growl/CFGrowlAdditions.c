








#include <Carbon/Carbon.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "CFGrowlAdditions.h"

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

extern Boolean CFStringGetFileSystemRepresentation() __attribute__((weak_import));
extern CFIndex CFStringGetMaximumSizeOfFileSystemRepresentation(CFStringRef string) __attribute__((weak_import));

char *createFileSystemRepresentationOfString(CFStringRef str) {
	char *buffer;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_4
	


	if (CFStringGetFileSystemRepresentation) {
		CFIndex size = CFStringGetMaximumSizeOfFileSystemRepresentation(str);
		buffer = malloc(size);
		CFStringGetFileSystemRepresentation(str, buffer, size);
	} else 
#endif
	{
		buffer = malloc(512);
		CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, str, kCFURLPOSIXPathStyle, false);
		if (!CFURLGetFileSystemRepresentation(url, false, (UInt8 *)buffer, 512)) {
			free(buffer);
			buffer = NULL;
		}
		CFRelease(url);
	}
	return buffer;
}

STRING_TYPE createStringWithDate(CFDateRef date) {
	CFLocaleRef locale = CFLocaleCopyCurrent();
	CFDateFormatterRef dateFormatter = CFDateFormatterCreate(kCFAllocatorDefault,
															 locale,
															 kCFDateFormatterMediumStyle,
															 kCFDateFormatterMediumStyle);
	CFRelease(locale);
	CFStringRef dateString = CFDateFormatterCreateStringWithDate(kCFAllocatorDefault,
																 dateFormatter,
																 date);
	CFRelease(dateFormatter);
	return dateString;
}

STRING_TYPE createStringWithContentsOfFile(CFStringRef filename, CFStringEncoding encoding) {
	CFStringRef str = NULL;

	char *path = createFileSystemRepresentationOfString(filename);
	if (path) {
		FILE *fp = fopen(path, "rb");
		if (fp) {
			fseek(fp, 0, SEEK_END);
			unsigned long size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			unsigned char *buffer = malloc(size);
			if (buffer && fread(buffer, 1, size, fp) == size)
				str = CFStringCreateWithBytes(kCFAllocatorDefault, buffer, size, encoding, true);
			fclose(fp);
		}
		free(path);
	}

	return str;
}

STRING_TYPE createStringWithStringAndCharacterAndString(STRING_TYPE str0, UniChar ch, STRING_TYPE str1) {
	CFStringRef cfstr0 = (CFStringRef)str0;
	CFStringRef cfstr1 = (CFStringRef)str1;
	CFIndex len0 = (cfstr0 ? CFStringGetLength(cfstr0) : 0);
	CFIndex len1 = (cfstr1 ? CFStringGetLength(cfstr1) : 0);
	size_t length = (len0 + (ch != 0xffff) + len1);

	UniChar *buf = malloc(sizeof(UniChar) * length);
	size_t i = 0U;

	if (cfstr0) {
		CFStringGetCharacters(cfstr0, CFRangeMake(0, len0), buf);
		i += len0;
	}
	if (ch != 0xffff)
		buf[i++] = ch;
	if (cfstr1)
		CFStringGetCharacters(cfstr1, CFRangeMake(0, len1), &buf[i]);

	return CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault, buf, length,  kCFAllocatorMalloc);
}

char *copyCString(STRING_TYPE str, CFStringEncoding encoding) {
	CFIndex size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(str), encoding) + 1;
	char *buffer = calloc(size, 1);
	CFStringGetCString(str, buffer, size, encoding);
	return buffer;
}

STRING_TYPE copyCurrentProcessName(void) {
	ProcessSerialNumber PSN = { 0, kCurrentProcess };
	CFStringRef name = NULL;
	OSStatus err = CopyProcessName(&PSN, &name);
	if (err != noErr) {
		NSLog(CFSTR("in copyCurrentProcessName in CFGrowlAdditions: Could not get process name because CopyProcessName returned %li"), (long)err);
		name = NULL;
	}
	return name;
}

URL_TYPE copyCurrentProcessURL(void) {
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	FSRef fsref;
	CFURLRef URL = NULL;
	OSStatus err = GetProcessBundleLocation(&psn, &fsref);
	if (err != noErr) {
		NSLog(CFSTR("in copyCurrentProcessURL in CFGrowlAdditions: Could not get application location, because GetProcessBundleLocation returned %li\n"), (long)err);
	} else {
		URL = CFURLCreateFromFSRef(kCFAllocatorDefault, &fsref);
	}
	return URL;
}
STRING_TYPE copyCurrentProcessPath(void) {
	CFURLRef URL = copyCurrentProcessURL();
	CFStringRef path = CFURLCopyFileSystemPath(URL, kCFURLPOSIXPathStyle);
	CFRelease(URL);
	return path;
}

URL_TYPE copyTemporaryFolderURL(void) {
	FSRef ref;
	CFURLRef url = NULL;

	OSStatus err = FSFindFolder(kOnAppropriateDisk, kTemporaryFolderType, kCreateFolder, &ref);
	if (err != noErr)
		NSLog(CFSTR("in copyTemporaryFolderPath in CFGrowlAdditions: Could not locate temporary folder because FSFindFolder returned %li"), (long)err);
	else
		url = CFURLCreateFromFSRef(kCFAllocatorDefault, &ref);

	return url;
}
STRING_TYPE copyTemporaryFolderPath(void) {
	CFStringRef path = NULL;

	CFURLRef url = copyTemporaryFolderURL();
	if (url) {
		path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		CFRelease(url);
	}

	return path;
}

DATA_TYPE readFile(const char *filename)
{
	CFDataRef data;
	
	FILE *fp = fopen(filename, "r");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		long dataLength = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		unsigned char *fileData = malloc(dataLength);
		fread(fileData, 1, dataLength, fp);
		fclose(fp);
		data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, fileData, dataLength, kCFAllocatorMalloc);
	} else
		data = NULL;

	return data;
}

URL_TYPE copyURLForApplication(STRING_TYPE appName)
{
	CFURLRef appURL = NULL;
	OSStatus err = LSFindApplicationForInfo(  kLSUnknownCreator,
											 NULL,
											     appName,
											  NULL,
											  &appURL);
	return (err == noErr) ? appURL : NULL;
}

STRING_TYPE createStringWithAddressData(DATA_TYPE aAddressData) {
	struct sockaddr *socketAddress = (struct sockaddr *)CFDataGetBytePtr(aAddressData);
	
	
	
	char stringBuffer[40];
	CFStringRef addressAsString = NULL;
	if (socketAddress->sa_family == AF_INET) {
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)socketAddress;
		if (inet_ntop(AF_INET, &(ipv4->sin_addr), stringBuffer, 40))
			addressAsString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s:%d"), stringBuffer, ipv4->sin_port);
		else
			addressAsString = CFSTR("IPv4 un-ntopable");
	} else if (socketAddress->sa_family == AF_INET6) {
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)socketAddress;
		if (inet_ntop(AF_INET6, &(ipv6->sin6_addr), stringBuffer, 40))
			
			addressAsString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("[%s]:%d"), stringBuffer, ipv6->sin6_port);
		else
			addressAsString = CFSTR("IPv6 un-ntopable");
	} else
		addressAsString = CFSTR("neither IPv6 nor IPv4");

	return addressAsString;
}

STRING_TYPE createHostNameForAddressData(DATA_TYPE aAddressData) {
	char hostname[NI_MAXHOST];
	struct sockaddr *socketAddress = (struct sockaddr *)CFDataGetBytePtr(aAddressData);
	if (getnameinfo(socketAddress, (socklen_t)CFDataGetLength(aAddressData),
					hostname, (socklen_t)sizeof(hostname),
					 NULL,  0,
					NI_NAMEREQD))
		return NULL;
	else
		return CFStringCreateWithCString(kCFAllocatorDefault, hostname, kCFStringEncodingASCII);
}

DATA_TYPE copyIconDataForPath(STRING_TYPE path) {
	CFDataRef data = NULL;

	
	CFURLRef URL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle,  false);
	if (URL) {
		data = copyIconDataForURL(URL);
		CFRelease(URL);
	}

	return data;
}

DATA_TYPE copyIconDataForURL(URL_TYPE URL)
{
	CFDataRef data = NULL;

	if (URL) {
		FSRef ref;
		if (CFURLGetFSRef(URL, &ref)) {
			IconRef icon = NULL;
			SInt16 label_noOneCares;
			OSStatus err = GetIconRefFromFileInfo(&ref,
												   0U,  NULL,
												  kFSCatInfoNone,  NULL,
												  kIconServicesNoBadgeFlag | kIconServicesUpdateIfNeededFlag,
												  &icon,
												  &label_noOneCares);
			if (err != noErr) {
				NSLog(CFSTR("in copyIconDataForURL in CFGrowlAdditions: could not get icon for %@: GetIconRefFromFileInfo returned %li\n"), URL, (long)err);
			} else {
				IconFamilyHandle fam = NULL;
				err = IconRefToIconFamily(icon, kSelectorAllAvailableData, &fam);
				if (err != noErr) {
					NSLog(CFSTR("in copyIconDataForURL in CFGrowlAdditions: could not get icon for %@: IconRefToIconFamily returned %li\n"), URL, (long)err);
				} else {
					HLock((Handle)fam);
					data = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)*(Handle)fam, GetHandleSize((Handle)fam));
					HUnlock((Handle)fam);
					DisposeHandle((Handle)fam);
				}
				ReleaseIconRef(icon);
			}
		}
	}

	return data;
}

URL_TYPE createURLByMakingDirectoryAtURLWithName(URL_TYPE parent, STRING_TYPE name)
{
	CFURLRef newDirectory = NULL;

	CFAllocatorRef allocator = parent ? CFGetAllocator(parent) : name ? CFGetAllocator(name) : kCFAllocatorDefault;

	if (parent) parent = CFRetain(parent);
	else {
		char *cwdBytes = alloca(PATH_MAX);
		getcwd(cwdBytes, PATH_MAX);
		parent = CFURLCreateFromFileSystemRepresentation(allocator, (const unsigned char *)cwdBytes, strlen(cwdBytes),  true);
		if (!name) {
			newDirectory = parent;
			goto end;
		}
	}
	if (!parent)
		NSLog(CFSTR("in createURLByMakingDirectoryAtURLWithName in CFGrowlAdditions: parent directory URL is NULL (please tell the Growl developers)\n"), parent);
	else {
		if (name)
			name = CFRetain(name);
		else {
			name = CFURLCopyLastPathComponent(parent);
			CFURLRef newParent = CFURLCreateCopyDeletingLastPathComponent(allocator, parent);
			CFRelease(parent);
			parent = newParent;
		}

		if (!name)
			NSLog(CFSTR("in createURLByMakingDirectoryAtURLWithName in CFGrowlAdditions: name of directory to create is NULL (please tell the Growl developers)\n"), parent);
		else {
			FSRef parentRef;
			if (!CFURLGetFSRef(parent, &parentRef))
				NSLog(CFSTR("in createURLByMakingDirectoryAtURLWithName in CFGrowlAdditions: could not create FSRef for parent directory at %@ (please tell the Growl developers)\n"), parent);
			else {
				FSRef newDirectoryRef;

				struct HFSUniStr255 nameUnicode;
				CFRange range = { 0, MIN(CFStringGetLength(name), USHRT_MAX) };
				CFStringGetCharacters(name, range, nameUnicode.unicode);
				nameUnicode.length = range.length;

				struct FSRefParam refPB = {
					.ref              = &parentRef,
					.nameLength       = nameUnicode.length,
					.name             = nameUnicode.unicode,
					.whichInfo        = kFSCatInfoNone,
					.catInfo          = NULL,
					.textEncodingHint = kTextEncodingUnknown,
					.newRef           = &newDirectoryRef,
				};

				OSStatus err = PBCreateDirectoryUnicodeSync(&refPB);
				if (err == dupFNErr) {
					
					err = PBMakeFSRefUnicodeSync(&refPB);
				}
				if (err == noErr) {
					NSLog(CFSTR("PBCreateDirectoryUnicodeSync or PBMakeFSRefUnicodeSync returned %li; calling CFURLCreateFromFSRef"), (long)err); 
					newDirectory = CFURLCreateFromFSRef(allocator, &newDirectoryRef);
					NSLog(CFSTR("CFURLCreateFromFSRef returned %@"), newDirectory); 
				} else
					NSLog(CFSTR("in createURLByMakingDirectoryAtURLWithName in CFGrowlAdditions: could not create directory '%@' in parent directory at %@: FSCreateDirectoryUnicode returned %li (please tell the Growl developers)"), name, parent, (long)err);
			}

		} 
		if(parent)
			CFRelease(parent);
		if(name)
			CFRelease(name);
	} 

end:
	return newDirectory;
}

#ifndef COPYFORK_BUFSIZE
#	define COPYFORK_BUFSIZE 5242880U /*5 MiB*/
#endif

static OSStatus copyFork(const struct HFSUniStr255 *forkName, const FSRef *srcFile, const FSRef *destDir, const struct HFSUniStr255 *destName, FSRef *outDestFile) {
	OSStatus err, closeErr;
	struct FSForkIOParam srcPB = {
		.ref = srcFile,
		.forkNameLength = forkName->length,
		.forkName = forkName->unicode,
		.permissions = fsRdPerm,
	};
	unsigned char debuggingPathBuf[PATH_MAX] = "";
	OSStatus debuggingPathErr;

	err = PBOpenForkSync(&srcPB);
	if (err != noErr) {
		debuggingPathErr = FSRefMakePath(srcFile, debuggingPathBuf, PATH_MAX);
		if (debuggingPathErr != noErr)
			snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for source file: FSRefMakePath returned %li)", (long)debuggingPathErr);
		NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBOpenForkSync (source: %s) returned %li"), debuggingPathBuf, (long)err);
	} else {
		FSRef destFile;

		



		struct FSCatalogInfo catInfo;
		struct FSRefParam refPB = {
			.ref       = srcFile,
			.whichInfo = kFSCatInfoGettableInfo & kFSCatInfoSettableInfo,
			.catInfo   = &catInfo,
			.spec      = NULL,
			.parentRef = NULL,
			.outName   = destName ? NULL : (struct HFSUniStr255 *)(destName = alloca(sizeof(struct HFSUniStr255))),
		};

		err = PBGetCatalogInfoSync(&refPB);
		if (err != noErr) {
			debuggingPathErr = FSRefMakePath(srcFile, debuggingPathBuf, PATH_MAX);
			if (debuggingPathErr != noErr)
				snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for source file: FSRefMakePath returned %li)", (long)debuggingPathErr);
			NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBGetCatalogInfoSync (source: %s) returned %li"), debuggingPathBuf, (long)err);
		} else {
			refPB.ref              = destDir;
			refPB.nameLength       = destName->length;
			refPB.name             = destName->unicode;
			refPB.textEncodingHint = kTextEncodingUnknown;
			refPB.newRef           = &destFile;

			const char *functionName = "PBMakeFSRefUnicodeSync"; 

			err = PBMakeFSRefUnicodeSync(&refPB);
			if ((err != noErr) && (err != fnfErr)) {
			handleMakeFSRefError:
				debuggingPathErr = FSRefMakePath(destDir, debuggingPathBuf, PATH_MAX);
				if (debuggingPathErr != noErr)
					snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for destination directory: FSRefMakePath returned %li)", (long)debuggingPathErr);

				
				CFStringRef debuggingFilename = CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
																				   destName->unicode,
																				   destName->length,
																				    kCFAllocatorNull);
				if (!debuggingFilename)
					debuggingFilename = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, "(could not get filename for destination file: CFStringCreateWithCharactersNoCopy returned NULL)", kCFStringEncodingASCII,  kCFAllocatorNull);

				NSLog(CFSTR("in copyFork in CFGrowlAdditions: %s (destination: %s/%@) returned %li"), functionName, debuggingPathBuf, debuggingFilename, (long)err);

				if (debuggingFilename) CFRelease(debuggingFilename);
			} else {
				
				err = PBCreateFileUnicodeSync(&refPB);
				if (err == noErr) {
					



					FNNotify(destDir, kFNDirectoryModifiedMessage, kNilOptions);
				} else if (err == dupFNErr) {
					


					err = noErr;
				} else {
					functionName = "PBCreateFileUnicodeSync";
					goto handleMakeFSRefError;
				}
			}
		}
		if (err == noErr) {
			if (outDestFile)
				memcpy(outDestFile, &destFile, sizeof(destFile));

			struct FSForkIOParam destPB = {
				.ref            = &destFile,
				.forkNameLength = forkName->length,
				.forkName       = forkName->unicode,
				.permissions    = fsWrPerm,
			};
			err = PBOpenForkSync(&destPB);
			NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBOpenForkSync (dest) returned %li"), (long)err);
			if (err != noErr) {
				debuggingPathErr = FSRefMakePath(&destFile, debuggingPathBuf, PATH_MAX);
				if (debuggingPathErr != noErr)
					snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for dest file: FSRefMakePath returned %li)", (long)debuggingPathErr);
				NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBOpenForkSync (destination: %s) returned %li"), debuggingPathBuf, (long)err);
			} else {
				void *buf = malloc(COPYFORK_BUFSIZE);
				if (buf) {
					srcPB.buffer = destPB.buffer = buf;
					srcPB.requestCount = COPYFORK_BUFSIZE;
					while (err == noErr) {
						err = PBReadForkSync(&srcPB);
						if (err == eofErr) {
							err = noErr;
							if (srcPB.actualCount == 0)
								break;
						}
						if (err != noErr) {
							debuggingPathErr = FSRefMakePath(&destFile, debuggingPathBuf, PATH_MAX);
							if (debuggingPathErr != noErr)
								snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for source file: FSRefMakePath returned %li)", (long)debuggingPathErr);
							NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBReadForkSync (source: %s) returned %li"), debuggingPathBuf, (long)err);
						} else {
							destPB.requestCount = srcPB.actualCount;
							err = PBWriteForkSync(&destPB);
							if (err != noErr) {
								debuggingPathErr = FSRefMakePath(&destFile, debuggingPathBuf, PATH_MAX);
								if (debuggingPathErr != noErr)
									snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for dest file: FSRefMakePath returned %li)", (long)debuggingPathErr);
								NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBWriteForkSync (destination: %s) returned %li"), debuggingPathBuf, (long)err);
							}
						}
					}

					free(buf);
				}

				closeErr = PBCloseForkSync(&destPB);
				if (closeErr != noErr) {
					debuggingPathErr = FSRefMakePath(&destFile, debuggingPathBuf, PATH_MAX);
					if (debuggingPathErr != noErr)
						snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for dest file: FSRefMakePath returned %li)", (long)debuggingPathErr);
					NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBCloseForkSync (destination: %s) returned %li"), debuggingPathBuf, (long)err);
				}
				if (err == noErr) err = closeErr;
			}
		}

		closeErr = PBCloseForkSync(&srcPB);
		if (closeErr != noErr) {
			debuggingPathErr = FSRefMakePath(&destFile, debuggingPathBuf, PATH_MAX);
			if (debuggingPathErr != noErr)
				snprintf((char *)debuggingPathBuf, PATH_MAX, "(could not get path for source file: FSRefMakePath returned %li)", (long)debuggingPathErr);
			NSLog(CFSTR("in copyFork in CFGrowlAdditions: PBCloseForkSync (source: %s) returned %li"), debuggingPathBuf, (long)err);
		}
		if (err == noErr) err = closeErr;
	}

	return err;
}

static OSStatus GrowlCopyObjectSync(const FSRef *fileRef, const FSRef *destRef, FSRef *destFileRef) {
	OSStatus err;
	struct HFSUniStr255 forkName;
	struct FSForkIOParam forkPB = {
		.ref = fileRef,
		.forkIterator = {
			.initialize = 0
		},
		.outForkName = &forkName,
	};

	do {
		err = PBIterateForksSync(&forkPB);
		NSLog(CFSTR("PBIterateForksSync returned %li"), (long)err);
		if (err != noErr) {
			if (err != errFSNoMoreItems)
				NSLog(CFSTR("in GrowlCopyObjectSync in CFGrowlAdditions: PBIterateForksSync returned %li"), (long)err);
		} else {
			err = copyFork(&forkName, fileRef, destRef,  NULL,  destFileRef);
			
		}
	} while (err == noErr);
	if (err == errFSNoMoreItems) err = noErr;

	return err;
}

URL_TYPE createURLByCopyingFileFromURLToDirectoryURL(URL_TYPE file, URL_TYPE dest)
{
	CFURLRef destFileURL = NULL;

	FSRef fileRef, destRef, destFileRef;
	Boolean gotFileRef = CFURLGetFSRef(file, &fileRef);
	Boolean gotDestRef = CFURLGetFSRef(dest, &destRef);
	if (!gotFileRef)
		NSLog(CFSTR("in createURLByCopyingFileFromURLToDirectoryURL in CFGrowlAdditions: CFURLGetFSRef failed with source URL %@"), file);
	else if (!gotDestRef)
		NSLog(CFSTR("in createURLByCopyingFileFromURLToDirectoryURL in CFGrowlAdditions: CFURLGetFSRef failed with destination URL %@"), dest);
	else {
		OSStatus err;

		



#if defined(NSAppKitVersionNumber10_3) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4 && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
		if (FSCopyObjectSync) {
			err = FSCopyObjectSync(&fileRef, &destRef,  NULL, &destFileRef, kFSFileOperationOverwrite);
		} else {
#endif
			err = GrowlCopyObjectSync(&fileRef, &destRef, &destFileRef);
#if defined(NSAppKitVersionNumber10_3) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4 && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
		}
#endif

		if (err == noErr)
			destFileURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &destFileRef);
		else
			NSLog(CFSTR("in createURLByCopyingFileFromURLToDirectoryURL in CFGrowlAdditions: CopyObjectSync returned %li for source URL %@"), (long)err, file);
	}

	return destFileURL;
}

PLIST_TYPE createPropertyListFromURL(URL_TYPE file, u_int32_t mutability, CFPropertyListFormat *outFormat, STRING_TYPE *outErrorString)
{
	CFPropertyListRef plist = NULL;

	if (!file)
		NSLog(CFSTR("in createPropertyListFromURL in CFGrowlAdditions: cannot read from a NULL URL"));
	else {
		CFReadStreamRef stream = CFReadStreamCreateWithFile(kCFAllocatorDefault, file);
		if (!stream)
			NSLog(CFSTR("in createPropertyListFromURL in CFGrowlAdditions: could not create stream for reading from URL %@"), file);
		else {
			if (!CFReadStreamOpen(stream))
				NSLog(CFSTR("in createPropertyListFromURL in CFGrowlAdditions: could not open stream for reading from URL %@"), file);
			else {
				CFPropertyListFormat format;
				CFStringRef errorString = NULL;

				plist = CFPropertyListCreateFromStream(kCFAllocatorDefault,
													   stream,
													    0,
													   mutability,
													   &format,
													   &errorString);
				if (!plist)
					NSLog(CFSTR("in createPropertyListFromURL in CFGrowlAdditions: could not read property list from URL %@ (error string: %@)"), file, errorString);

				if (outFormat) *outFormat = format;
				if (errorString) {
					if (outErrorString)
						*outErrorString = errorString;
					else
						CFRelease(errorString);
				}

				CFReadStreamClose(stream);
			}

			CFRelease(stream);
		}
	}

	return plist;
}
