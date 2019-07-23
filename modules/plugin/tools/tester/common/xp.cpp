




































#include "xp.h"

#ifndef XP_WIN
#include "pplib.h"
#endif 








BOOL XP_IsFile(LPSTR szFileName)
{
#ifdef XP_WIN
  OFSTRUCT of;
  return (HFILE_ERROR != OpenFile(szFileName, &of, OF_EXIST));
#endif
#if defined(XP_UNIX) || defined(XP_OS2)
  struct stat s;
  return (stat(szFileName, &s) != -1);
#endif
#ifdef XP_MAC 
	return 1;
#endif
}

void XP_DeleteFile(LPSTR szFileName)
{
#ifdef XP_WIN
  DeleteFile(szFileName);
  return;
#endif 

#if (defined XP_UNIX || defined XP_MAC || defined XP_OS2)
  remove(szFileName);
  return;
#endif 
}

XP_HFILE XP_CreateFile(LPSTR szFileName)
{
#ifdef XP_WIN
  OFSTRUCT of;
  HFILE hFile = OpenFile(szFileName, &of, OF_CREATE | OF_WRITE);
  return (hFile != HFILE_ERROR) ? hFile : NULL;
#else
  return (XP_HFILE)fopen(szFileName, "w+");
#endif 
}

XP_HFILE XP_OpenFile(LPSTR szFileName)
{
#ifdef XP_WIN
  OFSTRUCT of;
  HFILE hFile = OpenFile(szFileName, &of, OF_READ | OF_WRITE);
  return (hFile != HFILE_ERROR) ? hFile : NULL;
#else
  return (XP_HFILE)fopen(szFileName, "r+");
#endif 
}

void XP_CloseFile(XP_HFILE hFile)
{
  if(hFile != NULL)
  {
#ifdef XP_WIN
    CloseHandle((HANDLE)hFile);
    return;
#endif 

#if (defined UNIX || defined XP_MAC || defined XP_OS2)
    fclose(hFile);
    return;
#endif 
  }
}

DWORD XP_WriteFile(XP_HFILE hFile, void * pBuf, int iSize)
{
#ifdef XP_WIN
  DWORD dwRet;
  WriteFile((HANDLE)hFile, pBuf, iSize, &dwRet, NULL);
  return dwRet;
#endif 

#if (defined XP_UNIX || defined XP_MAC || defined XP_OS2)
  return (DWORD)fwrite(pBuf, iSize, 1, hFile);
#endif 
}

void XP_FlushFileBuffers(XP_HFILE hFile)
{
#ifdef XP_WIN
  FlushFileBuffers((HANDLE)hFile);
  return;
#endif 

#if (defined XP_UNIX || defined XP_MAC || defined XP_OS2)
  fflush(hFile);
#endif 
}







DWORD XP_GetPrivateProfileString(LPSTR szSection, LPSTR szKey, LPSTR szDefault, LPSTR szString, DWORD dwSize, LPSTR szFileName)
{
#ifdef XP_WIN
  return GetPrivateProfileString(szSection, szKey, szDefault, szString, dwSize, szFileName);
#else
  XP_HFILE hFile = XP_OpenFile(szFileName);
  if(hFile == NULL)
  {
    strcpy(szString, szDefault);
    int iRet = strlen(szString);
    return iRet;
  }
  DWORD dwRet = PP_GetString(szSection, szKey, szDefault, szString, dwSize, hFile);
  XP_CloseFile(hFile);
  return dwRet;
#endif 
}

int XP_GetPrivateProfileInt(LPSTR szSection, LPSTR szKey, int iDefault, LPSTR szFileName)
{
#ifdef XP_WIN
  return GetPrivateProfileInt(szSection, szKey, iDefault, szFileName);
#else
  static char szString[80];
  XP_GetPrivateProfileString(szSection, szKey, "", szString, sizeof(szString), szFileName);
  if(!*szString)
    return iDefault;
  int iRet = atoi(szString);
  return iRet;
#endif 
}

BOOL XP_WritePrivateProfileString(LPSTR szSection, LPSTR szKey, LPSTR szString, LPSTR szFileName)
{
#ifdef XP_WIN
  return WritePrivateProfileString(szSection, szKey, szString, szFileName);
#else
  XP_HFILE hFile = XP_OpenFile(szFileName);
  if(hFile == NULL)
  {
    hFile = XP_CreateFile(szFileName);
    if(hFile == NULL)
      return FALSE;
  }
  BOOL bRet = PP_WriteString(szSection, szKey, szString, hFile);
  XP_CloseFile(hFile);
  return bRet;
#endif 
}

BOOL XP_WritePrivateProfileInt(LPSTR szSection, LPSTR szKey, int iValue, LPSTR szFileName)
{
  char szString[80];
  _itoa(iValue, szString, 10);
  return XP_WritePrivateProfileString(szSection, szKey, szString, szFileName);
}








DWORD XP_GetTickCount()
{
#ifdef XP_WIN
  return GetTickCount();
#else
  return (DWORD)0;
#endif 
}

void XP_Sleep(DWORD dwSleepTime)
{
  if((long)dwSleepTime <= 0L)
    return;

#ifdef XP_WIN
  Sleep(dwSleepTime);
#endif 
}
