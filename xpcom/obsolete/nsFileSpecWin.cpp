








































#include <sys/stat.h>
#include <direct.h>
#include <limits.h>
#include <stdlib.h>
#include "prio.h"
#include "nsError.h"

#include <windows.h>
#include <mbstring.h>

#if (_MSC_VER == 1100) || defined(__GNUC__)
#define INITGUID
#include <objbase.h>
DEFINE_OLEGUID(IID_IPersistFile, 0x0000010BL, 0, 0);
#endif

#include <shlobj.h>
#include <shellapi.h>
#include <shlguid.h>

#ifndef WINCE
#ifdef UNICODE
#define CreateDirectoryW  CreateDirectory
#else
#define CreateDirectoryA  CreateDirectory
#endif 
#endif


void nsFileSpecHelpers::Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs)




{
    if (ioPath.IsEmpty())
        return;
  
    NS_ASSERTION(strchr((const char*)ioPath, '/') == 0,
		"This smells like a Unix path. Native path expected! "
		"Please fix.");
	if (inMakeDirs)
    {
        const int mode = 0755;
        nsSimpleCharString unixStylePath = ioPath;
        nsFileSpecHelpers::NativeToUnix(unixStylePath);
        nsFileSpecHelpers::MakeAllDirectories((const char*)unixStylePath, mode);
    }
    char buffer[_MAX_PATH];
    errno = 0;
    *buffer = '\0';
    char* canonicalPath = _fullpath(buffer, ioPath, _MAX_PATH);

	if (canonicalPath)
	{
		NS_ASSERTION( canonicalPath[0] != '\0', "Uh oh...couldn't convert" );
		if (canonicalPath[0] == '\0')
			return;
	}
    ioPath = canonicalPath;
} 


void nsFileSpecHelpers::UnixToNative(nsSimpleCharString& ioPath)



{
	
	
	
	if (ioPath.IsEmpty())
		return;
		
  
	char* src = (char*)ioPath;
  if (*src == '/') {
    if (!src[1]) {
      
      nsSimpleCharString temp = src + 1;
      ioPath = temp;
      return;
    }
	  
		char* colonPointer = src + 2;
		if (strstr(src, "|/") == colonPointer)
	    *colonPointer = ':';
	  
	  nsSimpleCharString temp = src + 1;
	  ioPath = temp;
	}

    
    
    for (src = (char*)ioPath; *src; ++src)
    {
        if (*src == '/')
            *src = '\\';
    }

} 


void nsFileSpecHelpers::NativeToUnix(nsSimpleCharString& ioPath)



{
	if (ioPath.IsEmpty())
		return;
		
	
	nsSimpleCharString temp("/");

	char* cp = (char*)ioPath + 1;
	if (strstr(cp, ":\\") == cp)
		*cp = '|';    
    else
      if (cp[0] == '\\')	
        cp--;
        else
        temp[0] = '\0'; 
	
	
	for (; *cp; cp++)
    {
      if(IsDBCSLeadByte(*cp) && *(cp+1) != nsnull)
      {
         cp++;
         continue;
      }
      if (*cp == '\\')
        *cp = '/';
    }
	
	temp += ioPath;
	ioPath = temp;
}


nsFileSpec::nsFileSpec(const nsFilePath& inPath)

{

	*this = inPath;
}


void nsFileSpec::operator = (const nsFilePath& inPath)

{
	mPath = (const char*)inPath;
	nsFileSpecHelpers::UnixToNative(mPath);
	mError = NS_OK;
} 


nsFilePath::nsFilePath(const nsFileSpec& inSpec)

{
	*this = inSpec;
} 


void nsFilePath::operator = (const nsFileSpec& inSpec)

{
	mPath = inSpec.mPath;
	nsFileSpecHelpers::NativeToUnix(mPath);
} 


void nsFileSpec::SetLeafName(const char* inLeafName)

{
	NS_ASSERTION(inLeafName, "Attempt to SetLeafName with a null string");
	mPath.LeafReplace('\\', inLeafName);
} 


char* nsFileSpec::GetLeafName() const

{
    return mPath.GetLeaf('\\');
} 


PRBool nsFileSpec::Exists() const

{
	struct stat st;
	return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st); 
} 


void nsFileSpec::GetModDate(TimeStamp& outStamp) const

{
	struct stat st;
    if (!mPath.IsEmpty() && stat(nsNSPRPath(*this), &st) == 0) 
        outStamp = st.st_mtime; 
    else
        outStamp = 0;
} 


PRUint32 nsFileSpec::GetFileSize() const

{
	struct stat st;
    if (!mPath.IsEmpty() && stat(nsNSPRPath(*this), &st) == 0) 
        return (PRUint32)st.st_size; 
    return 0;
} 


PRBool nsFileSpec::IsFile() const

{
  struct stat st;
  return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (_S_IFREG & st.st_mode);
} 


PRBool nsFileSpec::IsDirectory() const

{
	struct stat st;
	return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (_S_IFDIR & st.st_mode);
} 


PRBool nsFileSpec::IsHidden() const

{
    PRBool hidden = PR_FALSE;
    if (!mPath.IsEmpty())
    {
        DWORD attr = GetFileAttributes(mPath);
        if (FILE_ATTRIBUTE_HIDDEN & attr)
            hidden = PR_TRUE;
    }
    return hidden;
}



PRBool nsFileSpec::IsSymlink() const

{
    HRESULT hres; 
    IShellLink* psl; 
    
    PRBool isSymlink = PR_FALSE;
#ifndef WINCE
    CoInitialize(NULL);
    
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
        
        
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
        
        if (SUCCEEDED(hres)) 
        {
            WCHAR wsz[MAX_PATH]; 
            
            MultiByteToWideChar(CP_ACP, 0, mPath, -1, wsz, MAX_PATH); 
 
            
            hres = ppf->Load(wsz, STGM_READ); 
            if (SUCCEEDED(hres)) 
            {
                isSymlink = PR_TRUE;
            }
            
            
            ppf->Release(); 
        }
        
        
        psl->Release();
    }

    CoUninitialize();
#endif
    return isSymlink;
}



nsresult nsFileSpec::ResolveSymlink(PRBool& wasSymlink)

{
    wasSymlink = PR_FALSE;  
#ifndef WINCE
	if (Exists())
		return NS_OK;


    HRESULT hres; 
    IShellLink* psl; 

    CoInitialize(NULL);

    
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
        
        
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
        
        if (SUCCEEDED(hres)) 
        {
            WCHAR wsz[MAX_PATH]; 
            
            MultiByteToWideChar(CP_ACP, 0, mPath, -1, wsz, MAX_PATH); 
 
            
            hres = ppf->Load(wsz, STGM_READ); 
            if (SUCCEEDED(hres)) 
            {
                wasSymlink = PR_TRUE;

                
                hres = psl->Resolve(nsnull, SLR_NO_UI ); 
                if (SUCCEEDED(hres)) 
                { 
                    char szGotPath[MAX_PATH]; 
                    WIN32_FIND_DATA wfd; 

                    
                    hres = psl->GetPath( szGotPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY ); 

                    if (SUCCEEDED(hres))
                    {
                        
                        mPath = szGotPath;
                        mError = NS_OK;
                    }
                } 
            }
            else {
                
                hres = 0;
            }

            
            ppf->Release(); 
        }
        
        psl->Release();
    }

    CoUninitialize();

    if (SUCCEEDED(hres))
        return NS_OK;

    return NS_FILE_FAILURE;
#else
    return NS_OK;
#endif 
}




void nsFileSpec::GetParent(nsFileSpec& outSpec) const

{
  outSpec.mPath = mPath;
  char* chars = (char*)outSpec.mPath;
  chars[outSpec.mPath.Length() - 1] = '\0'; 
  unsigned char* cp = _mbsrchr((unsigned char*)chars, '\\');
  if (cp++)
    outSpec.mPath.SetLength(cp - (unsigned char*)chars); 
} 


void nsFileSpec::operator += (const char* inRelativePath)

{
	NS_ASSERTION(inRelativePath, "Attempt to do += with a null string");

	if (!inRelativePath || mPath.IsEmpty())
		return;
	
	if (mPath[mPath.Length() - 1] == '\\')
		mPath += "x";
	else
		mPath += "\\x";
	
	
	nsSimpleCharString dosPath = inRelativePath;
	nsFileSpecHelpers::UnixToNative(dosPath);
	SetLeafName(dosPath);
} 


void nsFileSpec::CreateDirectory(int )

{
	
	if (!mPath.IsEmpty())
	    mkdir(nsNSPRPath(*this));
} 


void nsFileSpec::Delete(PRBool inRecursive) const

{
    if (IsDirectory())
    {
	    if (inRecursive)
        {
            for (nsDirectoryIterator i(*this, PR_FALSE); i.Exists(); i++)
                {
                    nsFileSpec& child = (nsFileSpec&)i;
                    child.Delete(inRecursive);
                }		
        }
	    rmdir(nsNSPRPath(*this));
    }
	else if (!mPath.IsEmpty())
    {
        remove(nsNSPRPath(*this));
    }
} 



void nsFileSpec::RecursiveCopy(nsFileSpec newDir) const

{
    if (IsDirectory())
    {
		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

		for (nsDirectoryIterator i(*this, PR_FALSE); i.Exists(); i++)
		{
			nsFileSpec& child = (nsFileSpec&)i;

			if (child.IsDirectory())
			{
				nsFileSpec tmpDirSpec(newDir);

				char *leafname = child.GetLeafName();
				tmpDirSpec += leafname;
				nsCRT::free(leafname);

				child.RecursiveCopy(tmpDirSpec);
			}
			else
			{
   				child.RecursiveCopy(newDir);
			}
		}
    }
    else if (!mPath.IsEmpty())
    {
		nsFileSpec& filePath = (nsFileSpec&) *this;

		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

        filePath.CopyToDir(newDir);
    }
} 


nsresult
nsFileSpec::Truncate(PRInt32 aNewFileLength) const

{
    DWORD status;
    HANDLE hFile;

    
    
    hFile = CreateFile(mPath,
                       GENERIC_WRITE, 
                       FILE_SHARE_READ, 
                       NULL, 
                       OPEN_EXISTING, 
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL); 
    if (hFile == INVALID_HANDLE_VALUE)
        return NS_FILE_FAILURE;

    
    status = SetFilePointer(hFile, aNewFileLength, NULL, FILE_BEGIN);
    if (status == 0xffffffff)
        goto error;

    
    if (!SetEndOfFile(hFile))
        goto error;

    if (!CloseHandle(hFile))
        return NS_FILE_FAILURE;

    return NS_OK;

 error:
    CloseHandle(hFile);
    return NS_FILE_FAILURE;

} 


nsresult nsFileSpec::Rename(const char* inNewName)

{
	NS_ASSERTION(inNewName, "Attempt to Rename with a null string");

    
    if (strchr(inNewName, '/')) 
        return NS_FILE_FAILURE;

    char* oldPath = nsCRT::strdup(mPath);
    
    SetLeafName(inNewName);        

    if (PR_Rename(oldPath, mPath) != NS_OK)
    {
        
        mPath = oldPath;
        return NS_FILE_FAILURE;
    }
    
    nsCRT::free(oldPath);
    
    return NS_OK;
} 


nsresult nsFileSpec::CopyToDir(const nsFileSpec& inParentDirectory) const

{
    
    if (inParentDirectory.IsDirectory() && (! IsDirectory() ) )
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inParentDirectory.GetCString());
        destPath += "\\";
        destPath += leafname;
        nsCRT::free(leafname);
        
        
        int copyOK = CopyFile(GetCString(), destPath, PR_TRUE);
        if (copyOK)
            return NS_OK;
    }
    return NS_FILE_FAILURE;
} 


nsresult nsFileSpec::MoveToDir(const nsFileSpec& inNewParentDirectory)

{
    
    if (inNewParentDirectory.IsDirectory() && (! IsDirectory() ) )
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inNewParentDirectory.GetCString());
        destPath += "\\";
        destPath += leafname;
        nsCRT::free(leafname);

        
        int copyOK = MoveFile(GetCString(), destPath);

        if (copyOK)
        {
            *this = inNewParentDirectory + GetLeafName(); 
            return NS_OK;
        }
        
    }
    return NS_FILE_FAILURE;
} 


nsresult nsFileSpec::Execute(const char* inArgs ) const

{    
#ifndef WINCE
    if (!IsDirectory())
    {
        nsSimpleCharString fileNameWithArgs = "\"";
        fileNameWithArgs += mPath + "\" " + inArgs;
        int execResult = WinExec( fileNameWithArgs, SW_NORMAL );     
        if (execResult > 31)
            return NS_OK;
    }
#endif
    return NS_FILE_FAILURE;
} 



PRInt64 nsFileSpec::GetDiskSpaceAvailable() const

{
#ifndef WINCE
    PRInt64 int64;
    
    LL_I2L(int64 , LONG_MAX);

    char aDrive[_MAX_DRIVE + 2];
	_splitpath( (const char*)mPath, aDrive, NULL, NULL, NULL);

	if (aDrive[0] == '\0')
	{
        
        
        if (mPath.Length() > 2 && mPath[0] == '/' && mPath[2] == '|')
        {
            aDrive[0] = mPath[1];
            aDrive[1] = ':';
            aDrive[2] = '\0';
        }
        else
        {
            
            return int64; 
        }
    }

	strcat(aDrive, "\\");

    
    DWORD dwSecPerClus, dwBytesPerSec, dwFreeClus, dwTotalClus;
    ULARGE_INTEGER liFreeBytesAvailableToCaller, liTotalNumberOfBytes, liTotalNumberOfFreeBytes;
    double nBytes = 0;

    BOOL (WINAPI* getDiskFreeSpaceExA)(LPCTSTR lpDirectoryName, 
                                       PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                       PULARGE_INTEGER lpTotalNumberOfBytes,    
                                       PULARGE_INTEGER lpTotalNumberOfFreeBytes) = NULL;

    HINSTANCE hInst = LoadLibrary("KERNEL32.DLL");
    NS_ASSERTION(hInst != NULL, "COULD NOT LOAD KERNEL32.DLL");
    if (hInst != NULL)
    {
        getDiskFreeSpaceExA =  (BOOL (WINAPI*)(LPCTSTR lpDirectoryName, 
                                               PULARGE_INTEGER lpFreeBytesAvailableToCaller,
                                               PULARGE_INTEGER lpTotalNumberOfBytes,    
                                               PULARGE_INTEGER lpTotalNumberOfFreeBytes)) 
        GetProcAddress(hInst, "GetDiskFreeSpaceExA");
        FreeLibrary(hInst);
    }

    if (getDiskFreeSpaceExA && (*getDiskFreeSpaceExA)(aDrive,
                                                      &liFreeBytesAvailableToCaller, 
                                                      &liTotalNumberOfBytes,  
                                                      &liTotalNumberOfFreeBytes))
    {
        nBytes = (double)(signed __int64)liFreeBytesAvailableToCaller.QuadPart;
    }
    else if ( GetDiskFreeSpace(aDrive, &dwSecPerClus, &dwBytesPerSec, &dwFreeClus, &dwTotalClus))
    {
        nBytes = (double)dwFreeClus*(double)dwSecPerClus*(double) dwBytesPerSec;
    }
    return (PRInt64)nBytes;
#else
    return (PRInt64)0;
#endif
}








nsDirectoryIterator::nsDirectoryIterator(const nsFileSpec& inDirectory, PRBool resolveSymlink)

	: mCurrent(inDirectory)
	, mDir(nsnull)
    , mStarting(inDirectory)
	, mExists(PR_FALSE)
    , mResoveSymLinks(resolveSymlink)
{
    mDir = PR_OpenDir(inDirectory);
	mCurrent += "dummy";
    mStarting += "dummy";
    ++(*this);
} 


nsDirectoryIterator::~nsDirectoryIterator()

{
    if (mDir)
	    PR_CloseDir(mDir);
} 


nsDirectoryIterator& nsDirectoryIterator::operator ++ ()

{
	mExists = PR_FALSE;
	if (!mDir)
		return *this;
    PRDirEntry* entry = PR_ReadDir(mDir, PR_SKIP_BOTH); 
	if (entry)
    {
      mExists = PR_TRUE;
      mCurrent = mStarting;
      mCurrent.SetLeafName(entry->name);
      if (mResoveSymLinks)
      {   
          PRBool ignore;
          mCurrent.ResolveSymlink(ignore);
      }
    }
	return *this;
} 


nsDirectoryIterator& nsDirectoryIterator::operator -- ()

{
	return ++(*this); 
} 

