







































#include "nsINIParser.h"

nsINIParser::nsINIParser(char *aFilename)
{
    FILE    *fd = NULL;
    long    eofpos = 0;
    int     rd = 0;

    mFileBuf = NULL;
    mFileBufSize = 0;
    mError = OK;
    DUMP("nsINIParser");

    
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

    
    mFileBuf = (char *) malloc((eofpos+1) * sizeof(char));
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
    mFileBuf[mFileBufSize] = '\0';

    
    fclose(fd);

    return;

bail:
    mError = E_READ;
    return;
}

nsINIParser::~nsINIParser()
{
    DUMP("~nsINIParser");
}

int
nsINIParser::GetString( char *aSection, char *aKey, 
                        char *aValBuf, int *aIOValBufSize )
{
    char *secPtr = NULL;
    mError = OK;
    DUMP("GetString");

    
    if ( !aSection || !aKey || !aValBuf || 
         !aIOValBufSize || (*aIOValBufSize <= 0) )
        return E_PARAM;

    
    mError = FindSection(aSection, &secPtr);
    if (mError != OK)
        return mError;

    
    mError = FindKey(secPtr, aKey, aValBuf, aIOValBufSize);

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
nsINIParser::FindSection(char *aSection, char **aOutSecPtr)
{
    char *currChar = mFileBuf;
    char *nextSec = NULL;
    char *secClose = NULL;
    char *nextNL = NULL;
    int aSectionLen = strlen(aSection);
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

        
        if (strncmp(aSection, currChar, aSectionLen) == 0
              && secClose-currChar == aSectionLen)
        {
            *aOutSecPtr = secClose + 1;
            mError = OK;
            break;
        }
    }

    return mError;
}

int
nsINIParser::FindKey(char *aSecPtr, char *aKey, char *aVal, int *aIOValSize)
{
    char *nextNL = NULL;
    char *secEnd = NULL;
    char *currLine = aSecPtr;
    char *nextEq = NULL;
    int  aKeyLen = strlen(aKey); 
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

        
        if (strncmp(currLine, aKey, aKeyLen) == 0
              && nextEq-currLine == aKeyLen)
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
