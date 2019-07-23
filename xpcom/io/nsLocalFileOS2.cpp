











































#include "nsCOMPtr.h"
#include "nsMemory.h"

#include "nsLocalFile.h"
#include "nsNativeCharsetUtils.h"

#include "nsISimpleEnumerator.h"
#include "nsIDirectoryEnumerator.h"
#include "nsIComponentManager.h"
#include "nsIProgrammingLanguage.h"
#include "prtypes.h"
#include "prio.h"

#include "nsReadableUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsIMutableArray.h"
#include "nsTraceRefcntImpl.h"

#define CHECK_mWorkingPath()                    \
    PR_BEGIN_MACRO                              \
        if (mWorkingPath.IsEmpty())             \
            return NS_ERROR_NOT_INITIALIZED;    \
    PR_END_MACRO





static nsresult ConvertOS2Error(int err)
{
    nsresult rv;

    switch (err)
    {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_INVALID_DRIVE:
            rv = NS_ERROR_FILE_NOT_FOUND;
            break;
        case ERROR_ACCESS_DENIED:
        case ERROR_NOT_SAME_DEVICE:
            rv = NS_ERROR_FILE_ACCESS_DENIED;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_INVALID_BLOCK:
        case ERROR_INVALID_HANDLE:
        case ERROR_ARENA_TRASHED:
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
        case ERROR_CURRENT_DIRECTORY:
            rv = NS_ERROR_FILE_DIR_NOT_EMPTY;
            break;
        case ERROR_WRITE_PROTECT:
            rv = NS_ERROR_FILE_READ_ONLY;
            break;
        case ERROR_HANDLE_DISK_FULL:
            rv = NS_ERROR_FILE_TOO_BIG;
            break;
        case ERROR_FILE_EXISTS:
        case ERROR_ALREADY_EXISTS:
        case ERROR_CANNOT_MAKE:
            rv = NS_ERROR_FILE_ALREADY_EXISTS;
            break;
        case ERROR_FILENAME_EXCED_RANGE:
            rv = NS_ERROR_FILE_NAME_TOO_LONG;
            break;
        case 0:
            rv = NS_OK;
        default:
            rv = NS_ERROR_FAILURE;
    }
    return rv;
}

static void
myLL_L2II(PRInt64 result, PRInt32 *hi, PRInt32 *lo )
{
    PRInt64 a64, b64;  
                       
                       

    
    LL_SHR(a64, result, 32);
    LL_L2I(*hi, a64);

    
    LL_SHL(b64, result, 32);
    LL_SHR(a64, b64, 32);
    LL_L2I(*lo, a64);
}


static unsigned char* PR_CALLBACK
_mbschr(const unsigned char* stringToSearch, int charToSearchFor)
{
    const unsigned char* p = stringToSearch;

    do {
        if (*p == charToSearchFor)
            break;
        p  = (const unsigned char*)WinNextChar(0,0,0,(char*)p);
    } while (*p);

    
    return *p ? (unsigned char*)p : NULL;
}


static unsigned char* PR_CALLBACK
_mbsstr(const unsigned char* stringToSearch, const unsigned char* subString)
{
    const unsigned char* pStr = stringToSearch;
    const unsigned char* pSub = subString;

    do {
        while (*pStr && *pStr != *pSub)
            pStr = (const unsigned char*)WinNextChar(0,0,0,(char*)pStr);

        if (!*pStr)
            break;

        const unsigned char* pNxt = pStr;
        do {
            pSub = (const unsigned char*)WinNextChar(0,0,0,(char*)pSub);
            pNxt = (const unsigned char*)WinNextChar(0,0,0,(char*)pNxt);
        } while (*pSub && *pSub == *pNxt);

        if (!*pSub)
            break;

        pSub = subString;
        pStr = (const unsigned char*)WinNextChar(0,0,0,(char*)pStr);

    } while (*pStr);

    
    return *pSub ? NULL : (unsigned char*)pStr;
}


NS_EXPORT unsigned char*
_mbsrchr(const unsigned char* stringToSearch, int charToSearchFor)
{
    int length = strlen((const char*)stringToSearch);
    const unsigned char* p = stringToSearch+length;

    do {
        if (*p == charToSearchFor)
            break;
        p  = (const unsigned char*)WinPrevChar(0,0,0,(char*)stringToSearch,(char*)p);
    } while (p > stringToSearch);

    
    return (*p == charToSearchFor) ? (unsigned char*)p : NULL;
}


static nsresult PR_CALLBACK
CreateDirectoryA(PSZ path, PEAOP2 ppEABuf)
{
    APIRET rc;
    nsresult rv;
    FILESTATUS3 pathInfo;

    rc = DosCreateDir(path, ppEABuf);
    if (rc != NO_ERROR)
    {
        rv = ConvertOS2Error(rc);

        
        
        rc = DosQueryPathInfo(path, FIL_STANDARD,
                              &pathInfo, sizeof(pathInfo));
        if (rc == NO_ERROR)
            rv = ERROR_FILE_EXISTS;
    }
    else
        rv = rc;

    return rv;
}

static int isleadbyte(int c)
{
    static BOOL bDBCSFilled = FALSE;
    
    static BYTE DBCSInfo[12] = { 0 };
    BYTE *curr;
    BOOL retval = FALSE;

    if(!bDBCSFilled)
    {
        COUNTRYCODE ctrycodeInfo = { 0 };
        APIRET rc = NO_ERROR;
        ctrycodeInfo.country = 0;     
        ctrycodeInfo.codepage = 0;    

        rc = DosQueryDBCSEnv(sizeof(DBCSInfo), &ctrycodeInfo, DBCSInfo);
        
        if (rc != NO_ERROR)
            return FALSE;

        bDBCSFilled=TRUE;
    }

    
    
    curr = DBCSInfo;
    while(*curr != 0 && *(curr+1) != 0)
    {
        if(c >= *curr && c <= *(curr+1))
        {
            retval=TRUE;
            break;
        }
        curr+=2;
    }

    return retval;
}





class nsDirEnumerator : public nsISimpleEnumerator,
                        public nsIDirectoryEnumerator
{
    public:

        NS_DECL_ISUPPORTS

        nsDirEnumerator() : mDir(nsnull)
        {
        }

        nsresult Init(nsILocalFile* parent)
        {
            nsCAutoString filepath;
            parent->GetNativeTarget(filepath);

            if (filepath.IsEmpty())
            {
                parent->GetNativePath(filepath);
            }

            if (filepath.IsEmpty())
            {
                return NS_ERROR_UNEXPECTED;
            }

            mDir = PR_OpenDir(filepath.get());
            if (mDir == nsnull)    
                return NS_ERROR_FAILURE;

            mParent = parent;
            return NS_OK;
        }

        NS_IMETHOD HasMoreElements(PRBool *result)
        {
            nsresult rv;
            if (mNext == nsnull && mDir)
            {
                PRDirEntry* entry = PR_ReadDir(mDir, PR_SKIP_BOTH);
                if (entry == nsnull)
                {
                    

                    PRStatus status = PR_CloseDir(mDir);
                    if (status != PR_SUCCESS)
                        return NS_ERROR_FAILURE;
                    mDir = nsnull;

                    *result = PR_FALSE;
                    return NS_OK;
                }

                nsCOMPtr<nsIFile> file;
                rv = mParent->Clone(getter_AddRefs(file));
                if (NS_FAILED(rv))
                    return rv;

                rv = file->AppendNative(nsDependentCString(entry->name));
                if (NS_FAILED(rv))
                    return rv;

                
                PRBool exists;
                rv = file->Exists(&exists);
                if (NS_FAILED(rv) || !exists)
                {
                    return HasMoreElements(result);
                }

                mNext = do_QueryInterface(file);
            }
            *result = mNext != nsnull;
            if (!*result)
                Close();
            return NS_OK;
        }

        NS_IMETHOD GetNext(nsISupports **result)
        {
            nsresult rv;
            PRBool hasMore;
            rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv)) return rv;

            *result = mNext;        
            NS_IF_ADDREF(*result);

            mNext = nsnull;
            return NS_OK;
        }

        NS_IMETHOD GetNextFile(nsIFile **result)
        {
            *result = nsnull;
            PRBool hasMore = PR_FALSE;
            nsresult rv = HasMoreElements(&hasMore);
            if (NS_FAILED(rv) || !hasMore)
                return rv;
            *result = mNext;
            NS_IF_ADDREF(*result);
            mNext = nsnull;
            return NS_OK;
        }

        NS_IMETHOD Close()
        {
            if (mDir)
            {
                PRStatus status = PR_CloseDir(mDir);
                NS_ASSERTION(status == PR_SUCCESS, "close failed");
                if (status != PR_SUCCESS)
                    return NS_ERROR_FAILURE;
                mDir = nsnull;
            }
            return NS_OK;
        }

        
        
        ~nsDirEnumerator()
        {
            Close();
        }

    protected:
        PRDir*                  mDir;
        nsCOMPtr<nsILocalFile>  mParent;
        nsCOMPtr<nsILocalFile>  mNext;
};

NS_IMPL_ISUPPORTS2(nsDirEnumerator, nsISimpleEnumerator, nsIDirectoryEnumerator)





class nsDriveEnumerator : public nsISimpleEnumerator
{
public:
    nsDriveEnumerator();
    virtual ~nsDriveEnumerator();
    NS_DECL_ISUPPORTS
    NS_DECL_NSISIMPLEENUMERATOR
    nsresult Init();

private:
    
    
    PRUint32    mDrives;
    PRUint8     mLetter;
};

NS_IMPL_ISUPPORTS1(nsDriveEnumerator, nsISimpleEnumerator)

nsDriveEnumerator::nsDriveEnumerator()
 : mDrives(0), mLetter(0)
{
}

nsDriveEnumerator::~nsDriveEnumerator()
{
}

nsresult nsDriveEnumerator::Init()
{
    ULONG   ulCurrent;

    
    DosError(FERR_DISABLEHARDERR);
    APIRET rc = DosQueryCurrentDisk(&ulCurrent, (PULONG)&mDrives);
    DosError(FERR_ENABLEHARDERR);
    if (rc)
        return NS_ERROR_FAILURE;

    mLetter = 'A';
    return NS_OK;
}

NS_IMETHODIMP nsDriveEnumerator::HasMoreElements(PRBool *aHasMore)
{
    
    *aHasMore = (mDrives != 0);
    return NS_OK;
}

NS_IMETHODIMP nsDriveEnumerator::GetNext(nsISupports **aNext)
{
    if (!mDrives)
    {
        *aNext = nsnull;
        return NS_OK;
    }

    
    while ((mDrives & 1) == 0)
    {
        mDrives >>= 1;
        mLetter++;
    }

    
    char drive[4] = "x:\\";
    drive[0] = mLetter;
    mDrives >>= 1;
    mLetter++;

    nsILocalFile *file;
    nsresult rv = NS_NewNativeLocalFile(nsDependentCString(drive),
                                        PR_FALSE, &file);
    *aNext = file;

    return rv;
}









typedef struct _MVHDR {
    USHORT  usEAType;
    USHORT  usCodePage;
    USHORT  usNumEntries;
    USHORT  usDataType;
    USHORT  usDataLth;
    char    data[1];
} MVHDR;

typedef MVHDR *PMVHDR;


class TypeEaEnumerator
{
public:
    TypeEaEnumerator() : mEaBuf(nsnull) { }
    ~TypeEaEnumerator() { if (mEaBuf) NS_Free(mEaBuf); }

    nsresult Init(nsLocalFile * aFile);
    char *   GetNext(PRUint32 *lth);

private:
    char *  mEaBuf;
    char *  mpCur;
    PMVHDR  mpMvh;
    USHORT  mLth;
    USHORT  mCtr;
};


nsresult TypeEaEnumerator::Init(nsLocalFile * aFile)
{
#define EABUFSIZE 512

    
    mEaBuf = (char*)NS_Alloc(EABUFSIZE);
    if (!mEaBuf)
        return NS_ERROR_OUT_OF_MEMORY;

    PFEA2LIST   pfea2list = (PFEA2LIST)mEaBuf;
    pfea2list->cbList = EABUFSIZE;

    
    nsresult rv = aFile->GetEA(".TYPE", pfea2list);
    if (NS_FAILED(rv))
        return rv;

    
    
    mpMvh = (PMVHDR)&(pfea2list->list[0].szName[pfea2list->list[0].cbName+1]);
    if (mpMvh->usEAType != EAT_MVMT)
        if (mpMvh->usEAType != EAT_MVST || mpMvh->usDataType != EAT_ASCII)
            return NS_ERROR_FAILURE;

    
    mLth = 0;
    mCtr = 0;
    mpCur = (char*)(mpMvh->usEAType == EAT_MVMT ?
                    &mpMvh->usDataType : &mpMvh->usDataLth);

    return NS_OK;
}


char *   TypeEaEnumerator::GetNext(PRUint32 *lth)
{
    char *  result = nsnull;

    
    
    while (mCtr++ < mpMvh->usNumEntries) {

        
        mpCur += mLth;

        
        
        if (mpMvh->usEAType == EAT_MVMT) {
            if (*((PUSHORT)mpCur) != EAT_ASCII)
                continue;
            mpCur += sizeof(USHORT);
        }

        
        mLth = *lth = *((PUSHORT)mpCur);
        mpCur += sizeof(USHORT);
        result = mpCur;
        break;
    }

    return result;
}





nsLocalFile::nsLocalFile()
  : mDirty(PR_TRUE)
{
}

NS_METHOD
nsLocalFile::nsLocalFileConstructor(nsISupports* outer, const nsIID& aIID, void* *aInstancePtr)
{
    NS_ENSURE_ARG_POINTER(aInstancePtr);
    NS_ENSURE_NO_AGGREGATION(outer);

    nsLocalFile* inst = new nsLocalFile();
    if (inst == NULL)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = inst->QueryInterface(aIID, aInstancePtr);
    if (NS_FAILED(rv))
    {
        delete inst;
        return rv;
    }
    return NS_OK;
}






NS_IMPL_THREADSAFE_ADDREF(nsLocalFile)
NS_IMPL_THREADSAFE_RELEASE(nsLocalFile)
NS_IMPL_QUERY_INTERFACE5_CI(nsLocalFile,
                            nsILocalFile,
                            nsIFile,
                            nsILocalFileOS2,
                            nsIHashable,
                            nsIClassInfo)
NS_IMPL_CI_INTERFACE_GETTER4(nsLocalFile,
                             nsILocalFile,
                             nsIFile,
                             nsILocalFileOS2,
                             nsIHashable)

NS_DECL_CLASSINFO(nsLocalFile)
NS_IMPL_THREADSAFE_CI(nsLocalFile)






nsLocalFile::nsLocalFile(const nsLocalFile& other)
  : mDirty(PR_TRUE)
  , mWorkingPath(other.mWorkingPath)
{
}




nsresult
nsLocalFile::Stat()
{
    
    if (!mDirty)
        return NS_OK;

    
    if (mWorkingPath.IsEmpty())
        return NS_ERROR_FILE_INVALID_PATH;

    
    char temp[4];
    const char *nsprPath = mWorkingPath.get();
    if (mWorkingPath.Length() == 2 && mWorkingPath.CharAt(1) == ':')
    {
        temp[0] = mWorkingPath[0];
        temp[1] = mWorkingPath[1];
        temp[2] = '\\';
        temp[3] = '\0';
        nsprPath = temp;
    }

    
    DosError(FERR_DISABLEHARDERR);
    PRStatus status = PR_GetFileInfo64(nsprPath, &mFileInfo64);
    DosError(FERR_ENABLEHARDERR);
    if (status != PR_SUCCESS)
        return NS_ERROR_FILE_NOT_FOUND;

    mDirty = PR_FALSE;
    return NS_OK;
}






NS_IMETHODIMP
nsLocalFile::Clone(nsIFile **file)
{
    
    *file = new nsLocalFile(*this);
    if (!*file)
      return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*file);

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::InitWithNativePath(const nsACString &filePath)
{
    MakeDirty();

    nsACString::const_iterator begin, end;
    filePath.BeginReading(begin);
    filePath.EndReading(end);

    
    if (begin == end)
        return NS_ERROR_FAILURE;

    char firstChar = *begin;
    char secondChar = *(++begin);

    
    

    char *path = nsnull;
    PRInt32 pathLen = 0;

    if ( ( (secondChar == ':') && !FindCharInReadable('/', begin, end) ) ||  
         ( (firstChar == '\\') && (secondChar == '\\') ) )  
    {
        
        path = ToNewCString(filePath);
        pathLen = filePath.Length();
    }

    if (path == nsnull)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    
    PRInt32 len = pathLen - 1;
    if (path[len] == '\\' && !::isleadbyte(path[len-1]))
    {
        path[len] = '\0';
        pathLen = len;
    }

    mWorkingPath.Adopt(path, pathLen);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::OpenNSPRFileDesc(PRInt32 flags, PRInt32 mode, PRFileDesc **_retval)
{
    nsresult rv = Stat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    *_retval = PR_Open(mWorkingPath.get(), flags, mode);
    if (*_retval)
        return NS_OK;

    return NS_ErrorAccordingToNSPR();
}


NS_IMETHODIMP
nsLocalFile::OpenANSIFileDesc(const char *mode, FILE * *_retval)
{
    nsresult rv = Stat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    *_retval = fopen(mWorkingPath.get(), mode);
    if (*_retval)
        return NS_OK;

    return NS_ERROR_FAILURE;
}



NS_IMETHODIMP
nsLocalFile::Create(PRUint32 type, PRUint32 attributes)
{
    if (type != NORMAL_FILE_TYPE && type != DIRECTORY_TYPE)
        return NS_ERROR_FILE_UNKNOWN_TYPE;

    nsresult rv = Stat();
    if (NS_FAILED(rv) && rv != NS_ERROR_FILE_NOT_FOUND)
        return rv;

    
    
    
    
    
    
    
    
    
    
    
    

    unsigned char* path = (unsigned char*) mWorkingPath.BeginWriting();

    if (path[0] == '\\' && path[1] == '\\')
    {
        
        path = _mbschr(path + 2, '\\');
        if (!path)
            return NS_ERROR_FILE_INVALID_PATH;
        ++path;
    }

    
    unsigned char* slash = _mbschr(path, '\\');

    if (slash)
    {
        
        ++slash;
        slash = _mbschr(slash, '\\');

        while (slash)
        {
            *slash = '\0';

            rv = CreateDirectoryA(const_cast<char*>(mWorkingPath.get()), NULL);
            if (rv) {
                rv = ConvertOS2Error(rv);
                if (rv != NS_ERROR_FILE_ALREADY_EXISTS)
                    return rv;
            }
            *slash = '\\';
            ++slash;
            slash = _mbschr(slash, '\\');
        }
    }

    if (type == NORMAL_FILE_TYPE)
    {
        PRFileDesc* file = PR_Open(mWorkingPath.get(), PR_RDONLY | PR_CREATE_FILE | PR_APPEND | PR_EXCL, attributes);
        if (!file) return NS_ERROR_FILE_ALREADY_EXISTS;

        PR_Close(file);
        return NS_OK;
    }

    if (type == DIRECTORY_TYPE)
    {
        rv = CreateDirectoryA(const_cast<char*>(mWorkingPath.get()), NULL);
        if (rv)
            return ConvertOS2Error(rv);
        else
            return NS_OK;
    }

    return NS_ERROR_FILE_UNKNOWN_TYPE;
}


NS_IMETHODIMP
nsLocalFile::AppendNative(const nsACString &node)
{
    
    return AppendNativeInternal(PromiseFlatCString(node), PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::AppendRelativeNativePath(const nsACString &node)
{
    
    return AppendNativeInternal(PromiseFlatCString(node), PR_TRUE);
}

nsresult
nsLocalFile::AppendNativeInternal(const nsAFlatCString &node, PRBool multipleComponents)
{
    if (node.IsEmpty())
        return NS_OK;

    
    const unsigned char * nodePath = (const unsigned char *) node.get();
    if (*nodePath == '\\'                           
        || _mbschr(nodePath, '/')                   
        || node.Equals(".."))                       
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    if (multipleComponents)
    {
        
        
        
        const unsigned char * doubleDot = _mbsstr(nodePath, (const unsigned char *)"\\..");
        while (doubleDot)
        {
            doubleDot += 3;
            if (*doubleDot == '\0' || *doubleDot == '\\')
                return NS_ERROR_FILE_UNRECOGNIZED_PATH;
            doubleDot = _mbsstr(doubleDot, (unsigned char *)"\\..");
        }
        
        
        if (*nodePath == '.') {
            nodePath = (const unsigned char*)WinNextChar(0,0,0,(char*)nodePath);
            if (*nodePath == '.' &&
                *WinNextChar(0,0,0,(char*)nodePath) == '\\')
                return NS_ERROR_FILE_UNRECOGNIZED_PATH;
        }
    }
    else if (_mbschr(nodePath, '\\'))   
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    MakeDirty();

    mWorkingPath.Append(NS_LITERAL_CSTRING("\\") + node);

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Normalize()
{
    

    if (mWorkingPath.IsEmpty())
        return NS_OK;

    
    nsAutoString path;
    NS_CopyNativeToUnicode(mWorkingPath, path);

    
    
    
    
    
    PRInt32 rootIdx = 2;        
    if (path.First() == '\\')   
    {
        rootIdx = path.FindChar('\\', 2);   
        if (rootIdx == kNotFound)
            return NS_OK;                   
        rootIdx = path.FindChar('\\', rootIdx+1);
        if (rootIdx == kNotFound)
            return NS_OK;                   
    }
    else if (path.CharAt(rootIdx) != '\\')
    {
        
        
        
        
        char drv[4] = "A:.";
        char cwd[CCHMAXPATH];

        drv[0] = mWorkingPath.First();
        if (DosQueryPathInfo(drv, FIL_QUERYFULLNAME, cwd, sizeof(cwd)))
            return NS_ERROR_FILE_NOT_FOUND;

        nsAutoString currentDir;
        NS_CopyNativeToUnicode(nsDependentCString(cwd), currentDir);

        if (currentDir.Last() == '\\')
            path.Replace(0, 2, currentDir);
        else
            path.Replace(0, 2, currentDir + NS_LITERAL_STRING("\\"));
    }
    NS_POSTCONDITION(0 < rootIdx && rootIdx < (PRInt32)path.Length(), "rootIdx is invalid");
    NS_POSTCONDITION(path.CharAt(rootIdx) == '\\', "rootIdx is invalid");

    
    if (rootIdx + 1 == (PRInt32)path.Length())
        return NS_OK;

    
    nsAutoString normal;
    const PRUnichar * pathBuffer = path.get();  
    normal.SetCapacity(path.Length()); 
    normal.Assign(pathBuffer, rootIdx);

    
    
    
    
    
    
    
    
    
    
    PRInt32 len, begin, end = rootIdx;
    while (end < (PRInt32)path.Length())
    {
        
        
        
        
        
        begin = end + 1;
        end = path.FindChar('\\', begin);
        if (end == kNotFound)
            end = path.Length();
        len = end - begin;

        
        if (len == 0)
            continue;

        
        if (pathBuffer[begin] == '.')
        {
            
            if (len == 1)
                continue;

            
            if (len >= 2 && pathBuffer[begin+1] == '.')
            {
                
                if (len == 2)
                {
                    PRInt32 prev = normal.RFindChar('\\');
                    if (prev >= rootIdx)
                        normal.Truncate(prev);
                    continue;
                }

                
                
                int idx = len - 1;
                for (; idx >= 2; --idx)
                {
                    if (pathBuffer[begin+idx] != '.')
                        break;
                }

                
                
                if (idx < 2)
                    continue;
            }
        }

        
        normal.Append(pathBuffer + begin - 1, len + 1);
    }

    
    PRInt32 filePathLen = normal.Length() - 1;
    while(filePathLen > 0 && (normal[filePathLen] == ' ' || normal[filePathLen] == '.'))
    {
        normal.Truncate(filePathLen--);
    }

    NS_CopyUnicodeToNative(normal, mWorkingPath);
    MakeDirty();

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetNativeLeafName(nsACString &aLeafName)
{
    aLeafName.Truncate();

    const char* temp = mWorkingPath.get();
    if(temp == nsnull)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    const char* leaf = (const char*) _mbsrchr((const unsigned char*) temp, '\\');

    
    if (leaf == nsnull)
        leaf = temp;
    else
        leaf++;

    aLeafName.Assign(leaf);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetNativeLeafName(const nsACString &aLeafName)
{
    MakeDirty();

    const unsigned char* temp = (const unsigned char*) mWorkingPath.get();
    if(temp == nsnull)
        return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    
    PRInt32 offset = (PRInt32) (_mbsrchr(temp, '\\') - temp);
    if (offset)
    {
        mWorkingPath.Truncate(offset+1);
    }
    mWorkingPath.Append(aLeafName);

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetNativePath(nsACString &_retval)
{
    _retval = mWorkingPath;
    return NS_OK;
}





nsresult
nsLocalFile::GetEA(const char *eaName, PFEA2LIST pfea2list)
{
    
    if (!pfea2list || !pfea2list->cbList)
        return NS_ERROR_FAILURE;

    
    
    union {
        GEA2LIST    gea2list;
        char        dummy[sizeof(GEA2LIST)+32];
    };
    EAOP2       eaop2;

    eaop2.fpFEA2List = pfea2list;
    eaop2.fpGEA2List = &gea2list;

    
    dummy[sizeof(GEA2LIST)+31] = 0;
    gea2list.list[0].oNextEntryOffset = 0;
    strcpy(gea2list.list[0].szName, eaName);
    gea2list.list[0].cbName = strlen(gea2list.list[0].szName);
    gea2list.cbList = sizeof(GEA2LIST) + gea2list.list[0].cbName;

    
    APIRET rc = DosQueryPathInfo(mWorkingPath.get(), FIL_QUERYEASFROMLIST,
                                 &eaop2, sizeof(eaop2));
    if (rc)
        return ConvertOS2Error(rc);

    
    if (!pfea2list->list[0].cbValue)
        return NS_ERROR_FAILURE;

    return NS_OK;
}




NS_IMETHODIMP
nsLocalFile::GetFileTypes(nsIArray **_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = 0;

    
    TypeEaEnumerator typeEnum;
    nsresult rv = typeEnum.Init(this);
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIMutableArray> mutArray =
        do_CreateInstance(NS_ARRAY_CONTRACTID);
    NS_ENSURE_STATE(mutArray);

    PRInt32  cnt;
    PRUint32 lth;
    char *   ptr;

    
    for (cnt=0, ptr=typeEnum.GetNext(&lth); ptr; ptr=typeEnum.GetNext(&lth)) {
        nsCOMPtr<nsISupportsCString> typeString(
                    do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) {
            nsCAutoString temp;
            temp.Assign(ptr, lth);
            typeString->SetData(temp);
            mutArray->AppendElement(typeString, PR_FALSE);
            cnt++;
        }
    }

    
    if (cnt) {
        *_retval = mutArray;
        NS_ADDREF(*_retval);
        rv = NS_OK;
    }
    else
        rv = NS_ERROR_FAILURE;

    return rv;
}




NS_IMETHODIMP
nsLocalFile::IsFileType(const nsACString& fileType, PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    
    TypeEaEnumerator typeEnum;
    nsresult rv = typeEnum.Init(this);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 lth;
    char *   ptr;

    
    for (ptr = typeEnum.GetNext(&lth); ptr; ptr = typeEnum.GetNext(&lth))
        if (fileType.EqualsASCII(ptr, lth)) {
            *_retval = PR_TRUE;
            break;
        }

    return NS_OK;
}





#pragma pack(1)
    typedef struct _TYPEEA {
        struct {
            ULONG   ulcbList;
            ULONG   uloNextEntryOffset;
            BYTE    bfEA;
            BYTE    bcbName;
            USHORT  uscbValue;
            char    chszName[sizeof(".TYPE")];
        } hdr;
        struct {
            USHORT  usEAType;
            USHORT  usCodePage;
            USHORT  usNumEntries;
            USHORT  usDataType;
            USHORT  usDataLth;
        } info;
        char    data[1];
    } TYPEEA;

    typedef struct _TYPEEA2 {
        USHORT  usDataType;
        USHORT  usDataLth;
    } TYPEEA2;
#pragma pack()



NS_IMETHODIMP
nsLocalFile::SetFileTypes(const nsACString& fileTypes)
{
    if (fileTypes.IsEmpty())
        return NS_ERROR_FAILURE;

    PRUint32 cnt = CountCharInReadable(fileTypes, ',');
    PRUint32 lth = fileTypes.Length() - cnt + (cnt * sizeof(TYPEEA2));
    PRUint32 size = sizeof(TYPEEA) + lth;

    char *pBuf = (char*)NS_Alloc(size);
    if (!pBuf)
        return NS_ERROR_OUT_OF_MEMORY;

    TYPEEA *pEA = (TYPEEA *)pBuf;

    
    pEA->hdr.ulcbList = size - 1;
    pEA->hdr.uloNextEntryOffset = 0;
    pEA->hdr.bfEA = 0;
    pEA->hdr.bcbName = sizeof(".TYPE") - 1;
    pEA->hdr.uscbValue = sizeof(pEA->info) + lth;
    strcpy(pEA->hdr.chszName, ".TYPE");

    pEA->info.usEAType = EAT_MVMT;
    pEA->info.usCodePage = 0;
    pEA->info.usNumEntries = ++cnt;

    nsACString::const_iterator begin, end, delim;
    fileTypes.BeginReading(begin);
    fileTypes.EndReading(end);
    delim = begin;

    
    
    
    do {
        FindCharInReadable( ',', delim, end);
        lth = delim.get() - begin.get();
        pEA->info.usDataType = EAT_ASCII;
        pEA->info.usDataLth = lth;
        memcpy(pEA->data, begin.get(), lth);
        pEA = (TYPEEA *)((char*)pEA + lth + sizeof(TYPEEA2));
        begin = ++delim;
    } while (--cnt);

    
    EAOP2 eaop2;
    eaop2.fpGEA2List = 0;
    eaop2.fpFEA2List = (PFEA2LIST)pBuf;

    int rc = DosSetPathInfo(mWorkingPath.get(), FIL_QUERYEASIZE,
                            &eaop2, sizeof(eaop2), 0);
    NS_Free(pBuf);

    if (rc)
        return ConvertOS2Error(rc);

    return NS_OK;
}





#pragma pack(1)
    typedef struct _SUBJEA {
        struct {
            ULONG   ulcbList;
            ULONG   uloNextEntryOffset;
            BYTE    bfEA;
            BYTE    bcbName;
            USHORT  uscbValue;
            char    chszName[sizeof(".SUBJECT")];
        } hdr;
        struct {
            USHORT  usEAType;
            USHORT  usDataLth;
        } info;
        char    data[1];
    } SUBJEA;
#pragma pack()


NS_IMETHODIMP
nsLocalFile::SetFileSource(const nsACString& aURI)
{
    if (aURI.IsEmpty())
        return NS_ERROR_FAILURE;

    
    PRUint32 lth = sizeof(SUBJEA) + aURI.Length();
    char *   pBuf = (char*)NS_Alloc(lth);
    if (!pBuf)
        return NS_ERROR_OUT_OF_MEMORY;

    SUBJEA *pEA = (SUBJEA *)pBuf;

    
    pEA->hdr.ulcbList = lth - 1;
    pEA->hdr.uloNextEntryOffset = 0;
    pEA->hdr.bfEA = 0;
    pEA->hdr.bcbName = sizeof(".SUBJECT") - 1;
    pEA->hdr.uscbValue = sizeof(pEA->info) + aURI.Length();
    strcpy(pEA->hdr.chszName, ".SUBJECT");
    pEA->info.usEAType = EAT_ASCII;
    pEA->info.usDataLth = aURI.Length();
    strcpy(pEA->data, PromiseFlatCString(aURI).get());

    
    EAOP2 eaop2;
    eaop2.fpGEA2List = 0;
    eaop2.fpFEA2List = (PFEA2LIST)pEA;

    int rc = DosSetPathInfo(mWorkingPath.get(), FIL_QUERYEASIZE,
                            &eaop2, sizeof(eaop2), 0);
    NS_Free(pBuf);

    if (rc)
        return ConvertOS2Error(rc);

    return NS_OK;
}



nsresult
nsLocalFile::CopySingleFile(nsIFile *sourceFile, nsIFile *destParent,
                            const nsACString &newName, PRBool move)
{
    nsresult rv;
    nsCAutoString filePath;

    nsCAutoString destPath;
    destParent->GetNativeTarget(destPath);

    destPath.Append("\\");

    if (newName.IsEmpty())
    {
        nsCAutoString aFileName;
        sourceFile->GetNativeLeafName(aFileName);
        destPath.Append(aFileName);
    }
    else
    {
        destPath.Append(newName);
    }

    rv = sourceFile->GetNativePath(filePath);

    if (NS_FAILED(rv))
        return rv;

    APIRET rc = NO_ERROR;

    if (move)
        rc = DosMove(filePath.get(), (PSZ)const_cast<char*>(destPath.get()));

    if (!move || rc == ERROR_NOT_SAME_DEVICE || rc == ERROR_ACCESS_DENIED)
    {
        
        
        

        do {
            rc = DosCopy(filePath.get(), (PSZ)const_cast<char*>(destPath.get()), DCPY_EXISTING);
            if (rc == ERROR_TOO_MANY_OPEN_FILES) {
                ULONG CurMaxFH = 0;
                LONG ReqCount = 20;
                APIRET rc2;
                rc2 = DosSetRelMaxFH(&ReqCount, &CurMaxFH);
                if (rc2 != NO_ERROR)
                    break;
            }
        } while (rc == ERROR_TOO_MANY_OPEN_FILES);

        
        if (rc == 65)
        {
            CHAR         achProgram[CCHMAXPATH];  
            RESULTCODES  rescResults;             

            strcpy(achProgram, "CMD.EXE  /C ");
            strcat(achProgram, """COPY ");
            strcat(achProgram, filePath.get());
            strcat(achProgram, " ");
            strcat(achProgram, (PSZ)const_cast<char*>(destPath.get()));
            strcat(achProgram, """");
            achProgram[strlen(achProgram) + 1] = '\0';
            achProgram[7] = '\0';
            DosExecPgm(NULL, 0,
                       EXEC_SYNC, achProgram, (PSZ)NULL,
                       &rescResults, achProgram);
            rc = 0; 

        } 

        
        
        if(move && (rc == NO_ERROR))
            DosDelete( filePath.get());

    } 

    if (rc)
        rv = ConvertOS2Error(rc);

    return rv;
}


nsresult
nsLocalFile::CopyMove(nsIFile *aParentDir, const nsACString &newName, PRBool move)
{
    
    CHECK_mWorkingPath();

    nsCOMPtr<nsIFile> newParentDir = aParentDir;

    nsresult rv  = Stat();
    if (NS_FAILED(rv))
        return rv;

    if (!newParentDir)
    {
        

        if (newName.IsEmpty())
            return NS_ERROR_INVALID_ARG;

        rv = GetParent(getter_AddRefs(newParentDir));
        if (NS_FAILED(rv))
            return rv;
    }

    if (!newParentDir)
        return NS_ERROR_FILE_DESTINATION_NOT_DIR;

    
    PRBool exists;
    newParentDir->Exists(&exists);
    if (!exists)
    {
        rv = newParentDir->Create(DIRECTORY_TYPE, 0644);  
        if (NS_FAILED(rv))
            return rv;
    }
    else
    {
        PRBool isDir;
        newParentDir->IsDirectory(&isDir);
        if (isDir == PR_FALSE)
        {
            return NS_ERROR_FILE_DESTINATION_NOT_DIR;
        }
    }

    
    PRBool done = PR_FALSE;
    PRBool isDir;
    IsDirectory(&isDir);

    
    if (move || !isDir)
    {
        
        
        rv = CopySingleFile(this, newParentDir, newName, move);
        done = NS_SUCCEEDED(rv);
        
        
        if (!done && !(move && isDir))
            return rv;
    }

    
    if (!done)
    {
        
        nsCOMPtr<nsIFile> target;
        rv = newParentDir->Clone(getter_AddRefs(target));

        if (NS_FAILED(rv))
            return rv;

        nsCAutoString allocatedNewName;
        if (newName.IsEmpty())
        {
            GetNativeLeafName(allocatedNewName);
        }
        else
        {
            allocatedNewName = newName;
        }

        rv = target->AppendNative(allocatedNewName);
        if (NS_FAILED(rv))
            return rv;

        allocatedNewName.Truncate();

        
        target->Exists(&exists);
        if (!exists)
        {
            
            rv = target->Create(DIRECTORY_TYPE, 0644);  
            if (NS_FAILED(rv))
                return rv;
        }
        else
        {
            
            PRBool isWritable;

            target->IsWritable(&isWritable);
            if (!isWritable)
                return NS_ERROR_FILE_ACCESS_DENIED;

            nsCOMPtr<nsISimpleEnumerator> targetIterator;
            rv = target->GetDirectoryEntries(getter_AddRefs(targetIterator));

            PRBool more;
            targetIterator->HasMoreElements(&more);
            
            if (more)
                return NS_ERROR_FILE_DIR_NOT_EMPTY;
        }

        nsDirEnumerator dirEnum;

        rv = dirEnum.Init(this);
        if (NS_FAILED(rv)) {
            NS_WARNING("dirEnum initialization failed");
            return rv;
        }

        PRBool more;
        while (NS_SUCCEEDED(dirEnum.HasMoreElements(&more)) && more)
        {
            nsCOMPtr<nsISupports> item;
            nsCOMPtr<nsIFile> file;
            dirEnum.GetNext(getter_AddRefs(item));
            file = do_QueryInterface(item);
            if (file)
            {
                if (move)
                {
                    rv = file->MoveToNative(target, EmptyCString());
                    NS_ENSURE_SUCCESS(rv,rv);
                }
                else
                {
                    rv = file->CopyToNative(target, EmptyCString());
                    NS_ENSURE_SUCCESS(rv,rv);
                }
            }
        }
        
        
        
        
        
        
        if (move)
        {
          rv = Remove(PR_FALSE); 
          NS_ENSURE_SUCCESS(rv,rv);
        }
    }


    
    if (move)
    {
        MakeDirty();

        nsCAutoString newParentPath;
        newParentDir->GetNativePath(newParentPath);

        if (newParentPath.IsEmpty())
            return NS_ERROR_FAILURE;

        if (newName.IsEmpty())
        {
            nsCAutoString aFileName;
            GetNativeLeafName(aFileName);

            InitWithNativePath(newParentPath);
            AppendNative(aFileName);
        }
        else
        {
            InitWithNativePath(newParentPath);
            AppendNative(newName);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::CopyToNative(nsIFile *newParentDir, const nsACString &newName)
{
    return CopyMove(newParentDir, newName, PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinksNative(nsIFile *newParentDir, const nsACString &newName)
{
    return CopyMove(newParentDir, newName, PR_FALSE);
}

NS_IMETHODIMP
nsLocalFile::MoveToNative(nsIFile *newParentDir, const nsACString &newName)
{
    return CopyMove(newParentDir, newName, PR_TRUE);
}

NS_IMETHODIMP
nsLocalFile::Load(PRLibrary * *_retval)
{
    
    CHECK_mWorkingPath();

    PRBool isFile;
    nsresult rv = IsFile(&isFile);

    if (NS_FAILED(rv))
        return rv;

    if (!isFile)
        return NS_ERROR_FILE_IS_DIRECTORY;

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(PR_FALSE);
#endif

    *_retval =  PR_LoadLibrary(mWorkingPath.get());

#ifdef NS_BUILD_REFCNT_LOGGING
    nsTraceRefcntImpl::SetActivityIsLegal(PR_TRUE);
#endif

    if (*_retval)
        return NS_OK;

    return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
nsLocalFile::Remove(PRBool recursive)
{
    
    CHECK_mWorkingPath();

    PRBool isDir = PR_FALSE;

    nsresult rv = IsDirectory(&isDir);
    if (NS_FAILED(rv))
        return rv;

    if (isDir)
    {
        if (recursive)
        {
            nsDirEnumerator dirEnum;

            rv = dirEnum.Init(this);
            if (NS_FAILED(rv))
                return rv;

            PRBool more;
            while (NS_SUCCEEDED(dirEnum.HasMoreElements(&more)) && more)
            {
                nsCOMPtr<nsISupports> item;
                dirEnum.GetNext(getter_AddRefs(item));
                nsCOMPtr<nsIFile> file = do_QueryInterface(item);
                if (file)
                    file->Remove(recursive);
            }
        }
        rv = rmdir(mWorkingPath.get());
    }
    else
    {
        rv = remove(mWorkingPath.get());
    }

    
    if (rv == (nsresult)-1)
        rv = NSRESULT_FOR_ERRNO();

    MakeDirty();
    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetLastModifiedTime(PRInt64 *aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aLastModifiedTime);

    *aLastModifiedTime = 0;
    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    
    PRInt64 usecPerMsec;
    LL_I2L(usecPerMsec, PR_USEC_PER_MSEC);
    LL_DIV(*aLastModifiedTime, mFileInfo64.modifyTime, usecPerMsec);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetLastModifiedTimeOfLink(PRInt64 *aLastModifiedTime)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTime(PRInt64 aLastModifiedTime)
{
    
    CHECK_mWorkingPath();

    return nsLocalFile::SetModDate(aLastModifiedTime);
}


NS_IMETHODIMP
nsLocalFile::SetLastModifiedTimeOfLink(PRInt64 aLastModifiedTime)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsLocalFile::SetModDate(PRInt64 aLastModifiedTime)
{
    nsresult rv = Stat();

    if (NS_FAILED(rv))
        return rv;

    PRExplodedTime pret;
    FILESTATUS3 pathInfo;

    
    rv = DosQueryPathInfo(mWorkingPath.get(), FIL_STANDARD,
                          &pathInfo, sizeof(pathInfo));

    if (NS_FAILED(rv))
    {
       rv = ConvertOS2Error(rv);
       return rv;
    }

    
    PR_ExplodeTime(aLastModifiedTime * PR_USEC_PER_MSEC, PR_LocalTimeParameters, &pret);

    
    if (pret.tm_year >= 1980)
        pathInfo.fdateLastWrite.year    = pret.tm_year-1980;
    else
        pathInfo.fdateLastWrite.year    = pret.tm_year;

    
    pathInfo.fdateLastWrite.month       = pret.tm_month + 1;
    pathInfo.fdateLastWrite.day         = pret.tm_mday;
    pathInfo.ftimeLastWrite.hours       = pret.tm_hour;
    pathInfo.ftimeLastWrite.minutes     = pret.tm_min;
    pathInfo.ftimeLastWrite.twosecs     = pret.tm_sec / 2;

    rv = DosSetPathInfo(mWorkingPath.get(), FIL_STANDARD,
                        &pathInfo, sizeof(pathInfo), 0UL);

    if (NS_FAILED(rv))
       return rv;

    MakeDirty();
    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetPermissions(PRUint32 *aPermissions)
{
    NS_ENSURE_ARG(aPermissions);

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    PRBool isWritable, isExecutable;
    IsWritable(&isWritable);
    IsExecutable(&isExecutable);

    *aPermissions = PR_IRUSR|PR_IRGRP|PR_IROTH;         
    if (isWritable)
        *aPermissions |= PR_IWUSR|PR_IWGRP|PR_IWOTH;    
    if (isExecutable)
        *aPermissions |= PR_IXUSR|PR_IXGRP|PR_IXOTH;    

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPermissionsOfLink(PRUint32 *aPermissionsOfLink)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
nsLocalFile::SetPermissions(PRUint32 aPermissions)
{
    
    CHECK_mWorkingPath();

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    APIRET rc;
    FILESTATUS3 pathInfo;

    
    rc = DosQueryPathInfo(mWorkingPath.get(), FIL_STANDARD,
                          &pathInfo, sizeof(pathInfo));

    if (rc != NO_ERROR)
       return ConvertOS2Error(rc);

    ULONG attr = 0;
    if (!(aPermissions & (PR_IWUSR|PR_IWGRP|PR_IWOTH)))
        attr = FILE_READONLY;

    if (attr == (pathInfo.attrFile & FILE_READONLY))
        return NS_OK;

    pathInfo.attrFile ^= FILE_READONLY;

    rc = DosSetPathInfo(mWorkingPath.get(), FIL_STANDARD,
                        &pathInfo, sizeof(pathInfo), 0UL);

    if (rc != NO_ERROR)
       return ConvertOS2Error(rc);

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetPermissionsOfLink(PRUint32 aPermissions)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsLocalFile::GetFileSize(PRInt64 *aFileSize)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aFileSize);
    *aFileSize = 0;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    *aFileSize = mFileInfo64.size;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::GetFileSizeOfLink(PRInt64 *aFileSize)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsLocalFile::SetFileSize(PRInt64 aFileSize)
{
    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    APIRET rc;
    HFILE hFile;
    ULONG actionTaken;

    rc = DosOpen(mWorkingPath.get(),
                 &hFile,
                 &actionTaken,
                 0,
                 FILE_NORMAL,
                 OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                 OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
                 NULL);

    if (rc != NO_ERROR)
    {
        MakeDirty();
        return NS_ERROR_FAILURE;
    }

    
    PRInt32 hi, lo;
    myLL_L2II(aFileSize, &hi, &lo );

    rc = DosSetFileSize(hFile, lo);
    DosClose(hFile);
    MakeDirty();

    if (rc != NO_ERROR)
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetDiskSpaceAvailable(PRInt64 *aDiskSpaceAvailable)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(aDiskSpaceAvailable);
    *aDiskSpaceAvailable = 0;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    ULONG ulDriveNo = toupper(mWorkingPath.CharAt(0)) + 1 - 'A';
    FSALLOCATE fsAllocate;
    APIRET rc = DosQueryFSInfo(ulDriveNo,
                               FSIL_ALLOC,
                               &fsAllocate,
                               sizeof(fsAllocate));

    if (rc != NO_ERROR)
       return NS_ERROR_FAILURE;

    *aDiskSpaceAvailable = fsAllocate.cUnitAvail;
    *aDiskSpaceAvailable *= fsAllocate.cSectorUnit;
    *aDiskSpaceAvailable *= fsAllocate.cbSector;

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetParent(nsIFile * *aParent)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG_POINTER(aParent);

    nsCAutoString parentPath(mWorkingPath);

    
    PRInt32 offset = (PRInt32) (_mbsrchr((const unsigned char *) parentPath.get(), '\\')
                     - (const unsigned char *) parentPath.get());
    
    
    
    
    if (offset < 0)
      return NS_ERROR_FILE_UNRECOGNIZED_PATH;

    if (offset == 1 && parentPath[0] == '\\') {
        aParent = nsnull;
        return NS_OK;
    }
    if (offset > 0)
        parentPath.Truncate(offset);
    else
        parentPath.AssignLiteral("\\\\.");

    nsCOMPtr<nsILocalFile> localFile;
    nsresult rv = NS_NewNativeLocalFile(parentPath, PR_FALSE, getter_AddRefs(localFile));

    if(NS_SUCCEEDED(rv) && localFile)
    {
        return CallQueryInterface(localFile, aParent);
    }
    return rv;
}

NS_IMETHODIMP
nsLocalFile::Exists(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    MakeDirty();
    nsresult rv = Stat();

    *_retval = NS_SUCCEEDED(rv);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsWritable(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    APIRET rc;
    FILESTATUS3 pathInfo;

    
    rc = DosQueryPathInfo(mWorkingPath.get(), FIL_STANDARD,
                          &pathInfo, sizeof(pathInfo));

    if (rc != NO_ERROR)
    {
       rc = ConvertOS2Error(rc);
       return rc;
    }

    
    
    *_retval = !((pathInfo.attrFile & FILE_READONLY) != 0);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsReadable(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsExecutable(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    
    PRBool isFile;
    rv = IsFile(&isFile);
    if (NS_FAILED(rv) || !isFile)
        return rv;

    nsCAutoString path;
    GetNativeTarget(path);

    
    const char* leaf = (const char*) _mbsrchr((const unsigned char*)path.get(), '\\');
    if (!leaf)
        return NS_OK;

    
    

    char*   pathEnd = WinPrevChar(0, 0, 0, leaf, strchr(leaf, 0));
    while (pathEnd > leaf)
    {
        if (*pathEnd != ' ' && *pathEnd != '.')
            break;
        *pathEnd = 0;
        pathEnd = WinPrevChar(0, 0, 0, leaf, pathEnd);
    }

    if (pathEnd == leaf)
        return NS_OK;

    
    char* ext = (char*) _mbsrchr((const unsigned char*)leaf, '.');
    if (!ext)
        return NS_OK;

    
#ifdef MOZ_OS2_HIGH_MEMORY
    
    
    
    strupr(ext);
#else
    WinUpper(0, 0, 0, ext);
#endif
    if (strcmp(ext, ".EXE") == 0 ||
        strcmp(ext, ".CMD") == 0 ||
        strcmp(ext, ".COM") == 0 ||
        strcmp(ext, ".BAT") == 0)
        *_retval = PR_TRUE;

    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsDirectory(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = (mFileInfo64.type == PR_FILE_DIRECTORY);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsFile(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    *_retval = (mFileInfo64.type == PR_FILE_FILE);
    return rv;
}

NS_IMETHODIMP
nsLocalFile::IsHidden(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);
    *_retval = PR_FALSE;

    nsresult rv = Stat();
    if (NS_FAILED(rv))
        return rv;

    APIRET rc;
    FILESTATUS3 pathInfo;

    
    rc = DosQueryPathInfo(mWorkingPath.get(), FIL_STANDARD,
                          &pathInfo, sizeof(pathInfo));

    if (rc != NO_ERROR)
    {
       rc = ConvertOS2Error(rc);
       return rc;
    }

    *_retval = ((pathInfo.attrFile & FILE_HIDDEN) != 0);
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::IsSymlink(PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    NS_ENSURE_ARG_POINTER(_retval);

    
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::IsSpecial(PRBool *_retval)
{
    NS_ENSURE_ARG(_retval);

    
    *_retval = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Equals(nsIFile *inFile, PRBool *_retval)
{
    NS_ENSURE_ARG(inFile);
    NS_ENSURE_ARG(_retval);

    nsCAutoString inFilePath;
    inFile->GetNativePath(inFilePath);

    *_retval = inFilePath.Equals(mWorkingPath);
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::Contains(nsIFile *inFile, PRBool recur, PRBool *_retval)
{
    
    CHECK_mWorkingPath();

    *_retval = PR_FALSE;

    nsCAutoString myFilePath;
    if ( NS_FAILED(GetNativeTarget(myFilePath)))
        GetNativePath(myFilePath);

    PRInt32 myFilePathLen = myFilePath.Length();

    nsCAutoString inFilePath;
    if ( NS_FAILED(inFile->GetNativeTarget(inFilePath)))
        inFile->GetNativePath(inFilePath);

    if ( strnicmp( myFilePath.get(), inFilePath.get(), myFilePathLen) == 0)
    {
        
        

        if (inFilePath[myFilePathLen] == '\\')
        {
            *_retval = PR_TRUE;
        }

    }

    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetNativeTarget(nsACString &_retval)
{
    
    CHECK_mWorkingPath();

    _retval = mWorkingPath;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetFollowLinks(PRBool *aFollowLinks)
{
    NS_ENSURE_ARG(aFollowLinks);
    *aFollowLinks = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::SetFollowLinks(PRBool aFollowLinks)
{
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetDirectoryEntries(nsISimpleEnumerator * *entries)
{
    NS_ENSURE_ARG(entries);
    nsresult rv;
    *entries = nsnull;

    if (mWorkingPath.EqualsLiteral("\\\\.")) {
        nsDriveEnumerator *drives = new nsDriveEnumerator;
        if (!drives)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(drives);
        rv = drives->Init();
        if (NS_FAILED(rv)) {
            NS_RELEASE(drives);
            return rv;
        }
        *entries = drives;
        return NS_OK;
    }

    PRBool isDir;
    rv = IsDirectory(&isDir);
    if (NS_FAILED(rv))
        return rv;
    if (!isDir)
        return NS_ERROR_FILE_NOT_DIRECTORY;

    nsDirEnumerator* dirEnum = new nsDirEnumerator();
    if (dirEnum == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(dirEnum);
    rv = dirEnum->Init(this);
    if (NS_FAILED(rv))
    {
        NS_RELEASE(dirEnum);
        return rv;
    }

    *entries = dirEnum;
    return NS_OK;
}

NS_IMETHODIMP
nsLocalFile::GetPersistentDescriptor(nsACString &aPersistentDescriptor)
{
    return GetNativePath(aPersistentDescriptor);
}

NS_IMETHODIMP
nsLocalFile::SetPersistentDescriptor(const nsACString &aPersistentDescriptor)
{
    return InitWithNativePath(aPersistentDescriptor);
}

#ifndef OPEN_DEFAULT
#define OPEN_DEFAULT       0
#define OPEN_CONTENTS      1
#endif

NS_IMETHODIMP
nsLocalFile::Reveal()
{
    PRBool isDirectory = PR_FALSE;
    nsCAutoString path;

    IsDirectory(&isDirectory);
    if (isDirectory)
    {
        GetNativePath(path);
    }
    else
    {
        nsCOMPtr<nsIFile> parent;
        GetParent(getter_AddRefs(parent));
        if (parent)
            parent->GetNativePath(path);
    }

    HOBJECT hobject = WinQueryObject(path.get());
    WinSetFocus(HWND_DESKTOP, HWND_DESKTOP);
    WinOpenObject(hobject, OPEN_DEFAULT, TRUE);

    
    return NS_OK;
}


NS_IMETHODIMP
nsLocalFile::Launch()
{
  HOBJECT hobject = WinQueryObject(mWorkingPath.get());
  WinSetFocus(HWND_DESKTOP, HWND_DESKTOP);
  WinOpenObject(hobject, OPEN_DEFAULT, TRUE);

  
  return NS_OK;
}

nsresult
NS_NewNativeLocalFile(const nsACString &path, PRBool followLinks, nsILocalFile* *result)
{
    nsLocalFile* file = new nsLocalFile();
    if (file == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(file);

    if (!path.IsEmpty()) {
        nsresult rv = file->InitWithNativePath(path);
        if (NS_FAILED(rv)) {
            NS_RELEASE(file);
            return rv;
        }
    }

    *result = file;
    return NS_OK;
}





NS_IMETHODIMP
nsLocalFile::InitWithPath(const nsAString &filePath)
{
    if (filePath.IsEmpty())
        return InitWithNativePath(EmptyCString());

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(filePath, tmp);
    if (NS_SUCCEEDED(rv))
        return InitWithNativePath(tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::Append(const nsAString &node)
{
    if (node.IsEmpty())
        return NS_OK;

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(node, tmp);
    if (NS_SUCCEEDED(rv))
        return AppendNative(tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::AppendRelativePath(const nsAString &node)
{
    if (node.IsEmpty())
        return NS_OK;

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(node, tmp);
    if (NS_SUCCEEDED(rv))
        return AppendRelativeNativePath(tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetLeafName(nsAString &aLeafName)
{
    nsCAutoString tmp;
    nsresult rv = GetNativeLeafName(tmp);
    if (NS_SUCCEEDED(rv))
        rv = NS_CopyNativeToUnicode(tmp, aLeafName);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::SetLeafName(const nsAString &aLeafName)
{
    if (aLeafName.IsEmpty())
        return SetNativeLeafName(EmptyCString());

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(aLeafName, tmp);
    if (NS_SUCCEEDED(rv))
        return SetNativeLeafName(tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetPath(nsAString &_retval)
{
    return NS_CopyNativeToUnicode(mWorkingPath, _retval);
}

NS_IMETHODIMP
nsLocalFile::CopyTo(nsIFile *newParentDir, const nsAString &newName)
{
    if (newName.IsEmpty())
        return CopyToNative(newParentDir, EmptyCString());

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(newName, tmp);
    if (NS_SUCCEEDED(rv))
        return CopyToNative(newParentDir, tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::CopyToFollowingLinks(nsIFile *newParentDir, const nsAString &newName)
{
    if (newName.IsEmpty())
        return CopyToFollowingLinksNative(newParentDir, EmptyCString());

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(newName, tmp);
    if (NS_SUCCEEDED(rv))
        return CopyToFollowingLinksNative(newParentDir, tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::MoveTo(nsIFile *newParentDir, const nsAString &newName)
{
    if (newName.IsEmpty())
        return MoveToNative(newParentDir, EmptyCString());

    nsCAutoString tmp;
    nsresult rv = NS_CopyUnicodeToNative(newName, tmp);
    if (NS_SUCCEEDED(rv))
        return MoveToNative(newParentDir, tmp);

    return rv;
}

NS_IMETHODIMP
nsLocalFile::GetTarget(nsAString &_retval)
{
    nsCAutoString tmp;
    nsresult rv = GetNativeTarget(tmp);
    if (NS_SUCCEEDED(rv))
        rv = NS_CopyNativeToUnicode(tmp, _retval);

    return rv;
}



NS_IMETHODIMP
nsLocalFile::Equals(nsIHashable* aOther, PRBool *aResult)
{
    nsCOMPtr<nsIFile> otherfile(do_QueryInterface(aOther));
    if (!otherfile) {
        *aResult = PR_FALSE;
        return NS_OK;
    }

    return Equals(otherfile, aResult);
}

NS_IMETHODIMP
nsLocalFile::GetHashCode(PRUint32 *aResult)
{
    *aResult = nsCRT::HashCode(mWorkingPath.get());
    return NS_OK;
}

nsresult
NS_NewLocalFile(const nsAString &path, PRBool followLinks, nsILocalFile* *result)
{
    nsCAutoString buf;
    nsresult rv = NS_CopyUnicodeToNative(path, buf);
    if (NS_FAILED(rv)) {
        *result = nsnull;
        return rv;
    }
    return NS_NewNativeLocalFile(buf, followLinks, result);
}





void
nsLocalFile::GlobalInit()
{
}

void
nsLocalFile::GlobalShutdown()
{
}



