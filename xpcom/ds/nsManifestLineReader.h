




































#ifndef nsManifestLineReader_h__
#define nsManifestLineReader_h__

#include "nspr.h"
#include "nsDebug.h"

class nsManifestLineReader
{
public:
    nsManifestLineReader() : mBase(nsnull) {} 
    ~nsManifestLineReader() {}

    void Init(char* base, PRUint32 flen) 
    {
        mBase = mCur = mNext = base; 
        mLength = 0;
        mLimit = base + flen;
    }

    PRBool NextLine()
    {
        if(mNext >= mLimit)
            return PR_FALSE;
        
        mCur = mNext;
        mLength = 0;
        
        while(mNext < mLimit)
        {
            if(IsEOL(*mNext))
            {
                *mNext = '\0';
                for(++mNext; mNext < mLimit; ++mNext)
                    if(!IsEOL(*mNext))
                        break;
                return PR_TRUE;
            }
            ++mNext;
            ++mLength;
        }
        return PR_FALSE;        
    }

    int ParseLine(char** chunks, int* lengths, int maxChunks)
    {
        NS_ASSERTION(mCur && maxChunks && chunks, "bad call to ParseLine");
        int found = 0;
        chunks[found++] = mCur;

        if(found < maxChunks)
        {
            char *lastchunk = mCur;
            int *lastlength = lengths;
            for(char* cur = mCur; *cur; cur++)
            {
                if(*cur == ',')
                {
                    *cur = 0;
                    
                    *lastlength++ = cur - lastchunk;
                    chunks[found++] = lastchunk = cur+1;
                    if(found == maxChunks)
                        break;
                }
            }
            
            *lastlength = (mCur + mLength) - lastchunk;
        }
        return found;
    }

    char*       LinePtr() {return mCur;}    
    PRUint32    LineLength() {return mLength;}    

    PRBool      IsEOL(char c) {return c == '\n' || c == '\r';}
private:
    char*       mCur;
    PRUint32    mLength;
    char*       mNext;
    char*       mBase;
    char*       mLimit;
};

#endif
