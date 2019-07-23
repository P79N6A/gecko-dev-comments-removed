



#pragma once

#ifndef	__JMURLConnection__
#define	__JMURLConnection__

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import on
#endif

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=mac68k
#endif

typedef void* JMURLConnectionRef;
typedef void* JMURLInputStreamRef;
typedef void* JMURLOutputStreamRef;

enum JMURLConnectionOptions
{
	eDefault			= 0,
	eNoCaching			= (1 << 2),
	eNoUserInteraction	= (1 << 3),
	eNoRedirection		= (1 << 4)
};

typedef enum JMURLConnectionOptions JMURLConnectionOptions;

typedef CALLBACK_API_C(OSStatus, JMURLOpenConnectionProcPtr)(
	 JMTextRef url,
	 JMTextRef requestMethod,
	 JMURLConnectionOptions options,
	 JMAppletViewerRef appletViewer,
	 JMURLConnectionRef* urlConnectionRef
	);

typedef CALLBACK_API_C(OSStatus, JMURLCloseConnectionProcPtr)(
	 JMURLConnectionRef urlConnectionRef
	);

typedef CALLBACK_API_C(Boolean, JMURLUsingProxyProcPtr)(
	 JMURLConnectionRef urlConnectionRef
	);

typedef CALLBACK_API_C(OSStatus, JMURLGetCookieProcPtr)(
	 JMURLConnectionRef urlConnectionRef,
	 JMTextRef* cookie
	);

typedef CALLBACK_API_C(OSStatus, JMURLSetCookieProcPtr)(
	 JMURLConnectionRef urlConnectionRef,
	 JMTextRef cookie
	);

typedef CALLBACK_API_C(OSStatus, JMURLSetRequestPropertiesProcPtr)(
	 JMURLConnectionRef urlConnectionRef,
	 int numberOfProperties,
	 JMTextRef* keys,
	 JMTextRef* value
	);

typedef CALLBACK_API_C(OSStatus, JMURLGetResponsePropertiesCountProcPtr)(
	 JMURLInputStreamRef iStreamRef,
	 int* numberOfProperties
	);







typedef CALLBACK_API_C(OSStatus, JMURLGetResponsePropertiesProcPtr)(
	 JMURLInputStreamRef iStreamRef,
	 int numberOfProperties,
	 JMTextRef* keys,
	 JMTextRef* values
	);

typedef CALLBACK_API_C(OSStatus, JMURLOpenInputStreamProcPtr)(
	 JMURLConnectionRef urlConnectionRef,
	 JMURLInputStreamRef* urlInputStreamRef
	);

typedef CALLBACK_API_C(OSStatus, JMURLOpenOutputStreamProcPtr)(
	 JMURLConnectionRef urlConnectionRef,
	 JMURLOutputStreamRef* urlOutputStreamRef
	);

typedef CALLBACK_API_C(OSStatus, JMURLCloseInputStreamProcPtr)(
	 JMURLInputStreamRef urlInputStreamRef
	);

typedef CALLBACK_API_C(OSStatus, JMURLCloseOutputStreamProcPtr)(
	 JMURLOutputStreamRef urlOutputStreamRef
	);

typedef CALLBACK_API_C(OSStatus, JMURLReadProcPtr)(
	 JMURLInputStreamRef iStreamRef,
	 void* buffer,
	 UInt32 bufferSize,
	 SInt32* bytesRead
	);

typedef CALLBACK_API_C(OSStatus, JMURLWriteProcPtr)(
	 JMURLOutputStreamRef oStreamRef,
	 void* buffer,
	 SInt32 bytesToWrite
	);

struct JMURLCallbacks {
	UInt32 									fVersion;	
	JMURLOpenConnectionProcPtr 				fOpenConnection;
	JMURLCloseConnectionProcPtr 			fCloseConnection;
	JMURLUsingProxyProcPtr					fUsingProxy;
	JMURLGetCookieProcPtr					fGetCookie;
	JMURLSetCookieProcPtr					fSetCookie;
	JMURLSetRequestPropertiesProcPtr 		fSetRequestProperties;
	JMURLGetResponsePropertiesCountProcPtr	fGetResponsePropertiesCount;
	JMURLGetResponsePropertiesProcPtr		fGetResponseProperties;
	JMURLOpenInputStreamProcPtr				fOpenInputStream;
	JMURLOpenOutputStreamProcPtr			fOpenOutputStream;
	JMURLCloseInputStreamProcPtr			fCloseInputStream;
	JMURLCloseOutputStreamProcPtr			fCloseOutputStream;
	JMURLReadProcPtr 						fRead;
	JMURLWriteProcPtr 						fWrite;
};

typedef struct JMURLCallbacks		JMURLCallbacks;

EXTERN_API_C(OSStatus)
JMURLSetCallbacks(
	JMSessionRef session,
	const char* protocol,
	JMURLCallbacks* cb
	);

EXTERN_API_C(OSStatus)
JMURLOpenConnection(
	 JMSessionRef session,
	 JMTextRef url,
	 JMTextRef requestMethod,
	 JMURLConnectionOptions options,
	 JMAppletViewerRef appletViewer,
	 JMURLConnectionRef* urlConnectionRef
	);

EXTERN_API_C(OSStatus)
JMURLCloseConnection(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef
	);

EXTERN_API_C(Boolean)
JMURLUsingProxy(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef
	);

EXTERN_API_C(OSStatus)
JMURLGetCookie(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef,
	 JMTextRef* cookie
	);

EXTERN_API_C(OSStatus)
JMURLSetCookie(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef,
	 JMTextRef cookie
	);

EXTERN_API_C(OSStatus)
JMURLSetRequestProperties(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef,
	 int numberOfProperties,
	 JMTextRef* keys,
	 JMTextRef* value
	);

EXTERN_API_C(OSStatus)
JMURLGetResponsePropertiesCount(
	 JMSessionRef session,
	 JMURLInputStreamRef iStreamRef,
	 int* numberOfProperties
	);

EXTERN_API_C(OSStatus)
JMURLGetResponseProperties(
	 JMSessionRef session,
	 JMURLInputStreamRef iStreamRef,
	 int numberOfProperties,
	 JMTextRef* keys,
	 JMTextRef* values
	);

EXTERN_API_C(OSStatus)
JMURLOpenInputStream(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef,
	 JMURLInputStreamRef* urlInputStreamRef
	);

EXTERN_API_C(OSStatus)
JMURLOpenOutputStream(
	 JMSessionRef session,
	 JMURLConnectionRef urlConnectionRef,
	 JMURLOutputStreamRef* urlOutputStreamRef
	);

EXTERN_API_C(OSStatus)
JMURLCloseInputStream(
	 JMSessionRef session,
	 JMURLInputStreamRef urlInputStreamRef
	);

EXTERN_API_C(OSStatus)
JMURLCloseOutputStream(
	 JMSessionRef session,
	 JMURLOutputStreamRef urlOutputStreamRef
	);

EXTERN_API_C(OSStatus)
JMURLRead(
	 JMSessionRef session,
	 JMURLInputStreamRef urlInputStreamRef,
	 void* buffer,
	 UInt32 bufferSize,
	 SInt32* bytesRead
	);

EXTERN_API_C(OSStatus)
JMURLWrite(
	 JMSessionRef session,
	 JMURLOutputStreamRef urlOutputStreamRef,
	 void* buffer,
	 SInt32 bytesToWrite
	);

#if PRAGMA_ALIGN_SUPPORTED
#pragma options align=reset
#endif

#if PRAGMA_IMPORT_SUPPORTED
#pragma import off
#endif

#ifdef __cplusplus
}
#endif

#endif

