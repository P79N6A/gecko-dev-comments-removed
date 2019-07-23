




































#ifndef nsPluginManifestLineReader_h__
#define nsPluginManifestLineReader_h__

#include "nspr.h"
#include "nsDebug.h"

#ifdef XP_WIN
#define PLUGIN_REGISTRY_FIELD_DELIMITER '|'
#else
#define PLUGIN_REGISTRY_FIELD_DELIMITER ':'
#endif

#define PLUGIN_REGISTRY_END_OF_LINE_MARKER '$'

class nsPluginManifestLineReader
{
public:
    nsPluginManifestLineReader() {mBase = mCur = mNext = mLimit = 0;} 
    ~nsPluginManifestLineReader() { if (mBase) delete[] mBase; mBase=0;}

    char* Init(PRUint32 flen) 
    {
        mBase = mCur = mNext = new char[flen+1];
        if (mBase) {
          mLimit = mBase + flen;
          *mLimit = 0;
        }
        mLength = 0;
        return mBase;
    }

    PRBool NextLine()
    {
        if(mNext >= mLimit)
            return PR_FALSE;
        
        mCur = mNext;
        mLength = 0;
        
        char *lastDelimiter = 0;
        while(mNext < mLimit)
        {
            if(IsEOL(*mNext))
            {
                if(lastDelimiter)
                {
                    if(lastDelimiter && *(mNext - 1) != PLUGIN_REGISTRY_END_OF_LINE_MARKER)
                        return PR_FALSE;
                    *lastDelimiter = '\0';
                } else
                    *mNext = '\0';

                for(++mNext; mNext < mLimit; ++mNext)
                    if(!IsEOL(*mNext))
                        break;
                return PR_TRUE;
            }
            if(*mNext == PLUGIN_REGISTRY_FIELD_DELIMITER)
                lastDelimiter = mNext;
            ++mNext;
            ++mLength;
        }
        return PR_FALSE;        
    }

    int ParseLine(char** chunks, int maxChunks)
    {
        NS_ASSERTION(mCur && maxChunks && chunks, "bad call to ParseLine");
        int found = 0;
        chunks[found++] = mCur;
        
        if(found < maxChunks)
        {
            for(char* cur = mCur; *cur; cur++)
            {
                if(*cur == PLUGIN_REGISTRY_FIELD_DELIMITER)
                {
                    *cur = 0;
                    chunks[found++] = cur+1;
                    if(found == maxChunks)
                        break;
                }
            }
        }
        return found;
    }

    char*       LinePtr() {return mCur;}    
    PRUint32    LineLength() {return mLength;}    

    PRBool      IsEOL(char c) {return c == '\n' || c == '\r';}

    char*       mBase;
private:
    char*       mCur;
    PRUint32    mLength;
    char*       mNext;
    char*       mLimit;
};

#endif
