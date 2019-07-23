



































 
#include "nsFileSpec.h"

#include "nsDebug.h"
#include "nsEscape.h"
#include "nsMemory.h"

#include "prtypes.h"
#include "plstr.h"
#include "plbase64.h"
#include "prmem.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsILocalFile.h"

#include <string.h>
#include <stdio.h>

#if defined(XP_WIN)
#include <mbstring.h>
#endif

#ifdef XP_OS2
extern unsigned char* _mbsrchr( const unsigned char*, int);
#endif


static inline char *GetLastSeparator(const char *str, char sep)
{
#if defined(XP_WIN) || defined(XP_OS2)
    return (char*) _mbsrchr((const unsigned char *) str, sep);
#else
    return (char*) strrchr(str, sep);
#endif
}

#ifdef XP_MACOSX
#include <sys/stat.h>
#include <Aliases.h>
#include <TextUtils.h>
#endif






nsSimpleCharString::nsSimpleCharString()

:   mData(nsnull)
{
    
} 


nsSimpleCharString::nsSimpleCharString(const char* inString)

:   mData(nsnull)
{
    if (inString)
        CopyFrom(inString, strlen(inString));
} 


nsSimpleCharString::nsSimpleCharString(const nsString& inString)

:   mData(nsnull)
{
    *this = inString;
} 


nsSimpleCharString::nsSimpleCharString(const nsSimpleCharString& inOther)

{
    mData = inOther.mData;
    AddRefData();
} 


nsSimpleCharString::nsSimpleCharString(const char* inData, PRUint32 inLength)

:   mData(nsnull)
{
    CopyFrom(inData, inLength);
} 


nsSimpleCharString::~nsSimpleCharString()

{
    ReleaseData();
} 


void nsSimpleCharString::operator = (const char* inString)

{
    if (inString)
        CopyFrom(inString, strlen(inString));
    else
        SetToEmpty();
} 


void nsSimpleCharString::operator = (const nsString& inString)

{
    PRUint32 len = inString.Length();
    ReallocData(len);
    if (!mData)
        return;
    nsFixedCString dataString(mData->mString, len + 1);
    LossyCopyUTF16toASCII(inString, dataString);
    NS_ASSERTION(dataString.get() == mData->mString, "buffer too small");
} 


void nsSimpleCharString::operator = (const nsSimpleCharString& inOther)

{
    if (mData == inOther.mData)
        return;
    ReleaseData();
    mData = inOther.mData;
    AddRefData();
} 


void nsSimpleCharString::operator += (const char* inOther)

{
    if (!inOther)
        return;
    int newLength = Length() + strlen(inOther);
    ReallocData(newLength);
    strcat(mData->mString, inOther);
} 


nsSimpleCharString nsSimpleCharString::operator + (const char* inOther) const

{
    nsSimpleCharString result(*this);
    result += inOther;
    return result;
} 


void nsSimpleCharString::Catenate(const char* inString1, const char* inString2)

{
    if (!inString2)
    {
        *this += inString1;
        return;
    }
    int newLength = Length() + strlen(inString1) + strlen(inString2);
    ReallocData(newLength);
    strcat(mData->mString, inString1);
    strcat(mData->mString, inString2);
} 


void nsSimpleCharString::CopyFrom(const char* inData, PRUint32 inLength)

{
    if (!inData)
        return;
    ReallocData(inLength);
    if (!mData)
        return;
    if (inLength != 0) {
        memcpy(mData->mString, inData, inLength);
    }
    mData->mString[inLength] = '\0';
} 


void nsSimpleCharString::SetToEmpty()

{
    ReleaseData();
} 


void nsSimpleCharString::Unescape()

{
    if (!mData)
        return;
    ReallocData(mData->mLength);
    if (!mData)
        return;
    nsUnescape(mData->mString);
    mData->mLength = strlen(mData->mString);       
} 



void nsSimpleCharString::AddRefData()

{
    if (mData)
        ++mData->mRefCount;
} 


void nsSimpleCharString::ReleaseData()

{
    if (!mData)
        return;
    NS_ASSERTION(mData->mRefCount > 0, "String deleted too many times!");
    if (--mData->mRefCount == 0)
        PR_Free(mData);
    mData = nsnull;
} 


inline PRUint32 CalculateAllocLength(PRUint32 logicalLength)


{
    return ((1 + (logicalLength >> 8)) << 8);
}


void nsSimpleCharString::ReallocData(PRUint32 inLength)




{
    PRUint32 newAllocLength = CalculateAllocLength(inLength);
    PRUint32 oldAllocLength = CalculateAllocLength(Length());
    if (mData)
    {
        NS_ASSERTION(mData->mRefCount > 0, "String deleted too many times!");
        if (mData->mRefCount == 1)
        {
            
            if (newAllocLength > oldAllocLength)
                mData = (Data*)PR_Realloc(mData, newAllocLength + sizeof(Data));
            mData->mLength = inLength;
            mData->mString[inLength] = '\0'; 
            return;
        }
    }
    PRUint32 copyLength = Length();
    if (inLength < copyLength)
        copyLength = inLength;
    Data* newData = (Data*)PR_Malloc(newAllocLength + sizeof(Data));
    
    
    if (mData)
    {
        memcpy(newData, mData, sizeof(Data) + copyLength);
        mData->mRefCount--; 
    }
    else
        newData->mString[0] = '\0';

    mData = newData;
    mData->mRefCount = 1;
    mData->mLength = inLength;
} 



NS_NAMESPACE nsFileSpecHelpers

{
    enum
    {    kMaxFilenameLength = 31                
    ,    kMaxAltDigitLength    = 5
    ,    kMaxCoreLeafNameLength    = (kMaxFilenameLength - (kMaxAltDigitLength + 1))
    };
    NS_NAMESPACE_PROTOTYPE void Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs);
    NS_NAMESPACE_PROTOTYPE void MakeAllDirectories(const char* inPath, int mode);
#if defined(XP_WIN) || defined(XP_OS2)
    NS_NAMESPACE_PROTOTYPE void NativeToUnix(nsSimpleCharString& ioPath);
    NS_NAMESPACE_PROTOTYPE void UnixToNative(nsSimpleCharString& ioPath);
#endif
} NS_NAMESPACE_END


nsresult ns_file_convert_result(PRInt32 nativeErr)

{
    return nativeErr ?
        NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_FILES,((nativeErr)&0xFFFF))
        : NS_OK;
}


void nsSimpleCharString::LeafReplace(char inSeparator, const char* inLeafName)

{
    
    if (IsEmpty())
        return;
    if (!inLeafName)
    {
        SetToEmpty();
        return;
    }
    char* chars = mData->mString;
    char* lastSeparator = GetLastSeparator(chars, inSeparator);
    int oldLength = Length();
    PRBool trailingSeparator = (lastSeparator + 1 == chars + oldLength);
    if (trailingSeparator)
    {
        char savedCh = *lastSeparator;
        char *savedLastSeparator = lastSeparator;
        *lastSeparator = '\0';
        lastSeparator = GetLastSeparator(chars, inSeparator);
        *savedLastSeparator = savedCh;
    }
    if (lastSeparator)
        lastSeparator++; 
    else
        lastSeparator = chars; 

    PRUint32 savedLastSeparatorOffset = (lastSeparator - chars);
    int newLength =
        (lastSeparator - chars) + strlen(inLeafName) + (trailingSeparator != 0);
    ReallocData(newLength);

    chars = mData->mString; 
    chars[savedLastSeparatorOffset] = '\0'; 

    strcat(chars, inLeafName);
    if (trailingSeparator)
    {
        
        char sepStr[2] = "/";
        *sepStr = inSeparator;
        strcat(chars, sepStr);
    }
} 


char* nsSimpleCharString::GetLeaf(char inSeparator) const


{
    if (IsEmpty())
        return nsnull;

    char* chars = mData->mString;
    const char* lastSeparator = GetLastSeparator(chars, inSeparator);
    
    if (!lastSeparator)
        return nsCRT::strdup(*this);

    
    
    const char* leafPointer = lastSeparator + 1;
    if (*leafPointer)
        return nsCRT::strdup(leafPointer);

    
    *(char*)lastSeparator = '\0'; 
    leafPointer = GetLastSeparator(chars, inSeparator);
    char* result = leafPointer ? nsCRT::strdup(++leafPointer) : nsCRT::strdup(chars);
    
    *(char*)lastSeparator = inSeparator;
#if defined(XP_WIN) || defined(XP_OS2)
    
    if (!leafPointer && result[1] == '|' && result[2] == 0)
        result[1] = ':';
#endif
    return result;
} 

#if (defined(XP_UNIX) || defined(XP_WIN) || defined(XP_OS2) || defined(XP_BEOS))


void nsFileSpecHelpers::MakeAllDirectories(const char* inPath, int mode)



{
    if (!inPath)
        return;
        
    char* pathCopy = nsCRT::strdup( inPath );
    if (!pathCopy)
        return;

    const char kSeparator = '/'; 
    const int kSkipFirst = 1;

#if defined(XP_WIN) || defined(XP_OS2)
    
    
    NS_ASSERTION( pathCopy[0] != '/' || (pathCopy[1] && (pathCopy[2] == '|' || pathCopy[2] == '/')),
        "Not a UNC path and no drive letter!" );
#endif
    char* currentStart = pathCopy;
    char* currentEnd = strchr(currentStart + kSkipFirst, kSeparator);
    if (currentEnd)
    {
        nsFileSpec spec;
        *currentEnd = '\0';
        
#if defined(XP_WIN) || defined(XP_OS2)
        



        if (pathCopy[0] == '/' && pathCopy[1] && pathCopy[2] == '|')
        {
            char* startDir = (char*)PR_Malloc(strlen(pathCopy) + 2);
            strcpy(startDir, pathCopy);
            strcat(startDir, "/");

            spec = nsFilePath(startDir, PR_FALSE);
            
            PR_Free(startDir);
        }
        else
        {
            
            if (pathCopy[0] == '/' &&
                currentEnd == currentStart+kSkipFirst)
            {
                *currentEnd = '/';
                currentStart = strchr(pathCopy+2, kSeparator);
                currentStart = strchr(currentStart+1, kSeparator);
                currentEnd = strchr(currentStart+1, kSeparator);
                if (currentEnd)
                    *currentEnd = '\0';
            }
            spec = nsFilePath(pathCopy, PR_FALSE);
        }
#else
        spec = nsFilePath(pathCopy, PR_FALSE);
#endif        
        do
        {
            
            
            if (!spec.Exists() && *currentStart != kSeparator)
                spec.CreateDirectory(mode);
            
            currentStart = ++currentEnd;
            currentEnd = strchr(currentStart, kSeparator);
            if (!currentEnd)
                break;
            
            *currentEnd = '\0';

            spec += currentStart; 
        } while (currentEnd);
    }
    nsCRT::free(pathCopy);
} 

#endif 

#if defined(XP_WIN)
#include "nsFileSpecWin.cpp" 
#elif defined(XP_BEOS)
#include "nsFileSpecBeOS.cpp" 
#elif defined(XP_UNIX)
#include "nsFileSpecUnix.cpp" 
#elif defined(XP_OS2)
#include "nsFileSpecOS2.cpp" 
#endif






nsFileURL::nsFileURL(const char* inString, PRBool inCreateDirs)

{
    if (!inString)
        return;
    NS_ASSERTION(strstr(inString, kFileURLPrefix) == inString, "Not a URL!");
    
    
    
    nsSimpleCharString unescapedPath(inString + kFileURLPrefixLength);
    unescapedPath.Unescape();
    nsFilePath path(unescapedPath, inCreateDirs);
    *this = path;
} 


nsFileURL::nsFileURL(const nsString& inString, PRBool inCreateDirs)

{
    NS_LossyConvertUTF16toASCII cstring(inString);
    if (!inString.Length())
        return;
    NS_ASSERTION(strstr(cstring.get(), kFileURLPrefix) == cstring.get(),
                 "Not a URL!");
    
    
    
    nsSimpleCharString unescapedPath(cstring.get() + kFileURLPrefixLength);
    unescapedPath.Unescape();
    nsFilePath path(unescapedPath, inCreateDirs);
    *this = path;
} 


nsFileURL::nsFileURL(const nsFileURL& inOther)

: mURL(inOther.mURL)
{
} 


nsFileURL::nsFileURL(const nsFilePath& inOther)

{
    *this = inOther;
} 


nsFileURL::nsFileURL(const nsFileSpec& inOther)

{
    *this = inOther;
} 


nsFileURL::~nsFileURL()

{
}


void nsFileURL::operator = (const char* inString)

{
    
    

    mURL = inString;
    NS_ASSERTION(strstr(inString, kFileURLPrefix) == inString, "Not a URL!");
} 


void nsFileURL::operator +=(const char* inRelativeUnixPath)

{
    char* escapedPath = nsEscape(inRelativeUnixPath, url_Path);
    mURL += escapedPath;
    nsCRT::free(escapedPath);
} 


nsFileURL nsFileURL::operator +(const char* inRelativeUnixPath) const

{
   nsFileURL result(*this);
   result += inRelativeUnixPath;
   return result;
}  


void nsFileURL::operator = (const nsFileURL& inOther)

{
    mURL = inOther.mURL;
} 


void nsFileURL::operator = (const nsFilePath& inOther)

{
    mURL = kFileURLPrefix;
    char* original = (char*)(const char*)inOther; 
    if (!original || !*original) return;
#if defined(XP_WIN) || defined(XP_OS2)
    
    
    char oldchar = original[2];
    original[2] = 'x';
    char* escapedPath = nsEscape(original, url_Path);
    original[2] = escapedPath[2] = oldchar; 
#else
    char* escapedPath = nsEscape(original, url_Path);
#endif
    if (escapedPath)
        mURL += escapedPath;
    nsCRT::free(escapedPath);
} 


void nsFileURL::operator = (const nsFileSpec& inOther)

{
    *this = nsFilePath(inOther);
    if (mURL[mURL.Length() - 1] != '/' && inOther.IsDirectory())
        mURL += "/";
} 







nsFilePath::nsFilePath(const nsFilePath& inPath)

    : mPath(inPath.mPath)
{
}


nsFilePath::nsFilePath(const char* inString, PRBool inCreateDirs)

:    mPath(inString)
{
    if (mPath.IsEmpty())
    	return;
    	
    NS_ASSERTION(strstr(inString, kFileURLPrefix) != inString, "URL passed as path");

#if defined(XP_WIN) || defined(XP_OS2)
    nsFileSpecHelpers::UnixToNative(mPath);
#endif
    
    nsFileSpecHelpers::Canonify(mPath, inCreateDirs);
#if defined(XP_WIN) || defined(XP_OS2)
    
    
    
    NS_ASSERTION( mPath[1] == ':' || (mPath[0] == '\\' && mPath[1] == '\\'),
                 "unexpected canonical path" );
    nsFileSpecHelpers::NativeToUnix(mPath);
#endif
}


nsFilePath::nsFilePath(const nsString& inString, PRBool inCreateDirs)

:    mPath(inString)
{
    if (mPath.IsEmpty())
    	return;

    NS_ASSERTION(strstr((const char*)mPath, kFileURLPrefix) != (const char*)mPath, "URL passed as path");
#if defined(XP_WIN) || defined(XP_OS2)
    nsFileSpecHelpers::UnixToNative(mPath);
#endif
    
    nsFileSpecHelpers::Canonify(mPath, inCreateDirs);
#if defined(XP_WIN) || defined(XP_OS2)
    NS_ASSERTION( mPath[1] == ':' || (mPath[0] == '\\' && mPath[1] == '\\'),
                 "unexpected canonical path" );
    nsFileSpecHelpers::NativeToUnix(mPath);
#endif
}


nsFilePath::nsFilePath(const nsFileURL& inOther)

{
    mPath = (const char*)inOther.mURL + kFileURLPrefixLength;
    mPath.Unescape();
}

#if (defined XP_UNIX || defined XP_BEOS)

nsFilePath::nsFilePath(const nsFileSpec& inOther)

:    mPath(inOther.mPath)
{
}
#endif 


nsFilePath::~nsFilePath()

{
}

#if (defined XP_UNIX || defined XP_BEOS)

void nsFilePath::operator = (const nsFileSpec& inOther)

{
    

    mPath = inOther.mPath;
}
#endif 


void nsFilePath::operator = (const char* inString)

{

    NS_ASSERTION(strstr(inString, kFileURLPrefix) != inString, "URL passed as path");
    mPath = inString;
    if (mPath.IsEmpty())
    	return;
#if defined(XP_WIN) || defined(XP_OS2)
    nsFileSpecHelpers::UnixToNative(mPath);
#endif
    
    nsFileSpecHelpers::Canonify(mPath, PR_FALSE );
#if defined(XP_WIN) || defined(XP_OS2)
    nsFileSpecHelpers::NativeToUnix(mPath);
#endif
}


void nsFilePath::operator = (const nsFileURL& inOther)

{
    mPath = (const char*)nsFilePath(inOther);
}


void nsFilePath::operator = (const nsFilePath& inOther)

{
    mPath = inOther.mPath;
}


void nsFilePath::operator +=(const char* inRelativeUnixPath)

{
	NS_ASSERTION(inRelativeUnixPath, "Attempt append relative path with null path");

    char* escapedPath = nsEscape(inRelativeUnixPath, url_Path);
    mPath += escapedPath;
    nsCRT::free(escapedPath);
} 


nsFilePath nsFilePath::operator +(const char* inRelativeUnixPath) const

{
   NS_ASSERTION(inRelativeUnixPath, "Attempt append relative path with null path");

   nsFilePath resultPath(*this);
   resultPath += inRelativeUnixPath;
   return resultPath;
}  







nsFileSpec::nsFileSpec()

:    mError(NS_OK)		
{

}


void nsFileSpec::Clear()

{
    mPath.SetToEmpty();
    mError = NS_ERROR_NOT_INITIALIZED;
}


nsFileSpec::~nsFileSpec()

{
    
}


nsFileSpec::nsFileSpec(const nsPersistentFileDescriptor& inDescriptor)

{
    *this = inDescriptor;
}


nsFileSpec::nsFileSpec(const nsFileURL& inURL)

{
    *this = nsFilePath(inURL); 
}


void nsFileSpec::MakeUnique(const char* inSuggestedLeafName, PRBool inCreateFile)

{
    if (inSuggestedLeafName && *inSuggestedLeafName)
        SetLeafName(inSuggestedLeafName);
    MakeUnique(inCreateFile);
} 


void nsFileSpec::MakeUnique(PRBool inCreateFile)

{
    
    
    
    nsCAutoString path;
    nsCOMPtr<nsILocalFile> localFile;
    NS_NewNativeLocalFile(nsDependentCString(*this), PR_TRUE, getter_AddRefs(localFile));
    if (localFile)
    {
        nsresult rv;

        if (inCreateFile)
            rv = localFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
        else
            rv = localFile->CreateUnique(nsIFile::DIRECTORY_TYPE, 0700);

        if (NS_SUCCEEDED(rv))
            localFile->GetNativePath(path);
    }

    NS_ASSERTION(!path.IsEmpty(), "MakeUnique() failed!");
    *this = path.get(); 

} 


void nsFileSpec::operator = (const nsFileURL& inURL)

{
    *this = nsFilePath(inURL); 
}


void nsFileSpec::operator = (const nsPersistentFileDescriptor& inDescriptor)

{

    nsCAutoString data;
    inDescriptor.GetData(data);

#ifdef XP_MACOSX
    
    char* decodedData = PL_Base64Decode(data.get(), data.Length(), nsnull);
    Handle aliasH = nsnull;
    mError = NS_FILE_RESULT(::PtrToHand(decodedData, &aliasH, (data.Length() * 3) / 4));
    PR_Free(decodedData);
    if (NS_FAILED(mError))
        return; 

    Boolean changed;
    FSRef fileRef;
    mError = NS_FILE_RESULT(::FSResolveAlias(nsnull, (AliasHandle)aliasH, &fileRef, &changed));
    ::DisposeHandle(aliasH);

    UInt8 pathBuf[PATH_MAX];
    mError = NS_FILE_RESULT(::FSRefMakePath(&fileRef, pathBuf, PATH_MAX));
    if (NS_FAILED(mError))
      return;
    mPath = (const char*)pathBuf;
#else
    mPath = data.get();
    mError = NS_OK;
#endif
}





#if (defined XP_UNIX || defined XP_BEOS)

nsFileSpec::nsFileSpec(const nsFilePath& inPath)

:    mPath((const char*)inPath)
,    mError(NS_OK)
{

}


void nsFileSpec::operator = (const nsFilePath& inPath)

{
    mPath = (const char*)inPath;
    mError = NS_OK;
}
#endif 

#if (defined(XP_UNIX) || defined(XP_WIN) || defined(XP_OS2) || defined(XP_BEOS))

nsFileSpec::nsFileSpec(const nsFileSpec& inSpec)

:    mPath(inSpec.mPath)
,    mError(NS_OK)
{

}


nsFileSpec::nsFileSpec(const char* inString, PRBool inCreateDirs)

:    mPath(inString)
,    mError(NS_OK)
{

    
    nsFileSpecHelpers::Canonify(mPath, inCreateDirs);
}


nsFileSpec::nsFileSpec(const nsString& inString, PRBool inCreateDirs)

:    mPath(inString)
,    mError(NS_OK)
{

    
    nsFileSpecHelpers::Canonify(mPath, inCreateDirs);
}


void nsFileSpec::operator = (const nsFileSpec& inSpec)

{
    mPath = inSpec.mPath;
    mError = inSpec.Error();
}


void nsFileSpec::operator = (const char* inString)

{
    mPath = inString;
    
    nsFileSpecHelpers::Canonify(mPath, PR_FALSE );
    mError = NS_OK;
}
#endif 


nsFileSpec nsFileSpec::operator + (const char* inRelativePath) const

{
	NS_ASSERTION(inRelativePath, "Attempt to append name with a null string");

    nsFileSpec resultSpec = *this;
    resultSpec += inRelativePath;
    return resultSpec;
} 


PRBool nsFileSpec::operator == (const nsFileSpec& inOther) const

{
    PRBool amEmpty = mPath.IsEmpty();
    PRBool heEmpty = inOther.mPath.IsEmpty();
    if (amEmpty) 
        return heEmpty;
    if (heEmpty) 
        return PR_FALSE;
    
    nsSimpleCharString      str = mPath;
    nsSimpleCharString      inStr = inOther.mPath;
    
    
    PRUint32 strLast = str.Length() - 1, inLast = inStr.Length() - 1;
#if defined(XP_WIN) || defined(XP_OS2)
#define DIR_SEPARATOR '\\'      // XXX doesn't NSPR have this?
    
#ifdef XP_OS2
#define DIR_STRCMP     strcmp
#else
#define DIR_STRCMP    _stricmp
#endif
#else
#define DIR_SEPARATOR '/'
#if defined(VMS)
#define DIR_STRCMP     strcasecmp
#else
#define DIR_STRCMP     strcmp
#endif
#endif    
    if(str[strLast] == DIR_SEPARATOR)
        str[strLast] = '\0';

    if(inStr[inLast] == DIR_SEPARATOR)
        inStr[inLast] = '\0';

    if (DIR_STRCMP(str, inStr) == 0)
           return PR_TRUE;
#undef DIR_SEPARATOR
#undef DIR_STRCMP
   return PR_FALSE;
}


PRBool nsFileSpec::operator != (const nsFileSpec& inOther) const

{
    return (! (*this == inOther) );
}








const char* nsFileSpec::GetCString() const

{
    return mPath;
}



PRBool nsFileSpec::IsChildOf(nsFileSpec &possibleParent)

{
    nsFileSpec iter = *this, parent;
#ifdef DEBUG
    int depth = 0;
#endif
    while (1) {
#ifdef DEBUG
        
        NS_ASSERTION(depth < 100, "IsChildOf has lost its little mind");
        if (depth > 100)
            return PR_FALSE;
#endif
        if (iter == possibleParent)
            return PR_TRUE;

        iter.GetParent(parent); 
        if (iter.Failed())
            return PR_FALSE;

        if (iter == parent)     
            return PR_FALSE;
        
        iter = parent;
#ifdef DEBUG
        depth++;
#endif
    }

    
    return PR_FALSE;
}







nsPersistentFileDescriptor::nsPersistentFileDescriptor(const nsPersistentFileDescriptor& inDesc)

    : mDescriptorString(inDesc.mDescriptorString)
{
} 


void nsPersistentFileDescriptor::operator = (const nsPersistentFileDescriptor& inDesc)

{
    mDescriptorString = inDesc.mDescriptorString;
} 


nsPersistentFileDescriptor::nsPersistentFileDescriptor(const nsFileSpec& inSpec)

{
    *this = inSpec;
} 


void nsPersistentFileDescriptor::operator = (const nsFileSpec& inSpec)

{
#ifdef XP_MACOSX
    if (inSpec.Error())
        return;
    
    FSRef fileRef;
    Boolean isDir;
    OSErr err = ::FSPathMakeRef((const UInt8*)inSpec.GetCString(), &fileRef, &isDir);
    if (err != noErr)
        return;
    
    AliasHandle    aliasH;
    err = ::FSNewAlias(nsnull, &fileRef, &aliasH);
    if (err != noErr)
        return;

    PRUint32 bytes = ::GetHandleSize((Handle) aliasH);
    ::HLock((Handle)aliasH);
    char* buf = PL_Base64Encode((const char*)*aliasH, bytes, nsnull);
    ::DisposeHandle((Handle) aliasH);

    mDescriptorString = buf;
    PR_Free(buf);
#else
    mDescriptorString = inSpec.GetCString();
#endif
} 


nsPersistentFileDescriptor::~nsPersistentFileDescriptor()

{
} 


void nsPersistentFileDescriptor::GetData(nsAFlatCString& outData) const

{
    outData.Assign(mDescriptorString, mDescriptorString.Length());
}


void nsPersistentFileDescriptor::SetData(const nsAFlatCString& inData)

{
    mDescriptorString.CopyFrom(inData.get(), inData.Length());
}


void nsPersistentFileDescriptor::SetData(const char* inData, PRInt32 inSize)

{
    mDescriptorString.CopyFrom(inData, inSize);
}






nsNSPRPath::operator const char*() const



{
#if defined(XP_WIN) || defined(XP_OS2)
    if (!modifiedNSPRPath)
    {
        
        
        const char* unixPath = (const char*)mFilePath;
        if (!unixPath)
            return nsnull;

        ((nsNSPRPath*)this)->modifiedNSPRPath
                = nsCRT::strdup(*unixPath == '/' ? unixPath + 1: unixPath);
        
        
        if (modifiedNSPRPath[1] == '|')
             modifiedNSPRPath[1] = ':';
        
        
        int len = strlen(modifiedNSPRPath);
        if (modifiedNSPRPath[len - 1 ] == '/' && modifiedNSPRPath[len - 2 ] != ':')
            modifiedNSPRPath[len - 1 ] = '\0';     
    }
    return modifiedNSPRPath;    
#else
    return (const char*)mFilePath;
#endif
}


nsNSPRPath::~nsNSPRPath()

{
#if defined(XP_WIN) || defined(XP_OS2)
    if (modifiedNSPRPath)
        nsCRT::free(modifiedNSPRPath);
#endif
}


nsresult 
NS_FileSpecToIFile(nsFileSpec* fileSpec, nsILocalFile* *result)
{
    nsresult rv;

    nsCOMPtr<nsILocalFile> file(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID));

    if (!file) return NS_ERROR_FAILURE;

    rv = file->InitWithNativePath(nsDependentCString(fileSpec->GetNativePathCString()));

    if (NS_FAILED(rv)) return rv;

    *result = file;
    NS_ADDREF(*result);
    return NS_OK;
}




