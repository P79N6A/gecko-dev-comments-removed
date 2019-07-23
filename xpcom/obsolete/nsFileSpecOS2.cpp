



















































#define INCL_DOSERRORS
#define INCL_DOS
#define INCL_WINWORKPLACE
#include <os2.h>

#ifdef XP_OS2_VACPP
#include <direct.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <ctype.h>
#include <io.h>


void nsFileSpecHelpers::Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs)




{
    if (ioPath.IsEmpty())
        return;
  
    NS_ASSERTION(strchr((const char*)ioPath, '/') == 0,
		"This smells like a Unix path. Native path expected! "
		"Please fix.");
	if (inMakeDirs)
    {
        const int mode = 0700;
        nsSimpleCharString unixStylePath = ioPath;
        nsFileSpecHelpers::NativeToUnix(unixStylePath);
        nsFileSpecHelpers::MakeAllDirectories((const char*)unixStylePath, mode);
    }
    char buffer[_MAX_PATH];
    errno = 0;
    *buffer = '\0';
#ifdef XP_OS2
    PRBool removedBackslash = PR_FALSE;
    PRUint32 lenstr = ioPath.Length();
    char &lastchar = ioPath[lenstr -1];

    
    
    
    if( lastchar == '\\' && (lenstr != 3 || ioPath[1] != ':') && lenstr != 1)
    {
       lastchar = '\0';
       removedBackslash = PR_TRUE;
    }

    char canonicalPath[CCHMAXPATH] = "";

    DosQueryPathInfo( (char*) ioPath, 
                                          FIL_QUERYFULLNAME,
                                          canonicalPath, 
                                          CCHMAXPATH);
#else
    char* canonicalPath = _fullpath(buffer, ioPath, _MAX_PATH);
#endif

	if (canonicalPath)
	{
		NS_ASSERTION( canonicalPath[0] != '\0', "Uh oh...couldn't convert" );
		if (canonicalPath[0] == '\0')
			return;
#ifdef XP_OS2
                
                if (removedBackslash)
                   strcat( canonicalPath, "\\");
#endif
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

	src = (char*)ioPath;
		
    if (*src) {
	    
	    while (*++src)
        {
            if (*src == '/')
                *src = '\\';
        }
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
        temp[0] = '\0'; 
	
	
	for (; *cp; cp++)
    {
#ifdef XP_OS2
      
#else
      if(IsDBCSLeadByte(*cp) && *(cp+1) != nsnull)
      {
         cp++;
         continue;
      }
#endif
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
#ifdef XP_OS2
  return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (  S_IFREG & st.st_mode);
#else
  return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (_S_IFREG & st.st_mode);
#endif
} 


PRBool nsFileSpec::IsDirectory() const

{
	struct stat st;
#ifdef XP_OS2
        return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (  S_IFDIR & st.st_mode);
#else
	return !mPath.IsEmpty() && 0 == stat(nsNSPRPath(*this), &st) && (_S_IFDIR & st.st_mode);
#endif
} 


PRBool nsFileSpec::IsHidden() const

{
    PRBool hidden = PR_FALSE;
    if (!mPath.IsEmpty())
    {
#ifdef XP_OS2
        FILESTATUS3 fs3;
        APIRET rc;

        rc = DosQueryPathInfo( mPath,
                                         FIL_STANDARD, 
                                         &fs3,
                                         sizeof fs3);
        if(!rc)
          hidden = fs3.attrFile & FILE_HIDDEN ? PR_TRUE : PR_FALSE;
#else
        DWORD attr = GetFileAttributes(mPath);
        if (FILE_ATTRIBUTE_HIDDEN & attr)
            hidden = PR_TRUE;
#endif
    }
    return hidden;
}



PRBool nsFileSpec::IsSymlink() const

{
#ifdef XP_OS2
    return PR_FALSE;                
#else
    HRESULT hres; 
    IShellLink* psl; 
    
    PRBool isSymlink = PR_FALSE;
    
    CoInitialize(NULL);
    
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl); 
    if (SUCCEEDED(hres)) 
    { 
        IPersistFile* ppf; 
        
        
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf); 
        
        if (SUCCEEDED(hres)) 
        {
            WORD wsz[MAX_PATH]; 
            
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

    return isSymlink;
#endif
}



nsresult nsFileSpec::ResolveSymlink(PRBool& wasSymlink)

{
#ifdef XP_OS2
    return NS_OK;           
#else
    wasSymlink = PR_FALSE;  

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
            WORD wsz[MAX_PATH]; 
            
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
#endif
}




void nsFileSpec::GetParent(nsFileSpec& outSpec) const

{
	outSpec.mPath = mPath;
	char* chars = (char*)outSpec.mPath;
	chars[outSpec.mPath.Length() - 1] = '\0'; 
    char* cp = strrchr(chars, '\\');
    if (cp++)
	    outSpec.mPath.SetLength(cp - chars); 
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
#ifdef XP_OS2
	    
            PR_MkDir(nsNSPRPath(*this), PR_CREATE_FILE);
#else
	    mkdir(nsNSPRPath(*this));
#endif
} 


void nsFileSpec::Delete(PRBool inRecursive) const

{
    if (IsDirectory())
    {
	    if (inRecursive)
        {
            for (nsDirectoryIterator i(*this, PR_FALSE); i.Exists(); i++)
                {
                    nsFileSpec& child = i.Spec();
                    child.Delete(inRecursive);
                }		
        }
#ifdef XP_OS2
            
            PR_RmDir(nsNSPRPath(*this));
#else
	    rmdir(nsNSPRPath(*this));
#endif
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
			nsFileSpec& child = i.Spec();

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
#ifdef XP_OS2
    APIRET rc;
    HFILE hFile;
    ULONG actionTaken;

    rc = DosOpen(mPath,
                       &hFile,
                       &actionTaken,
                       0,                 
                       FILE_NORMAL,
                       OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                       OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
                       NULL);
                                 
    if (rc != NO_ERROR)
        return NS_FILE_FAILURE;

    rc = DosSetFileSize(hFile, aNewFileLength);

    if (rc == NO_ERROR) 
        DosClose(hFile);
    else
        goto error; 
#else
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
#endif

    return NS_OK;

 error:
#ifdef XP_OS2
    DosClose(hFile);
#else
    CloseHandle(hFile);
#endif
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
        
        
#ifdef XP_OS2
        APIRET rc;
        PRBool copyOK;

        rc = DosCopy(GetCString(), (PSZ)destPath, DCPY_EXISTING);

        if (rc == NO_ERROR)
            copyOK = PR_TRUE;
        else
            copyOK = PR_FALSE;
#else
        int copyOK = CopyFile(GetCString(), destPath, PR_TRUE);
#endif
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

        if (DosMove(GetCString(), destPath) == NO_ERROR)
        {
            *this = inNewParentDirectory + GetLeafName(); 
            return NS_OK;
        }
        
    }
    return NS_FILE_FAILURE;
} 


nsresult nsFileSpec::Execute(const char* inArgs ) const

{    
    if (!IsDirectory())
    {
#ifdef XP_OS2
        nsresult result = NS_FILE_FAILURE;
    
        if (!mPath.IsEmpty())
        {
            nsSimpleCharString fileNameWithArgs = mPath + " " + inArgs;   
            result = NS_FILE_RESULT(system(fileNameWithArgs));
        } 
        return result;
#else
        nsSimpleCharString fileNameWithArgs = "\"";
        fileNameWithArgs += mPath + "\" " + inArgs;
        int execResult = WinExec( fileNameWithArgs, SW_NORMAL );     
        if (execResult > 31)
            return NS_OK;
#endif
    }
    return NS_FILE_FAILURE;
} 



PRInt64 nsFileSpec::GetDiskSpaceAvailable() const

{
    PRInt64 nBytes = 0;
    ULONG ulDriveNo = toupper(mPath[0]) + 1 - 'A';
    FSALLOCATE fsAllocate;
    APIRET rc = DosQueryFSInfo(ulDriveNo,
                               FSIL_ALLOC,
                               &fsAllocate,
                               sizeof(fsAllocate));

    if (rc == NO_ERROR) {
       nBytes = fsAllocate.cUnitAvail;
       nBytes *= fsAllocate.cSectorUnit;
       nBytes *= fsAllocate.cbSector;
    }

    return nBytes;
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

