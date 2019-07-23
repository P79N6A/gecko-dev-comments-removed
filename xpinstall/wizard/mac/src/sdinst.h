#ifndef _sdinst_h
#define _sdinst_h

#include <ConditionalMacros.h>
#include <MacTypes.h>
#include <Quickdraw.h>
#include <MixedMode.h>
#include <Events.h>

#if PRAGMA_ONCE
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if PRAGMA_IMPORT
#pragma import on
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=mac68k
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(push, 2)
#elif PRAGMA_STRUCT_PACK
	#pragma pack(2)
#endif

#define	DEFAULT_TIMEOUT		60
#define DEFAULT_RETRIES		5

typedef enum
{
	idle = 0,
	inProgress,
	userCanceled,
	failure,
	success
} DLStatus;

#ifdef WANT_WINTYPES
	typedef unsigned long DWORD;
	typedef int HINSTANCE;
	#define CONST const
	typedef char CHAR;
	typedef CONST CHAR* LPCSTR;
	typedef LPCSTR LPCTSTR;
	typedef long LONG;
	typedef LONG LPARAM;
	typedef void* LPVOID;
	typedef long* LPLONG;
	#define _cdecl
	typedef long HRESULT;
	typedef int HWND;
#endif

typedef struct SDI_tag
{
	DWORD		dwStructSize;
	HWND		hwndOwner;
	HINSTANCE	hInstance;
	DWORD		dwFlags;		
	DWORD		dwTimeOut;		
	DWORD		dwRetries;		
#if macintosh
	FSSpec		fsIDIFile;		
	short		dlDirVRefNum;	
	long		dlDirID;		
#else
	LPCTSTR		lpFileName;		
	LPCTSTR		lpDownloadDir;	
#endif
	LPCTSTR		lpTemplate;		
#if 0 
	DLGPROC		lpDlgHookProc;	
#endif
	LPARAM		lUserData;		
	DWORD		dwReserved;
	
} SDISTRUCT, *LPSDISTRUCT;



#define SDI_STARTOVER		0x00000001
	


#define SDI_USETEMPLATE		0x00000002
	




#define SDI_USEHOOK			0x00000004
	






#define SDI_DEFAULT_RETRIES	0x00000008

#define SDI_DEFAULT_TIMEOUT	0x00000010



HRESULT _cdecl SDI_NetInstall (LPSDISTRUCT);
HRESULT _cdecl SDI_QueryLog (	LPCTSTR,
								LPVOID,	
								LPLONG);

#if MACINTOSH
Boolean SDI_HandleEvent(const EventRecord* event);
#endif

#if PRAGMA_STRUCT_ALIGN
	#pragma options align=reset
#elif PRAGMA_STRUCT_PACKPUSH
	#pragma pack(pop)
#elif PRAGMA_STRUCT_PACK
	#pragma pack()
#endif

#ifdef PRAGMA_IMPORT_OFF
#pragma import off
#elif PRAGMA_IMPORT
#pragma import reset
#endif

#ifdef __cplusplus
}
#endif

#endif
