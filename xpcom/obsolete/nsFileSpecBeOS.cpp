



































 



#include <sys/stat.h>
#include <sys/param.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include "nsError.h"
#include "prio.h"   


#include <Entry.h>
#include <Path.h>
#include <Volume.h>


void nsFileSpecHelpers::Canonify(nsSimpleCharString& ioPath, PRBool inMakeDirs)


{
    if (ioPath.IsEmpty())
        return;
    if (inMakeDirs)
    {
        const mode_t mode = 0700;
        nsFileSpecHelpers::MakeAllDirectories((const char*)ioPath, mode);
    }
    char buffer[MAXPATHLEN];
    errno = 0;
    *buffer = '\0';
    BEntry e((const char *)ioPath, true);
    BPath p;
    e.GetPath(&p);
    ioPath = p.Path();
} 


void nsFileSpec::SetLeafName(const char* inLeafName)

{
    mPath.LeafReplace('/', inLeafName);
} 


char* nsFileSpec::GetLeafName() const

{
    return mPath.GetLeaf('/');
} 


PRBool nsFileSpec::Exists() const

{
    struct stat st;
    return !mPath.IsEmpty() && 0 == stat(mPath, &st); 
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
    PRBool hidden = PR_TRUE;
    char *leafname = GetLeafName();
    if (nsnull != leafname)
    {
        if ((!strcmp(leafname, ".")) || (!strcmp(leafname, "..")))
        {
            hidden = PR_FALSE;
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
			mPath = (char*)resolvedPath;
		} 

		BEntry e((const char *)mPath, true);	
		BPath p;
		status_t err;
		err = e.GetPath(&p);
		NS_ASSERTION(err == B_OK, "realpath failed");

		const char* canonicalPath = p.Path();
		if(err == B_OK)
			mPath = (char*)canonicalPath;
		else
			return NS_ERROR_FAILURE;
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
    
    if (mPath.IsEmpty() || strchr(inNewName, '/')) 
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


static int CrudeFileCopy(const char* in, const char* out)

{
	struct stat in_stat;
	int stat_result = -1;

	char	buf [1024];
	FILE	*ifp, *ofp;
	int	rbytes, wbytes;

	if (!in || !out)
		return -1;

	stat_result = stat (in, &in_stat);

	ifp = fopen (in, "r");
	if (!ifp) 
	{
		return -1;
	}

	ofp = fopen (out, "w");
	if (!ofp)
	{
		fclose (ifp);
		return -1;
	}

	while ((rbytes = fread (buf, 1, sizeof(buf), ifp)) > 0)
	{
		while (rbytes > 0)
		{
			if ( (wbytes = fwrite (buf, 1, rbytes, ofp)) < 0 )
			{
				fclose (ofp);
				fclose (ifp);
				unlink(out);
				return -1;
			}
			rbytes -= wbytes;
		}
	}
	fclose (ofp);
	fclose (ifp);

	if (stat_result == 0)
		chmod (out, in_stat.st_mode & 0777);

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
    char curdir [MAXPATHLEN];
    if (!mPath || !*mPath)
    {
        (void) getcwd(curdir, MAXPATHLEN);
        if (!curdir)
            return ULONGLONG_MAX;  
    }
    else
        sprintf(curdir, "%.200s", (const char*)mPath);

    BEntry e(curdir);
    if(e.InitCheck() != B_OK)
        return ULONGLONG_MAX; 
    entry_ref ref;
    e.GetRef(&ref);
    BVolume v(ref.device);

#ifdef DEBUG_DISK_SPACE
    printf("DiskSpaceAvailable: %d bytes\n", space);
#endif
    return v.FreeBytes();
} 






nsDirectoryIterator::nsDirectoryIterator(
    const nsFileSpec& inDirectory
,   PRBool resolveSymlinks)

    : mCurrent(inDirectory)
    , mStarting(inDirectory)
    , mExists(PR_FALSE)
    , mDir(nsnull)
    , mResoveSymLinks(resolveSymlinks)
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
    char* dot    = ".";
    char* dotdot = "..";
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
