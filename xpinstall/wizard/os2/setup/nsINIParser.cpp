







































#include "nsINIParser.h"

#ifdef __cplusplus
extern "C" {
#endif
unsigned long GetPrivateProfileString(const char* szAppName,
                                      const char* szKeyName,
                                      const char* szDefault,
                                      char* szReturnedString,
                                      int nSize,
                                      const char* szFileName)
{
   nsINIParser parser((char*)szFileName);
   if (parser.GetString((char*)szAppName, (char*)szKeyName, szReturnedString, &nSize) != nsINIParser::OK) {
      if (szDefault) {
         strcpy(szReturnedString, szDefault);
      }
   }
   return strlen(szReturnedString);
}

unsigned long WritePrivateProfileString(const char* szAppName,
                                        const char* szKeyName,
                                        const char* szValue,
                                        const char* szFileName)
{
   nsINIParser parser((char*)szFileName);
   if (parser.WriteString((char*)szAppName, (char*)szKeyName, (char*)szValue) != nsINIParser::OK) {
      return 0;
   }
   return 1;
}
#ifdef __cplusplus
}
#endif

nsINIParser::nsINIParser(char *aFilename)
{
    FILE    *fd = NULL;
    long    eofpos = 0;
    int     rd = 0;

    mfWrite = 0;
    mFileBuf = NULL;
    mFilename = (char*)malloc(strlen(aFilename)+1);
    strcpy(mFilename, aFilename);
    mFileBufSize = 0;
    mError = OK;

    
    if (!aFilename)
    {
        mError = E_PARAM;
        return;
    }

    
    fd = fopen(aFilename, "r");
    if (!fd)
        goto bail;
    
    
    if (fseek(fd, 0, SEEK_END) != 0)
        goto bail;
    eofpos = ftell(fd);
    if (eofpos == 0)
        goto bail;

    
    mFileBuf = (char *) calloc(1, eofpos * sizeof(char));
    if (!mFileBuf)
    {
        mError = E_MEM;
        return;
    }
    mFileBufSize = eofpos;

    
    if (fseek(fd, 0, SEEK_SET) != 0)
        goto bail;
    rd = fread((void *)mFileBuf, 1, eofpos, fd);
    if (!rd)
        goto bail;

    
    
    if (rd < eofpos) {
      mFileBufSize = rd;
    } else {
      mFileBufSize = eofpos;
    }

    
    fclose(fd);

    return;

bail:
    mError = E_READ;
    return;
}

nsINIParser::~nsINIParser()
{
    if (mfWrite) {
      FILE* hFile = fopen(mFilename, "w+");
      fwrite(mFileBuf, mFileBufSize, 1, (FILE *)hFile);
      fclose(hFile);
    }
    free(mFileBuf);
    free(mFilename);
}

int
nsINIParser::GetString( char *aSection, char *aKey, 
                        char *aValBuf, int *aIOValBufSize )
{
    char *secPtr = NULL;
    mError = OK;
    DUMP("GetString");

    
    if ( !aSection || !aValBuf || 
         !aIOValBufSize || (*aIOValBufSize <= 0) )
        return E_PARAM;

    
    mError = FindSection(aSection, &secPtr);
    if (mError != OK)
        return mError;

    if (aKey) {
        
        mError = GetValue(secPtr, aKey, aValBuf, aIOValBufSize);
    } else {
        mError = GetAllKeys(secPtr, aValBuf, aIOValBufSize);
    }


    return mError;
}

int
nsINIParser::GetStringAlloc( char *aSection, char *aKey,
                             char **aOutBuf, int *aOutBufSize )
{
    char buf[MAX_VAL_SIZE];
    int bufsize = MAX_VAL_SIZE;
    mError = OK;
    DUMP("GetStringAlloc");

    mError = GetString(aSection, aKey, buf, &bufsize);
    if (mError != OK)
        return mError;

    *aOutBuf = (char *) malloc(bufsize + 1);
    strncpy(*aOutBuf, buf, bufsize);
    *(*aOutBuf + bufsize) = 0;
    *aOutBufSize = bufsize + 1;

    return mError;
}

int
nsINIParser::GetError()
{
    DUMP("GetError");
    return mError;
}

char *
nsINIParser::ResolveName(char *aINIRoot)
{
    char *resolved = NULL;
    char *locale = NULL;
    struct stat st_exists;

    
    if (!aINIRoot)
        return NULL;

    locale = setlocale(LC_CTYPE, NULL);
    if (!locale) 
        return NULL;

    
    resolved = (char *) malloc(strlen(aINIRoot) + 5 + strlen(locale) + 1);
    if (!resolved)
        return NULL;

    
    sprintf(resolved, "%s.ini.%s", aINIRoot, locale);
    if (0 == stat(resolved, &st_exists))
        return resolved;

    
    sprintf(resolved, "%s.ini", aINIRoot);
    if (0 == stat(resolved, &st_exists))
        return resolved;
    
    
    return NULL;
}

int
nsINIParser::WriteString(char *aSection, char *aKey, char *aValBuf)
{
    mfWrite = 1;
    char *secPtr = NULL;
    char *keyPtr = NULL;
    mError = OK;
    DUMP("GetString");

    
    if ( !aSection || !aKey || !aValBuf )
        return E_PARAM;

    
    mError = FindSection(aSection, &secPtr);
    if (mError != OK) {
      
      char szNewSection[MAX_VAL_SIZE];
      strcpy(szNewSection, "[");
      strcat(szNewSection, aSection);
      strcat(szNewSection, "]");
      strcat(szNewSection, NLSTRING);
      mFileBuf = (char*)realloc(mFileBuf, mFileBufSize+strlen(szNewSection));
      memcpy(&mFileBuf[mFileBufSize], szNewSection, strlen(szNewSection));
      secPtr = &mFileBuf[mFileBufSize];
      mFileBufSize+= strlen(szNewSection);
      mError = OK;
    }

    
    mError = FindKey(secPtr, aKey, &keyPtr);
    if (mError != OK) {
      char szNewKeyValue[MAX_VAL_SIZE];
      strcpy(szNewKeyValue, aKey);
      strcat(szNewKeyValue, "=");
      strcat(szNewKeyValue, aValBuf);
      strcat(szNewKeyValue, NLSTRING);
      char *mNewFileBuf = (char*)calloc(1, mFileBufSize+strlen(szNewKeyValue));
      memcpy(mNewFileBuf, mFileBuf, mFileBufSize);
      
      while (*secPtr != NL) {
         secPtr++;
      }
      secPtr++;
      
      memcpy(mNewFileBuf+(secPtr-mFileBuf), szNewKeyValue, strlen(szNewKeyValue));
      memcpy(mNewFileBuf+(secPtr-mFileBuf)+strlen(szNewKeyValue), secPtr, mFileBufSize-(secPtr-mFileBuf));
      mFileBufSize += strlen(szNewKeyValue);
      free(mFileBuf);
      mFileBuf = mNewFileBuf;
      mError = OK;
    } else {
      while (*keyPtr != '=') {
         keyPtr++;
      }
      keyPtr++;
      int length = 0;
      char* lenPtr = keyPtr;
      while (*lenPtr != NL) {
         lenPtr++;
         length++;
      }
      int difference;
      difference = strlen(aValBuf)- length;
      char *mNewFileBuf = (char*)calloc(1, mFileBufSize+difference);
      memcpy(mNewFileBuf, mFileBuf, mFileBufSize);
      
      memcpy(mNewFileBuf+(keyPtr-mFileBuf), aValBuf, strlen(aValBuf));


      memcpy(mNewFileBuf+(keyPtr-mFileBuf)+strlen(aValBuf), lenPtr, mFileBufSize-(lenPtr-mFileBuf));
      mFileBufSize += difference;
      free(mFileBuf);
      mFileBuf = mNewFileBuf;
      mError = OK;


    }

    return mError;
}

int
nsINIParser::FindSection(char *aSection, char **aOutSecPtr)
{
    char *currChar = mFileBuf;
    char *nextSec = NULL;
    char *secClose = NULL;
    char *nextNL = NULL;
    mError = E_NO_SEC;
    DUMP("FindSection");

    
    if (!aSection || !aOutSecPtr)
    {
        mError = E_PARAM;
        return mError;
    }

    while (currChar < (mFileBuf + mFileBufSize))
    {
        
        nextSec = NULL;
        nextSec = strchr(currChar, '[');
        if (!nextSec)
            break;
            
        currChar = nextSec + 1;

        
        secClose = NULL; nextNL = NULL;
        secClose = strchr(currChar, ']');
        nextNL = strchr(currChar, NL);
        if ((!nextNL) || (nextNL < secClose))
        {
            currChar = nextNL;
            continue;
        }

        
        if (strnicmp(aSection, currChar, strlen(aSection)) == 0)
        {
            *aOutSecPtr = secClose + 1;
            mError = OK;
            break;
        }
    }

    return mError;
}

int
nsINIParser::GetValue(char *aSecPtr, char *aKey, char *aVal, int *aIOValSize)
{
    char *nextNL = NULL;
    char *secEnd = NULL;
    char *currLine = aSecPtr;
    char *nextEq = NULL;
    mError = E_NO_KEY;
    DUMP("FindKey");

    
    if (!aSecPtr || !aKey || !aVal || !aIOValSize || (*aIOValSize <= 0))
    {
        mError = E_PARAM;
        return mError;
    }

    
    secEnd = aSecPtr;
find_end:
    if (secEnd)
        secEnd = strchr(secEnd, '['); 
    if (!secEnd)
    {
        secEnd = strchr(aSecPtr, '\0'); 
        if (!secEnd)
        {
            mError = E_SEC_CORRUPT; 
            return mError;
        }
    }

    
    if (*secEnd == '[' && !(secEnd == aSecPtr || *(secEnd-1) == NL))
    {
        secEnd++;
        goto find_end;
    }

    while (currLine < secEnd)
    {
        nextNL = NULL;
        nextNL = strchr(currLine, NL);
        if (!nextNL)
            nextNL = mFileBuf + mFileBufSize;

        
        if (currLine == strchr(currLine, ';'))
        {
            currLine = nextNL + 1;
            continue;
        }

        
        nextEq = NULL;
        nextEq = strchr(currLine, '=');
        if (!nextEq || nextEq > nextNL) 
        {
            currLine = nextNL + 1;
            continue;
        }

        
        if (strnicmp(currLine, aKey, strlen(aKey)) == 0)
        {
            
            if (*aIOValSize < nextNL - nextEq)
            {
                mError = E_SMALL_BUF;
                *aVal = '\0';
                *aIOValSize = 0;
                return mError;
            }
                
            *aIOValSize = nextNL - (nextEq + 1); 
            strncpy(aVal, (nextEq + 1), *aIOValSize);
            *(aVal + *aIOValSize) = 0; 
            mError = OK;
            return mError;
        }
        else
        {
            currLine = nextNL + 1;
        }
    }

    return mError;
}

int
nsINIParser::GetAllKeys(char *aSecPtr, char *aVal, int *aIOValSize)
{
    char *nextNL = NULL;
    char *secEnd = NULL;
    char *currLine = aSecPtr;
    char *nextEq = NULL;
    char *outPtr = NULL;
    mError = E_NO_KEY;
    DUMP("FindKey");

    
    if (!aSecPtr || !aVal || !aIOValSize || (*aIOValSize <= 0))
    {
        mError = E_PARAM;
        return mError;
    }

    
    secEnd = aSecPtr;
find_end:
    if (secEnd)
        secEnd = strchr(secEnd, '['); 
    if (!secEnd)
    {
        secEnd = strchr(aSecPtr, '\0'); 
        if (!secEnd)
        {
            mError = E_SEC_CORRUPT; 
            return mError;
        }
    }

    
    if (*secEnd == '[' && !(secEnd == aSecPtr || *(secEnd-1) == NL))
    {
        secEnd++;
        goto find_end;
    }

    outPtr = aVal;

    while (currLine < secEnd)
    {
        nextNL = NULL;
        nextNL = strchr(currLine, NL);
        if (!nextNL)
            nextNL = mFileBuf + mFileBufSize;

        
        if (currLine == strchr(currLine, ';'))
        {
            currLine = nextNL + 1;
            continue;
        }

        
        nextEq = NULL;
        nextEq = strchr(currLine, '=');
        if (!nextEq || nextEq > nextNL) 
        {
            currLine = nextNL + 1;
            continue;
        }

        strncpy(aVal, currLine, nextEq-currLine);
        aVal+= nextEq-currLine;
        *aVal = '\0';
        aVal++;

        currLine = nextNL + 1;
    }

    return OK;
}

int
nsINIParser::FindKey(char *aSecPtr, char *aKey, char **aOutKeyPtr)
{
    char *nextNL = NULL;
    char *secEnd = NULL;
    char *currLine = aSecPtr;
    char *nextEq = NULL;
    mError = E_NO_KEY;
    DUMP("FindKey");

    
    if (!aSecPtr || !aKey || !aOutKeyPtr)
    {
        mError = E_PARAM;
        return mError;
    }

    
    secEnd = aSecPtr;
find_end:
    if (secEnd)
        secEnd = strchr(secEnd, '['); 
    if (!secEnd)
    {
        secEnd = strchr(aSecPtr, '\0'); 
        if (!secEnd)
        {
            mError = E_SEC_CORRUPT; 
            return mError;
        }
    }

    
    if (*secEnd == '[' && !(secEnd == aSecPtr || *(secEnd-1) == NL))
    {
        secEnd++;
        goto find_end;
    }

    while (currLine < secEnd)
    {
        nextNL = NULL;
        nextNL = strchr(currLine, NL);
        if (!nextNL)
            nextNL = mFileBuf + mFileBufSize;

        
        if (currLine == strchr(currLine, ';'))
        {
            currLine = nextNL + 1;
            continue;
        }

        
        nextEq = NULL;
        nextEq = strchr(currLine, '=');
        if (!nextEq || nextEq > nextNL) 
        {
            currLine = nextNL + 1;
            continue;
        }

        
        if (strnicmp(currLine, aKey, strlen(aKey)) == 0)
        {
            *aOutKeyPtr = currLine;
            mError = OK;
            return mError;
        }
        else
        {
            currLine = nextNL + 1;
        }
    }

    return mError;
}
