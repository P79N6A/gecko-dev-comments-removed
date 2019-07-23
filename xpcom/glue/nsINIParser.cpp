







































#include "nsINIParser.h"
#include "nsError.h"
#include "nsILocalFile.h"
#include "nsCRTGlue.h"

#include <stdlib.h>
#include <stdio.h>

#if defined(XP_WIN) || defined(XP_OS2)
#define BINARY_MODE "b"
#else
#define BINARY_MODE
#endif




class AutoFILE {
public:
  AutoFILE(FILE *fp = nsnull) : fp_(fp) {}
  ~AutoFILE() { if (fp_) fclose(fp_); }
  operator FILE *() { return fp_; }
  FILE** operator &() { return &fp_; }
private:
  FILE *fp_;
};

nsresult
nsINIParser::Init(nsILocalFile* aFile)
{
    nsresult rv;

    



    AutoFILE fd;

#ifdef XP_WIN
    nsAutoString path;
    rv = aFile->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    fd = _wfopen(path.get(), L"rb");
#else
    nsCAutoString path;
    rv = aFile->GetNativePath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    fd = fopen(path.get(), "r" BINARY_MODE);
#endif
    if (!fd)
      return NS_ERROR_FAILURE;

    return InitFromFILE(fd);
}

nsresult
nsINIParser::Init(const char *aPath)
{
    
    AutoFILE fd = fopen(aPath, "r" BINARY_MODE);
    if (!fd)
        return NS_ERROR_FAILURE;

    return InitFromFILE(fd);
}

static const char kNL[] = "\r\n";
static const char kEquals[] = "=";
static const char kWhitespace[] = " \t";
static const char kRBracket[] = "]";

nsresult
nsINIParser::InitFromFILE(FILE *fd)
{
    if (!mSections.Init())
        return NS_ERROR_OUT_OF_MEMORY;

    
    if (fseek(fd, 0, SEEK_END) != 0)
        return NS_ERROR_FAILURE;

    long flen = ftell(fd);
    if (flen == 0)
        return NS_ERROR_FAILURE;

    
    mFileContents = new char[flen + 1];
    if (!mFileContents)
        return NS_ERROR_OUT_OF_MEMORY;

    
    if (fseek(fd, 0, SEEK_SET) != 0)
        return NS_BASE_STREAM_OSERROR;

    int rd = fread(mFileContents, sizeof(char), flen, fd);
    if (rd != flen)
        return NS_BASE_STREAM_OSERROR;

    mFileContents[flen] = '\0';

    char *buffer = mFileContents;
    char *currSection = nsnull;
    INIValue *last = nsnull;

    
    while (char *token = NS_strtok(kNL, &buffer)) {
        if (token[0] == '#' || token[0] == ';') 
            continue;

        token = (char*) NS_strspnp(kWhitespace, token);
        if (!*token) 
            continue;

        if (token[0] == '[') { 
            ++token;
            currSection = token;
            last = nsnull;

            char *rb = NS_strtok(kRBracket, &token);
            if (!rb || NS_strtok(kWhitespace, &token)) {
                
                
                
                
                currSection = nsnull;
            }

            continue;
        }

        if (!currSection) {
            
            
            continue;
        }

        char *key = token;
        char *e = NS_strtok(kEquals, &token);
        if (!e)
            continue;

        INIValue *val = new INIValue(key, token);
        if (!val)
            return NS_ERROR_OUT_OF_MEMORY;

        
        
        if (!last) {
            mSections.Get(currSection, &last);
            while (last && last->next)
                last = last->next;
        }

        if (last) {
            

            last->next = val;
            last = val;
            continue;
        }

        
        mSections.Put(currSection, val);
    }

    return NS_OK;
}

nsresult
nsINIParser::GetString(const char *aSection, const char *aKey, 
                       nsACString &aResult)
{
    INIValue *val;
    mSections.Get(aSection, &val);

    while (val) {
        if (strcmp(val->key, aKey) == 0) {
            aResult.Assign(val->value);
            return NS_OK;
        }

        val = val->next;
    }

    return NS_ERROR_FAILURE;
}

nsresult
nsINIParser::GetString(const char *aSection, const char *aKey, 
                       char *aResult, PRUint32 aResultLen)
{
    INIValue *val;
    mSections.Get(aSection, &val);

    while (val) {
        if (strcmp(val->key, aKey) == 0) {
            strncpy(aResult, val->value, aResultLen);
            aResult[aResultLen - 1] = '\0';
            if (strlen(val->value) >= aResultLen)
                return NS_ERROR_LOSS_OF_SIGNIFICANT_DATA;

            return NS_OK;
        }

        val = val->next;
    }

    return NS_ERROR_FAILURE;
}

PLDHashOperator
nsINIParser::GetSectionsCB(const char *aKey, INIValue *aData,
                           void *aClosure)
{
    GSClosureStruct *cs = NS_REINTERPRET_CAST(GSClosureStruct*, aClosure);

    return cs->usercb(aKey, cs->userclosure) ? PL_DHASH_NEXT : PL_DHASH_STOP;
}

nsresult
nsINIParser::GetSections(INISectionCallback aCB, void *aClosure)
{
    GSClosureStruct gs = {
        aCB,
        aClosure
    };

    mSections.EnumerateRead(GetSectionsCB, &gs);
    return NS_OK;
}

nsresult
nsINIParser::GetStrings(const char *aSection,
                        INIStringCallback aCB, void *aClosure)
{
    INIValue *val;

    for (mSections.Get(aSection, &val);
         val;
         val = val->next) {

        if (!aCB(val->key, val->value, aClosure))
            return NS_OK;
    }

    return NS_OK;
}
