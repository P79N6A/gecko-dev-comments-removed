














#pragma once

#ifdef _UNICODE

#ifndef _T
#define __T(x)   L ## x
#define _T(x)    __T(x)
#define _TEXT(x) __T(x)
#endif
typedef wchar_t TCHAR;
typedef wchar_t _TUCHAR;


#define _tmain      wmain
#define _tWinMain   wWinMain
#define _tenviron   _wenviron
#define __targv     __wargv


#define _ftprintf   fwprintf
#define _sntprintf  _snwprintf
#define _stprintf   _swprintf
#define _tprintf    wprintf
#define _vftprintf  vfwprintf
#define _vsntprintf _vsnwprintf
#define _vstprintf  _vswprintf


#define _tscanf     wscanf
#define _stscanf    swscanf


#define _tcscat     wcscat
#define _tcschr     wcschr
#define _tcsclen    wcslen
#define _tcscpy     wcscpy
#define _tcsdup     _wcsdup
#define _tcslen     wcslen
#define _tcsnccpy   wcsncpy
#define _tcsncpy    wcsncpy
#define _tcsrchr    wcsrchr
#define _tcsstr     wcsstr
#define _tcstok     wcstok


#define _tcscmp     wcscmp
#define _tcsicmp    _wcsicmp
#define _tcsncicmp  _wcsnicmp
#define _tcsncmp    wcsncmp
#define _tcsnicmp   _wcsnicmp


#define _tcslwr     _wcslwr
#define _tcsupr     _wcsupr
#define _totlower   towlower
#define _totupper   towupper


#define _tcstoi64   _wcstoi64
#define _tcstol     wcstol
#define _tcstoul    wcstoul
#define _tstof      _wtof
#define _tstoi      _wtoi
#define _tstoi64    _wtoi64
#define _ttoi       _wtoi
#define _ttoi64     _wtoi64
#define _ttol       _wtol


#define _itot       _itow
#define _ltot       _ltow
#define _i64tot     _i64tow
#define _ui64tot    _ui64tow


#define _tfopen     _wfopen
#define _topen      _wopen
#define _tremove    _wremove
#define _tunlink    _wunlink


#define _fgettc     fgetwc
#define _fgetts     fgetws
#define _fputts     fputws
#define _gettchar   getwchar


#define _tchdir     _wchdir


#define _tgetenv    _wgetenv
#define _tsystem    _wsystem


#define _tcsftime   wcsftime

#else 

#ifndef _T
#define _T(x)    x
#define _TEXT(x) x
#endif
typedef char            TCHAR;
typedef unsigned char   _TUCHAR;


#define _tmain      main
#define _tWinMain   WinMain
#define _tenviron   environ
#define __targv     __argv


#define _ftprintf   fprintf
#define _sntprintf  _snprintf
#define _stprintf   sprintf
#define _tprintf    printf
#define _vftprintf  vfprintf
#define _vsntprintf _vsnprintf
#define _vstprintf  vsprintf


#define _tscanf     scanf
#define _stscanf    sscanf


#define _tcscat     strcat
#define _tcschr     strchr
#define _tcsclen    strlen
#define _tcscnlen   strnlen
#define _tcscpy     strcpy
#define _tcsdup     _strdup
#define _tcslen     strlen
#define _tcsnccpy   strncpy
#define _tcsrchr    strrchr
#define _tcsstr     strstr
#define _tcstok     strtok


#define _tcscmp     strcmp
#define _tcsicmp    _stricmp
#define _tcsncmp    strncmp
#define _tcsncicmp  _strnicmp
#define _tcsnicmp   _strnicmp


#define _tcslwr     _strlwr
#define _tcsupr     _strupr

#define _totupper   toupper
#define _totlower   tolower


#define _tcstol     strtol
#define _tcstoul    strtoul
#define _tstof      atof
#define _tstoi      atoi
#define _tstoi64    _atoi64
#define _tstoi64    _atoi64
#define _ttoi       atoi
#define _ttoi64     _atoi64
#define _ttol       atol


#define _i64tot     _i64toa
#define _itot       _itoa
#define _ltot       _ltoa
#define _ui64tot    _ui64toa


#define _tfopen     fopen
#define _topen      _open
#define _tremove    remove
#define _tunlink    _unlink


#define _fgettc     fgetc
#define _fgetts     fgets
#define _fputts     fputs
#define _gettchar   getchar


#define _tchdir     _chdir


#define _tgetenv    getenv
#define _tsystem    system


#define _tcsftime   strftime

#endif


#define _istgraph   isgraph
#define _istascii   __isascii
