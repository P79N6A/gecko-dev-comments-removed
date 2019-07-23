






















































#include "xp.h"

static BOOL isCurrentLineCommentedOut(char * pBuf, char * pPosition)
{
  for(;;)
  {
    if((*pPosition == ';') && ((pPosition - 1 < pBuf) || (*(pPosition - 1) == '\n') || (*(pPosition - 1) == '\r')))
      return TRUE;
    pPosition--;
    if(pPosition < pBuf)
      return FALSE;
    if((*pPosition == '\n') || (*pPosition == '\r'))
      return FALSE;
  }
}









static char * goToSectionBody(char * pBuf, char * szSection)
{
  if((pBuf == NULL) || (szSection == NULL))
    return NULL;
  char szSectionString[256];
  strcpy(szSectionString, "\n");   
  strcat(szSectionString, "[");
  strcat(szSectionString, szSection);
  strcat(szSectionString, "]");
  char * p = strstr(pBuf, szSectionString);
  if(p == NULL) {
    szSectionString[0] = '\r';     
    char * p = strstr(pBuf, szSectionString);
    if(p == NULL)
      return NULL;
  }
  p += strlen(szSectionString) + 1; 
  return p;
}

static int getSectionLength(char * pBuf, char * szSection)
{
  int iRet = 0;

  char * pSectionStart = goToSectionBody(pBuf, szSection);

  if(pSectionStart == NULL)
    return 0;

  char * pNextSection = strstr(pSectionStart, "[");

  if(pNextSection != NULL)
    iRet = (int)(pNextSection - pSectionStart);
  else
    iRet = (int)(pBuf + strlen(pBuf) - pSectionStart);

  return iRet;
}

static char * goToKeyBody(char * pSectionStart, int iSectionLength, char * szKey)
{
  if((pSectionStart == NULL) || (szKey == NULL))
    return NULL;
  if((int)strlen(szKey) >= iSectionLength)
    return NULL;
  char szKeyString[256];
  strcpy(szKeyString, szKey);
  strcat(szKeyString, "=");
  char chBackup = pSectionStart[iSectionLength];
  pSectionStart[iSectionLength] = '\0';
  char * p = strstr(pSectionStart, szKeyString);
  if(p == NULL)
  {
    pSectionStart[iSectionLength] = chBackup;
    return NULL;
  }
  p = strstr(p, "=");
  if(p == NULL)
  {
    pSectionStart[iSectionLength] = chBackup;
    return NULL;
  }
  p++;
  pSectionStart[iSectionLength] = chBackup;
  if(isCurrentLineCommentedOut(pSectionStart, p))
    return NULL;
  else
    return p;
}

static int getKeyLength(char * pSectionStart, int iSectionLength, char * szKey)
{
  int iRet = 0;

  char * pKeyStart = goToKeyBody(pSectionStart, iSectionLength, szKey);

  if(pKeyStart == NULL)
    return 0;

  char * pFirstCarriageReturn = strchr(pKeyStart, '\r');
  char * pFirstNewLine = strchr(pKeyStart, '\n');

  if((pFirstCarriageReturn == NULL) && (pFirstNewLine == NULL))
    return iSectionLength;

  char * p = NULL;

  if((pFirstCarriageReturn != NULL) && (pFirstNewLine != NULL))
    p = (pFirstCarriageReturn < pFirstNewLine) ? pFirstCarriageReturn : pFirstNewLine;
  else
    p = (pFirstCarriageReturn != NULL) ? pFirstCarriageReturn : pFirstNewLine;

  p--;

  
  while((*p == ' ') || (*p == '\t'))
    p--;

  p++;

  iRet = (int)(p - pKeyStart);

  return iRet;
}

static char * readFileToNewAllocatedZeroTerminatedBuffer(XP_HFILE hFile)
{
  if(hFile == NULL)
    return NULL;

  int iSize;
  
#ifdef XP_MAC
  iSize = 32000;  
#else  
  #if defined(XP_UNIX) || defined(XP_OS2)
    struct stat buf;
    fstat(fileno(hFile), &buf);
    iSize = buf.st_size;
  #else
    iSize = (int)_filelength((hFile));
  #endif
#endif

  if(iSize <= 0)
    return NULL;

  char * pBuf = new char[iSize + 1];
  if(pBuf == NULL)
    return NULL;

  fseek((FILE *)hFile, 0L, SEEK_SET);
  fread((void *)pBuf, iSize, 1, (FILE *)hFile);

  
  char * p = pBuf + iSize - 1;
  for(;;)
  {
    BOOL bNormalChar = ((int)*p > 0) && (isprint(*p) != 0);
    if(bNormalChar || (*p == '\n') || (*p == '\r') || (p < pBuf))
      break;
    p--;
  }
  p++;
  *p = '\0';

  iSize = strlen(pBuf);
  char * pBufFinal = new char[iSize + 1];
  memcpy((void *)pBufFinal, (void *)pBuf, iSize);
  pBufFinal[iSize] = '\0';
  delete [] pBuf;
  return pBufFinal;
}

DWORD PP_GetString(char * szSection, char * szKey, char * szDefault, char * szString, DWORD dwSize,  XP_HFILE hFile)
{
  char * pBuf = NULL;
  char * pSectionStart = NULL;
  int iSectionLength = 0;
  int iSize = 0;

  pBuf = readFileToNewAllocatedZeroTerminatedBuffer(hFile);

  if(pBuf == NULL)
    goto ReturnDefault;

  iSize = strlen(pBuf);

  
  
  
  if(szSection == NULL) 
  {
    char * pNextSection = NULL;
    char * pSource = pBuf;
    char * pDest = szString;
    int iBytes = 0;

    for(;;)
    {
      pSource = strstr(pSource, "[");
      if(pSource == NULL)
        break;

      pNextSection = pSource + 1;
      char * pEnd = strstr(pSource, "]");
      int iLength = pEnd - pNextSection;

      if(!isCurrentLineCommentedOut(pBuf, pSource))
      {
        if(iBytes + iLength + 2 < (int)dwSize)
          memcpy((void *)pDest, (void *)pNextSection, iLength);
        else
          break;

        pDest += iLength;
        *pDest = '\0';
        pDest++;
        iBytes += iLength + 1;  
        if(iBytes >= iSize)
          break;
      }
      pSource += iLength + 2; 
    }

    *pDest = '\0';
    iBytes++; 
    delete [] pBuf;
    return (DWORD)iBytes;
  }

  pSectionStart = goToSectionBody(pBuf, szSection);

  if(pSectionStart == NULL)
    goto ReturnDefault;

  iSectionLength = getSectionLength(pBuf, szSection);

  if(iSectionLength == 0)
    goto ReturnDefault;

  if(szKey == NULL) 
  {
    int iBytes = 0;
    char * pSource = pSectionStart - 1; 
    char * pDest = szString;

    for(;;)
    {
      char * pEqualSign = strstr(pSource, "=");

      if(pEqualSign > pSectionStart + iSectionLength)
        break;

      *pEqualSign = '\0';

      char * pLastCarriageReturn = strrchr(pSource, '\r');
      char * pLastNewLine = strrchr(pSource, '\n');

      if((pLastCarriageReturn == NULL) && (pLastNewLine == NULL))
      {
        pSource = pEqualSign + 1;
        continue;
      }

      char * pKey = (pLastCarriageReturn > pLastNewLine) ? pLastCarriageReturn : pLastNewLine;
      pKey++;
      int iLength = strlen(pKey);

      if(!isCurrentLineCommentedOut(pBuf, pKey))
      {
        if(iBytes + iLength + 2 < (int)dwSize)
          memcpy((void *)pDest, (void *)pKey, iLength);
        else
          break;

        pDest += iLength;
        *pDest = '\0';
        pDest++;
        iBytes += iLength + 1; 
        if(iBytes >= iSize)
          break;
      }
      pSource = pEqualSign + 1;
    }

    *pDest = '\0';
    iBytes++; 
    delete [] pBuf;
    return (DWORD)iBytes;
  }

  
  
  
  {
    char * pKeyStart = goToKeyBody(pSectionStart, iSectionLength, szKey);
    if(pKeyStart == NULL)
      goto ReturnDefault;
    int iKeyLength = getKeyLength(pSectionStart, iSectionLength, szKey);
    int iLength = (iKeyLength < (int)dwSize - 1) ? iKeyLength : (int)dwSize - 1;
    memcpy((void *)szString, (void *)pKeyStart, iLength);
    szString[iLength] = '\0';
    delete [] pBuf;
    return (DWORD)iLength;
  }

ReturnDefault:

  if(pBuf != NULL)
    delete [] pBuf;
  if((szDefault != NULL) && (szString != NULL))
    strcpy(szString, szDefault);
  return (DWORD)strlen(szString);
}

static BOOL replaceBytesInReallocatedBufferAfter(
                 char ** ppBuf,       
                 char * szString,     
                 int iPosition,       
                 int iBytesToReplace) 
{
  if(ppBuf == NULL)
    return FALSE;
  char * pBuf = *ppBuf;
  if(pBuf == NULL)
    return FALSE;
  
  int iNewLength = strlen(pBuf) - iBytesToReplace + strlen(szString);
  char * pBufNew = new char[iNewLength + 1]; 
  if(pBufNew == NULL)
    return FALSE;
  if(iPosition > 0)
    memcpy((void *)pBufNew, (void *)pBuf, iPosition);
  char * p = pBufNew + iPosition;
  memcpy((void *)p, (void *)szString, strlen(szString));
  p += strlen(szString);
  memcpy((void *)p, (void *)(pBuf + iPosition + iBytesToReplace), strlen(pBuf) - iPosition - iBytesToReplace);
  p += strlen(pBuf) - iPosition - iBytesToReplace;
  *p = '\0';
  delete [] pBuf;
  *ppBuf = pBufNew;
  return TRUE;
}

BOOL PP_WriteString(char * szSection, char * szKey, char * szString, XP_HFILE hFile)
{
  if((szSection == NULL) || (szKey == NULL) || (szString == NULL) || (hFile == NULL))
    return FALSE;

  char * pBuf = readFileToNewAllocatedZeroTerminatedBuffer(hFile);

  if(pBuf == NULL)
    return FALSE;

#if !(defined XP_MAC || defined XP_UNIX || defined XP_OS2)
  long lOldSize = (long)strlen(pBuf);
#endif

  char * pSectionStart = goToSectionBody(pBuf, szSection);

  
  if(pSectionStart == NULL)
  {
    char szNewSection[256];
    strcpy(szNewSection, "[");
    strcat(szNewSection, szSection);
    strcat(szNewSection, "]");
    strcat(szNewSection, "\n");
    strcat(szNewSection, "\n");
    if(!replaceBytesInReallocatedBufferAfter(&pBuf, szNewSection, 0, 0))
    {
      if(pBuf != NULL)
        delete [] pBuf;
      return FALSE;
    }
    pSectionStart = goToSectionBody(pBuf, szSection);
  }

  if(pSectionStart == NULL)
    return FALSE;

  int iSectionLength = getSectionLength(pBuf, szSection);
  char * pKeyStart = goToKeyBody(pSectionStart, iSectionLength, szKey);

  
  if(pKeyStart == NULL)
  {
    char szNewKey[256];
    strcpy(szNewKey, szKey);
    strcat(szNewKey, "=");
    strcat(szNewKey, "\n");
    int iPos = pSectionStart - pBuf;
    if(!replaceBytesInReallocatedBufferAfter(&pBuf, szNewKey, iPos, 0))
    {
      if(pBuf != NULL)
        delete [] pBuf;
      return FALSE;
    }
    pSectionStart = goToSectionBody(pBuf, szSection);
    iSectionLength = getSectionLength(pBuf, szSection);
    pKeyStart = goToKeyBody(pSectionStart, iSectionLength, szKey);
  }

  if(pKeyStart == NULL)
    return FALSE;

  int iKeyLength = getKeyLength(pSectionStart, iSectionLength, szKey);
  int iPos = pKeyStart - pBuf;
  if(!replaceBytesInReallocatedBufferAfter(&pBuf, szString, iPos, iKeyLength))
  {
    if(pBuf != NULL)
      delete [] pBuf;
    return FALSE;
  }
  
#if (defined XP_MAC || defined XP_UNIX || defined XP_OS2)
	rewind(hFile);
#else
  
  long lNewSize = (long)strlen(pBuf);
  long lSizeChange = lNewSize - lOldSize;
  if(lSizeChange != 0L)
  {
    long lOldFileSize = (long)_filelength((hFile));
    long lNewFileSize = lOldFileSize + lSizeChange;
    _chsize((hFile), lNewFileSize);
  }

  fseek((FILE *)hFile, 0L, SEEK_SET);
#endif

  fwrite(pBuf, strlen(pBuf), 1, (FILE *)hFile);

  delete [] pBuf;

  return TRUE;
}
