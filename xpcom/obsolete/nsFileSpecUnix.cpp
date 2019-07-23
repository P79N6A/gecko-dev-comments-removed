





































 



#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "xpcom-private.h"
#include "nsError.h"
#include "prio.h"   
#include "nsAutoBuffer.h"

#if defined(_SCO_DS)
#define _SVID3
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#undef Free
#endif

#ifdef HAVE_STATVFS
#define STATFS	statvfs
#else
#define STATFS	statfs
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN	1024  /* Guessing this is okay.  Works for SCO. */
#endif
 
#if defined(__QNX__)
#include <unix.h>	
#define f_bavail	f_bfree
extern "C" int truncate(const char *, off_t);
#endif

#if defined(SUNOS4)
extern "C" int statfs(char *, struct statfs *);
#endif

#if defined(OSF1)
extern "C" int statvfs(const char *, struct statvfs *);
#endif

#ifdef XP_MACOSX
static void  CopyUTF8toUTF16NFC(const nsACString& aSrc, nsAString& aResult);
#endif


void nsFileSpecHelpers::Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs)


{
    if (ioPath.IsEmpty())
        return;
    if (inMakeDirs)
    {
        const mode_t mode = 0755;
        nsFileSpecHelpers::MakeAllDirectories((const char*)ioPath, mode);
    }

    errno = 0;  

    if (ioPath[0] != '/')
    {
        
        char buffer[MAXPATHLEN];

        (void) getcwd(buffer, MAXPATHLEN);

        strcat(buffer, "/");
        strcat(buffer, ioPath);

        ioPath = buffer;
    }
} 


void nsFileSpec::SetLeafName(const char* inLeafName)

{
    mPath.LeafReplace('/', inLeafName);
} 


char* nsFileSpec::GetLeafName() const

{
#ifndef XP_MACOSX
    return mPath.GetLeaf('/');
#else
    char *name = mPath.GetLeaf('/');
    if (!name || !*name)
        return name;
    nsAutoString nameInNFC;
    CopyUTF8toUTF16NFC(nsDependentCString(name), nameInNFC);
    nsCRT::free(name);
    return nsCRT::strdup(NS_ConvertUTF16toUTF8(nameInNFC).get());
#endif
} 


PRBool nsFileSpec::Exists() const

{
    return !mPath.IsEmpty() && 0 == access(mPath, F_OK); 
} 


void nsFileSpec::GetModDate(TimeStamp& outStamp) const

{
	struct stat st;
    if (!mPath.IsEmpty() && stat(mPath, &st) == 0) 
        outStamp = st.st_mtime; 
    else
        outStamp = 0;
} 


PRUint32 nsFileSpec::GetFileSize() const

{
	struct stat st;
    if (!mPath.IsEmpty() && stat(mPath, &st) == 0) 
        return (PRUint32)st.st_size; 
    return 0;
} 


PRBool nsFileSpec::IsFile() const

{
    struct stat st;
    return !mPath.IsEmpty() && stat(mPath, &st) == 0 && S_ISREG(st.st_mode); 
} 


PRBool nsFileSpec::IsDirectory() const

{
    struct stat st;
    return !mPath.IsEmpty() && 0 == stat(mPath, &st) && S_ISDIR(st.st_mode); 
} 


PRBool nsFileSpec::IsHidden() const

{
    PRBool hidden = PR_FALSE;
    char *leafname = GetLeafName();
    if (nsnull != leafname)
    {
	

	
	if (leafname[0] == '.')
        {
            hidden = PR_TRUE;
        }
        nsCRT::free(leafname);
    }
    return hidden;
} 


PRBool nsFileSpec::IsSymlink() const

{
    struct stat st;
    if (!mPath.IsEmpty() && stat(mPath, &st) == 0 && S_ISLNK(st.st_mode))
        return PR_TRUE;

    return PR_FALSE;
} 


nsresult nsFileSpec::ResolveSymlink(PRBool& wasAliased)

{
    wasAliased = PR_FALSE;

    char resolvedPath[MAXPATHLEN];
    int charCount = readlink(mPath, (char*)&resolvedPath, MAXPATHLEN);
    if (0 < charCount)
    {
        if (MAXPATHLEN > charCount)
            resolvedPath[charCount] = '\0';
        
        wasAliased = PR_TRUE;

	
  
        if (resolvedPath[0] != '/') {
		SetLeafName(resolvedPath);
        }
        else {
        	mPath = (char*)&resolvedPath;
        }
	
	char* canonicalPath = realpath((const char *)mPath, resolvedPath);
	NS_ASSERTION(canonicalPath, "realpath failed");
	if (canonicalPath) {
		mPath = (char*)&resolvedPath;
	}
	else {
		return NS_ERROR_FAILURE;
	}
    }
    
    return NS_OK;
} 



void nsFileSpec::GetParent(nsFileSpec& outSpec) const

{
    outSpec.mPath = mPath;
	char* chars = (char*)outSpec.mPath;
	chars[outSpec.mPath.Length() - 1] = '\0'; 
    char* cp = strrchr(chars, '/');
    if (cp++)
	    outSpec.mPath.SetLength(cp - chars); 
} 


void nsFileSpec::operator += (const char* inRelativePath)

{
	NS_ASSERTION(inRelativePath, "Attempt to do += with a null string");

    if (!inRelativePath || mPath.IsEmpty())
        return;
    
    char endChar = mPath[(int)(strlen(mPath) - 1)];
    if (endChar == '/')
        mPath += "x";
    else
        mPath += "/x";
    SetLeafName(inRelativePath);
} 


void nsFileSpec::CreateDirectory(int mode)

{
    
    if (mPath.IsEmpty())
        return;
    mkdir(mPath, mode);
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
        rmdir(mPath);
    }
    else if (!mPath.IsEmpty())
        remove(mPath);
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



nsresult nsFileSpec::Truncate(PRInt32 offset) const

{
    char* Path = nsCRT::strdup(mPath);

    int rv = truncate(Path, offset) ;

    nsCRT::free(Path) ;

    if(!rv) 
        return NS_OK ;
    else
        return NS_ERROR_FAILURE ;
} 


nsresult nsFileSpec::Rename(const char* inNewName)

{
	NS_ASSERTION(inNewName, "Attempt to Rename with a null string");

    
    if (mPath.IsEmpty() || strchr(inNewName, '/')) 
        return NS_FILE_FAILURE;

    char* oldPath = nsCRT::strdup(mPath);
    
    SetLeafName(inNewName); 

    if (PR_Rename(oldPath, mPath) != NS_OK)
    {
        
        mPath = oldPath;
        nsCRT::free(oldPath);
        return NS_FILE_FAILURE;
    }
    
    nsCRT::free(oldPath);

    return NS_OK;
} 


static int CrudeFileCopy_DoCopy(PRFileDesc * pFileDescIn, PRFileDesc * pFileDescOut, char * pBuf, long bufferSize)

{
  PRInt32 rbytes, wbytes;                       

  
  
  
  
  rbytes = PR_Read(pFileDescIn, pBuf, bufferSize);
  if (rbytes < 0)                              
  {                                            
    return -1;                                 
  }

  while (rbytes > 0)                          
  {
    
    
    
    
    wbytes = PR_Write(pFileDescOut, pBuf, rbytes);   
    if (wbytes != rbytes)                    
    {                                        
      return -1;                             
    }

    
    rbytes = PR_Read(pFileDescIn, pBuf, bufferSize);
    if (rbytes < 0)                              
    {                                            
      return -1;                                 
    }
  }

  return 0;                          
} 


static int CrudeFileCopy(const char* in, const char* out)

{
  char buf[1024];                               
  PRInt32 rbytes, wbytes;                       
  struct stat in_stat;                          
  int stat_result = -1, ret;                 
  PRFileDesc * pFileDescIn, * pFileDescOut;     

  
  NS_ASSERTION(in && out, "CrudeFileCopy should be called with pointers to filenames...");

  
  if (in == out)
    return 0;

  
  if (strcmp(in,out) == 0)
    return 0;

  
  stat_result = stat(in, &in_stat);
  if(stat_result != 0)
  {
    
    
    return -1;                      
  }

  
  pFileDescIn = PR_Open(in, PR_RDONLY, 0444);
  if (pFileDescIn == 0)
  {
    return -1;                                  
  }  

  
  pFileDescOut = PR_Open(out, PR_WRONLY | PR_CREATE_FILE, 0600);
  if (pFileDescOut == 0)
  {
    
    PR_Close(pFileDescOut);                    
    return -1;                                 
  }  

  
  if (CrudeFileCopy_DoCopy(pFileDescIn, pFileDescOut, buf, sizeof(buf)) != 0)
  {                                            
    PR_Close(pFileDescOut);                    
    PR_Close(pFileDescIn);                     
    PR_Delete(out);                            
    return -1;                                 
  }

  
  
  
  PR_Close(pFileDescIn);            

  
  
  if (PR_Sync(pFileDescOut) != PR_SUCCESS) 
  {                                  
    PR_Close(pFileDescOut);          
    PR_Delete(out);                  
    return -1;                       
  }

  
  if (PR_Close(pFileDescOut) != PR_SUCCESS) 
  {                                  
    PR_Delete(out);                  
    return -1;                       
  }
    
  ret = chmod(out, in_stat.st_mode & 0777); 
  if (ret != 0)                      
  {
    PR_Delete(out);                  
    return -1;                       
  }

  return 0;                          
} 


nsresult nsFileSpec::CopyToDir(const nsFileSpec& inParentDirectory) const

{
    
    nsresult result = NS_FILE_FAILURE;

    if (inParentDirectory.IsDirectory() && (! IsDirectory() ) )
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inParentDirectory.GetCString());
        destPath += "/";
        destPath += leafname;
        nsCRT::free(leafname);
        result = NS_FILE_RESULT(CrudeFileCopy(GetCString(), destPath));
    }
    return result;
} 


nsresult nsFileSpec::MoveToDir(const nsFileSpec& inNewParentDirectory)

{
    
    nsresult result = NS_FILE_FAILURE;

    if (inNewParentDirectory.IsDirectory() && !IsDirectory())
    {
        char *leafname = GetLeafName();
        nsSimpleCharString destPath(inNewParentDirectory.GetCString());
        destPath += "/";
        destPath += leafname;
        nsCRT::free(leafname);

        result = NS_FILE_RESULT(CrudeFileCopy(GetCString(), (const char*)destPath));
        if (result == NS_OK)
        {
            
            ((nsFileSpec*)this)->Delete(PR_FALSE);
        
            *this = inNewParentDirectory + GetLeafName(); 
        }
    }
    return result;
} 


nsresult nsFileSpec::Execute(const char* inArgs ) const

{
    nsresult result = NS_FILE_FAILURE;
    
    if (!mPath.IsEmpty() && !IsDirectory())
    {
        nsSimpleCharString fileNameWithArgs = mPath + " " + inArgs;
        result = NS_FILE_RESULT(system(fileNameWithArgs));
    } 

    return result;

} 


PRInt64 nsFileSpec::GetDiskSpaceAvailable() const

{
    PRInt64 bytes; 
    LL_I2L(bytes , LONG_MAX); 


#if defined(HAVE_SYS_STATFS_H) || defined(HAVE_SYS_STATVFS_H)

    char curdir [MAXPATHLEN];
    if (mPath.IsEmpty())
    {
        (void) getcwd(curdir, MAXPATHLEN);
        if (!curdir)
            return bytes;  
    }
    else
        sprintf(curdir, "%.200s", (const char*)mPath);
 
    struct STATFS fs_buf;
#if defined(__QNX__) && !defined(HAVE_STATVFS) 
    if (STATFS(curdir, &fs_buf, 0, 0) < 0)
#else
    if (STATFS(curdir, &fs_buf) < 0)
#endif
        return bytes; 
 
#ifdef DEBUG_DISK_SPACE
    printf("DiskSpaceAvailable: %d bytes\n", 
       fs_buf.f_bsize * (fs_buf.f_bavail - 1));
#endif

    PRInt64 bsize,bavail;
    LL_I2L( bsize,  fs_buf.f_bsize );
    LL_I2L( bavail, fs_buf.f_bavail - 1 );
    LL_MUL( bytes, bsize, bavail );
    return bytes;

#else 
    



    return bytes;
#endif 

} 






nsDirectoryIterator::nsDirectoryIterator(const nsFileSpec& inDirectory, PRBool resolveSymLinks)

    : mCurrent(inDirectory)
    , mExists(PR_FALSE)
    , mResoveSymLinks(resolveSymLinks)
    , mStarting(inDirectory)
    , mDir(nsnull)

{
    mStarting += "sysygy"; 
    mCurrent += "sysygy"; 
    mDir = opendir((const char*)nsFilePath(inDirectory));
    ++(*this);
} 


nsDirectoryIterator::~nsDirectoryIterator()

{
    if (mDir)
        closedir(mDir);
} 


nsDirectoryIterator& nsDirectoryIterator::operator ++ ()

{
    mExists = PR_FALSE;
    if (!mDir)
        return *this;
    const char dot[]    = ".";
    const char dotdot[] = "..";
    struct dirent* entry = readdir(mDir);
    if (entry && strcmp(entry->d_name, dot) == 0)
        entry = readdir(mDir);
    if (entry && strcmp(entry->d_name, dotdot) == 0)
        entry = readdir(mDir);
    if (entry)
    {
        mExists = PR_TRUE;
	mCurrent = mStarting;	
        mCurrent.SetLeafName(entry->d_name);
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






#ifdef XP_MACOSX
typedef void (*UnicodeNormalizer) (CFMutableStringRef, CFStringNormalizationForm);
static void CopyUTF8toUTF16NFC(const nsACString& aSrc, nsAString& aResult)
{
    static PRBool sChecked = PR_FALSE;
    static UnicodeNormalizer sUnicodeNormalizer = NULL;

    
    if (!sChecked) {
        CFBundleRef carbonBundle =
            CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Carbon"));
        if (carbonBundle)
            sUnicodeNormalizer = (UnicodeNormalizer)
                ::CFBundleGetFunctionPointerForName(carbonBundle,
                                                    CFSTR("CFStringNormalize"));
        sChecked = PR_TRUE;
    }

    if (!sUnicodeNormalizer) {  
        CopyUTF8toUTF16(aSrc, aResult);
        return;  
    }

    const nsAFlatCString &inFlatSrc = PromiseFlatCString(aSrc);

    
    
    CFMutableStringRef inStr =
        ::CFStringCreateMutable(NULL, inFlatSrc.Length());

    if (!inStr) {
        CopyUTF8toUTF16(aSrc, aResult);
        return;  
    }
     
    ::CFStringAppendCString(inStr, inFlatSrc.get(), kCFStringEncodingUTF8); 

    sUnicodeNormalizer(inStr, kCFStringNormalizationFormC);

    CFIndex length = CFStringGetLength(inStr);
    const UniChar* chars = CFStringGetCharactersPtr(inStr);

    if (chars) 
        aResult.Assign(chars, length);
    else {
        nsAutoBuffer<UniChar, 512> buffer;
        if (buffer.EnsureElemCapacity(length)) {
            CFStringGetCharacters(inStr, CFRangeMake(0, length), buffer.get());
            aResult.Assign(buffer.get(), length);
        }
        else 
            CopyUTF8toUTF16(aSrc, aResult);
    }
    CFRelease(inStr);
}
#endif
