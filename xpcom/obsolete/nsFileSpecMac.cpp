



































 




#include <string.h>

#include "prtypes.h"
#include "nscore.h"

#include "FullPath.h"
#include "FileCopy.h"
#include "MoreFilesExtras.h"

#include <Aliases.h>
#include <Folders.h>
#include <Math64.h>
#include <TextUtils.h>
#include <Processes.h>
#include <limits.h>		

#include "nsFileSpec.h"
#include "nsEscape.h"
#include "nsXPIDLString.h"


const unsigned char* kAliasHavenFolderName = "\pnsAliasHaven";


namespace MacFileHelpers

{
	inline void						PLstrcpy(Str255 dst, ConstStr255Param src)
									{
										memcpy(dst, src, 1 + src[0]);
									}

	void							PLstrcpy(Str255 dst, const char* src, int inMaxLen=255);
	void							PLstrncpy(Str255 dst, const char* src, int inMaxLen);

	void							SwapSlashColon(char * s);
	OSErr							FSSpecFromUnixPath(
										const char * unixPath,
										FSSpec& ioSpec,
										Boolean hexDecode,
										Boolean resolveAlias,
										Boolean allowPartial = false,
										Boolean createDirs = false);
	char*							MacPathFromUnixPath(
										const char* unixPath,
										Boolean hexDecode);
	char*							EncodeMacPath(
										char* inPath, 
										Boolean prependSlash,
										Boolean doEscape );
	OSErr							FSSpecFromPathname(
										const char* inPathNamePtr,
										FSSpec& ioSpec,
										Boolean inCreateDirs);
	char*							PathNameFromFSSpec(
										const FSSpec& inSpec );
	OSErr							CreateFolderInFolder(
										short				refNum,		
										long				dirID,
										ConstStr255Param	folderName,	   
										short&				outRefNum,	  
										long&				outDirID);	  

	
	
	
	void							EnsureAliasHaven();
	void							SetNoResolve(Boolean inResolve);
	PRBool							IsAliasSafe(const FSSpec& inSpec);
	OSErr							MakeAliasSafe(FSSpec& inOutSpec);
	OSErr							ResolveAliasFile(FSSpec& inOutSpec, Boolean& wasAliased);

	Boolean							sNoResolve = false;
	long							sAliasHavenDirID = 0;
	short							sAliasHavenVRefNum = 0;
} 


void MacFileHelpers::PLstrcpy(Str255 dst, const char* src, int inMax)

{
	int srcLength = strlen(src);
	NS_ASSERTION(srcLength <= inMax, "Oops, string is too long!");
	if (srcLength > inMax)
		srcLength = inMax;
	dst[0] = srcLength;
	memcpy(&dst[1], src, srcLength);
}


void MacFileHelpers::PLstrncpy(Str255 dst, const char* src, int inMax)

{
	int srcLength = strlen(src);
	if (srcLength > inMax)
		srcLength = inMax;
	dst[0] = srcLength;
	memcpy(&dst[1], src, srcLength);
}


void MacFileHelpers::SwapSlashColon(char * s)


{
	while (*s)
	{
		if (*s == '/')
			*s++ = ':';
		else if (*s == ':')
			*s++ = '/';
		else
			*s++;
	}
} 


char* MacFileHelpers::EncodeMacPath(
	char* inPath, 
	Boolean prependSlash,
	Boolean doEscape )



{
	if (inPath == nsnull)
		return nsnull;
	int pathSize = strlen(inPath);
	
	
	
	
	char* c = inPath + pathSize - 1;
	if (*c == ':')
	{
		*c = 0;
		pathSize--;
	}

	char * newPath = nsnull;
	char * finalPath = nsnull;
	
	if (prependSlash)
	{
		newPath = new char[pathSize + 2];
		newPath[0] = ':';	 
		memcpy(&newPath[1], inPath, pathSize + 1);
	}
	else
	{
		newPath = new char[pathSize + 1];
		strcpy(newPath, inPath);
	}
	if (newPath)
	{
		SwapSlashColon( newPath );
		if (doEscape)
		{
			finalPath = nsEscape(newPath, url_Path);
			delete [] newPath;
		}
		else
			finalPath = newPath;
	}
	delete [] inPath;
	return finalPath;
} 


inline void MacFileHelpers::SetNoResolve(Boolean inResolve)

{
	sNoResolve = inResolve;
} 


OSErr MacFileHelpers::MakeAliasSafe(FSSpec& inOutSpec)



{
	EnsureAliasHaven();
	nsFileSpec dstDirSpec(sAliasHavenVRefNum, sAliasHavenDirID, "\p");

	
	nsFileSpec havenSpec(sAliasHavenVRefNum, sAliasHavenDirID, "\pG'day");
	if (havenSpec.Valid())
		havenSpec.MakeUnique(inOutSpec.name);
	
	if (havenSpec.Valid())
	{
		OSErr err = ::FSpFileCopy(
						&inOutSpec,
						dstDirSpec,
						havenSpec.GetLeafPName(),
						nil, 0, true);
		
		if (err != noErr)
			return err;
		inOutSpec = havenSpec;
	}
	return noErr;
} 


char* MacFileHelpers::MacPathFromUnixPath(const char* unixPath, Boolean hexDecode)

{
	
	size_t len = strlen(unixPath);
	char* result = new char[len + 2]; 
	
	
	if (result)
	{
		char* dst = result;
		const char* src = unixPath;
		if (*src == '/')			 
			src++;
		else if (strchr(src, '/') && *src != '.')
		{
			
			
			*dst++ = '/';
		}
		
		char c = '/';
		do
		{
			char cprev = c; 
			c = *src++;
			if (c == '.' && cprev == '/')
			{
				char* dstSaved = dst;
				
				*dst++ = '/';  
				c = *src++;
				if (c == '.')
				{
					*dst++ = '/'; 
					c = *src++;
				}
				if (c == '/')
				{
					
					
					src++;
				}
				else if (c)
				{
					
					
					src -= (dst - dstSaved);
					dst = dstSaved;
					
					
				}
				continue;
			}
			else if (c == '/' && cprev == '/')
			{
				
				
				
				
				continue;
			}
			*dst++ = c;
		} while (c);
		if (hexDecode)
			nsUnescape(result);	   
		MacFileHelpers::SwapSlashColon(result);
	}
	return result;
} 


OSErr MacFileHelpers::FSSpecFromPathname(
	const char* inPathNamePtr,
	FSSpec& ioSpec, 
	Boolean inCreateDirs)



{
	OSErr err;
	
	
	
	short inVRefNum = ioSpec.vRefNum;
	long inParID = ioSpec.parID;
	
	size_t inLength = strlen(inPathNamePtr);
	bool isRelative = (strchr(inPathNamePtr, ':') == 0 || *inPathNamePtr == ':');
#ifdef NS_DEBUG
	
	NS_ASSERTION(strchr(inPathNamePtr, '/') == 0,
			"Possible unix path where native path is required");
#endif
	if (inLength < 255)
	{
		Str255 pascalpath;
		MacFileHelpers::PLstrcpy(pascalpath, inPathNamePtr);
		if (isRelative)
			err = ::FSMakeFSSpec(inVRefNum, inParID, pascalpath, &ioSpec);
		else
			err = ::FSMakeFSSpec(0, 0, pascalpath, &ioSpec);
	}
	else if (!isRelative)
		err = FSpLocationFromFullPath(inLength, inPathNamePtr, &ioSpec);
	else
		err = bdNamErr;

	if ((err == dirNFErr || err == bdNamErr) && inCreateDirs)
	{
		const char* path = inPathNamePtr;
		if (isRelative)
		{
			ioSpec.vRefNum = inVRefNum;
			ioSpec.parID = inParID;
		}
		else
		{
			ioSpec.vRefNum = 0;
			ioSpec.parID = 0;
		}
		do {
			
			
			const char* nextColon = strchr(path + (*path == ':'), ':');
			
			if (!nextColon)
				nextColon = path + strlen(path);

			
			
			Str255 ppath;
			MacFileHelpers::PLstrncpy(ppath, path, nextColon - path + 1);
			
			
			
			err = ::FSMakeFSSpec(ioSpec.vRefNum, ioSpec.parID, ppath, &ioSpec);

			
			if (!*nextColon)
				break;

			
			
			if (err == noErr)
			{
				
				long dirID;
				Boolean isDirectory;
				err = ::FSpGetDirectoryID(&ioSpec, &dirID, &isDirectory);
				if (!isDirectory)
					return dupFNErr; 
				if (err)
					return err;
				ioSpec.parID = dirID;
			}
			else if (err == fnfErr)
			{
				
				
				err = ::FSpDirCreate(&ioSpec, smCurrentScript, &ioSpec.parID);
				
				if (err == fnfErr)
					err = noErr;
			}
			if (err != noErr)
				return err;
			path = nextColon; 
		} while (1);
	}
	return err;
} 


OSErr MacFileHelpers::CreateFolderInFolder(
	short				 refNum,	 
	long				dirID,
	ConstStr255Param	folderName,	   
	short&				  outRefNum,	
	long&				 outDirID)	  



{
	HFileParam hpb;
	hpb.ioVRefNum = refNum;
	hpb.ioDirID = dirID;
	hpb.ioNamePtr = (StringPtr)&folderName;

	OSErr err = PBDirCreateSync((HParmBlkPtr)&hpb);
	if (err == noErr)
	{
		outRefNum = hpb.ioVRefNum;
		outDirID = hpb.ioDirID;
	}
	else
	{
		outRefNum = 0;
		outDirID = 0;
	}
	return err;
} 


void MacFileHelpers::EnsureAliasHaven()

{
	
	if (sAliasHavenVRefNum != 0)
		return;

	
	FSSpec temp;
	if (FindFolder(0, kTemporaryFolderType, true, & temp.vRefNum, &temp.parID) == noErr)
	{
		CreateFolderInFolder(
			temp.vRefNum,				  
			temp.parID,
			kAliasHavenFolderName,		  
			sAliasHavenVRefNum,		   
			sAliasHavenDirID);		  
	}
} 


PRBool MacFileHelpers::IsAliasSafe(const FSSpec& inSpec)



{
	return sNoResolve
		|| (inSpec.parID == sAliasHavenDirID && inSpec.vRefNum == sAliasHavenVRefNum);
} 


OSErr MacFileHelpers::ResolveAliasFile(FSSpec& inOutSpec, Boolean& wasAliased)

{
	wasAliased = false;
	if (IsAliasSafe(inOutSpec))
		return noErr;
	Boolean dummy;	  
	return ::ResolveAliasFile(&inOutSpec, TRUE, &dummy, &wasAliased);
} 


OSErr MacFileHelpers::FSSpecFromUnixPath(
	const char * unixPath,
	FSSpec& ioSpec,
	Boolean hexDecode,
	Boolean resolveAlias,
	Boolean allowPartial,
	Boolean createDirs)





{
	if (unixPath == nsnull)
		return badFidErr;
	char* macPath = MacPathFromUnixPath(unixPath, hexDecode);
	if (!macPath)
		return memFullErr;

	OSErr err = noErr;
	if (!allowPartial)
	{
		NS_ASSERTION(*unixPath == '/' , "Not a full Unix path!");
	}
	err = FSSpecFromPathname(macPath, ioSpec, createDirs);
	if (err == fnfErr)
		err = noErr;
	Boolean dummy;	  
	if (err == noErr && resolveAlias)	 
		err = MacFileHelpers::ResolveAliasFile(ioSpec, dummy);
	delete [] macPath;
	NS_ASSERTION(err==noErr||err==fnfErr||err==dirNFErr||err==nsvErr, "Not a path!");
	return err;
} 


char* MacFileHelpers::PathNameFromFSSpec( const FSSpec& inSpec )




{
	char* result = nil;
	nsresult rv;

    FSSpec nonConstSpec = inSpec;
    nsCAutoString path;	
    nsCOMPtr<nsILocalFileMac> macFile;
    
    rv = NS_NewLocalFileWithFSSpec(&nonConstSpec, PR_TRUE, getter_AddRefs(macFile));
    if (NS_FAILED(rv)) return nsnull;
    nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(macFile, &rv));
    if (NS_FAILED(rv)) return nsnull;
    rv = localFile->GetNativePath(path);
    if (NS_FAILED(rv)) return nsnull;
    PRInt32 strLen = path.Length();
    result = new char [strLen + 1];
    if (!result) return nsnull;
	memcpy(result, path.get(), strLen);
	result[ strLen ] = 0;

	return result;
} 


#pragma mark -






nsFileSpec::nsFileSpec()

{

	Clear();
}


nsFileSpec::nsFileSpec(const FSSpec& inSpec, PRBool resolveAlias)

: mSpec(inSpec)
, mError(NS_OK)
{

	if (resolveAlias)
	{
		PRBool dummy;
		ResolveSymlink(dummy);
	}
}


void nsFileSpec::operator = (const FSSpec& inSpec)

{
	mSpec = inSpec;
	mError = NS_OK;
}


nsFileSpec::nsFileSpec(const nsFileSpec& inSpec)

:	 mSpec(inSpec.mSpec)
,	 mError(inSpec.Error())
{

}


nsFileSpec::nsFileSpec(const char* inNativePathString, PRBool inCreateDirs)

{

    Clear();		

	if (inNativePathString)
	{
		mError = NS_FILE_RESULT(MacFileHelpers::FSSpecFromPathname(
				  inNativePathString, mSpec, inCreateDirs));
		if (mError == NS_FILE_RESULT(fnfErr))
			mError = NS_OK;
	}

} 


nsFileSpec::nsFileSpec(const nsString& inNativePathString, PRBool inCreateDirs)

{

	Clear();		

	mError = NS_FILE_RESULT(
		MacFileHelpers::FSSpecFromPathname(
			NS_LossyConvertUTF16toASCII(inNativePathString).get(),
			mSpec, inCreateDirs));
	if (mError == NS_FILE_RESULT(fnfErr))
		mError = NS_OK;

} 


nsFileSpec::nsFileSpec(short vRefNum, long parID, ConstStr255Param fileName,  PRBool resolveAlias)

{

	mError = NS_FILE_RESULT(::FSMakeFSSpec(vRefNum, parID, fileName, &mSpec));
	if (mError == NS_FILE_RESULT(fnfErr))
		mError = NS_OK;
 
	if (resolveAlias)
	{
		PRBool dummy;
		ResolveSymlink(dummy);
	}
}


nsFileSpec::nsFileSpec(const nsFilePath& inPath)

{

	*this = inPath.GetFileSpec();
}


void nsFileSpec::operator = (const char* inString)

{
	Clear();		

	if (inString)
	{
		mError = NS_FILE_RESULT(MacFileHelpers::FSSpecFromPathname(inString, mSpec, true));
		if (mError == NS_FILE_RESULT(fnfErr))
			mError = NS_OK;
	}
	
} 


void nsFileSpec::operator = (const nsFileSpec& inSpec)

{
	mPath.SetToEmpty();
	mSpec.vRefNum = inSpec.mSpec.vRefNum;
	mSpec.parID = inSpec.mSpec.parID;

	PRInt32 copySize = inSpec.mSpec.name[0] + 1;
	if (copySize > sizeof(inSpec.mSpec.name))
		copySize = sizeof(inSpec.mSpec.name);
	memcpy(mSpec.name, inSpec.mSpec.name, copySize);
	mError = inSpec.Error();	
} 


void nsFileSpec::operator = (const nsFilePath& inPath)

{
	*this = inPath.GetFileSpec();
} 


inline void nsFileSpec::Clear()

{
	mPath.SetToEmpty();
	mSpec.vRefNum = 0;
	mSpec.parID = 0;
	mSpec.name[0] = 0;
	mError = NS_ERROR_NOT_INITIALIZED;
}


PRBool nsFileSpec::Exists() const

{
  if (NS_FAILED(mError)) return PR_FALSE;
	FSSpec temp;
	return ::FSMakeFSSpec(mSpec.vRefNum, mSpec.parID, mSpec.name, &temp) == noErr;
} 


void nsFileSpec::GetModDate(TimeStamp& outStamp) const

{
	CInfoPBRec pb;
	if (GetCatInfo(pb) == noErr)
		outStamp = ((DirInfo*)&pb)->ioDrMdDat; 
	else
		outStamp = 0;
} 


PRUint32 nsFileSpec::GetFileSize() const

{
	CInfoPBRec pb;
	if (noErr == GetCatInfo(pb))
		return (PRUint32)((HFileInfo*)&pb)->ioFlLgLen;
	return 0;
} 


void nsFileSpec::SetLeafName(const char* inLeafName)


{
	NS_ASSERTION(inLeafName, "Attempt to set leaf name with a null string");
	
	mPath.SetToEmpty();

	if (inLeafName)
	{
		
		Str255 partialPath;
		MacFileHelpers::PLstrcpy(partialPath, inLeafName);
		mError = NS_FILE_RESULT(
			::FSMakeFSSpec(mSpec.vRefNum, mSpec.parID, partialPath, &mSpec));
		if (mError == NS_FILE_RESULT(fnfErr))
			mError = NS_OK;
	}
	
} 


char* nsFileSpec::GetLeafName() const


{
	char leaf[sizeof(mSpec.name)];
	memcpy(leaf, &mSpec.name[1], mSpec.name[0]);
	leaf[mSpec.name[0]] = '\0';
	return nsCRT::strdup(leaf);
} 


void nsFileSpec::MakeAliasSafe()

{
	mPath.SetToEmpty();
	mError = NS_FILE_RESULT(MacFileHelpers::MakeAliasSafe(mSpec));
} 


void nsFileSpec::MakeUnique(ConstStr255Param inSuggestedLeafName)

{
	mPath.SetToEmpty();
	if (inSuggestedLeafName[0] > 0)
		MacFileHelpers::PLstrcpy(mSpec.name, inSuggestedLeafName);

	MakeUnique();
} 


PRBool nsFileSpec::IsFile() const

{
	long dirID;
	Boolean isDirectory;
	return (noErr == ::FSpGetDirectoryID(&mSpec, &dirID, &isDirectory) && !isDirectory);
} 


PRBool nsFileSpec::IsDirectory() const

{
	long dirID;
	Boolean isDirectory;
	return (noErr == ::FSpGetDirectoryID(&mSpec, &dirID, &isDirectory) && isDirectory);
} 


PRBool nsFileSpec::IsHidden() const

{
	CInfoPBRec		cInfo;
	PRBool			hidden = PR_FALSE;

	if (noErr == GetCatInfo(cInfo))
		if (cInfo.hFileInfo.ioFlFndrInfo.fdFlags & kIsInvisible)
			hidden = PR_TRUE;
	
	return hidden;
} 


PRBool nsFileSpec::IsSymlink() const

{
	CInfoPBRec		cInfo;
	PRBool			hidden = PR_FALSE;

	if (noErr == GetCatInfo(cInfo))
		if (cInfo.hFileInfo.ioFlFndrInfo.fdFlags & kIsAlias)
			hidden = PR_TRUE;
	
	return hidden;
} 


nsresult nsFileSpec::ResolveSymlink(PRBool& wasAliased)

{
	Boolean wasAliased2; 
	OSErr err = MacFileHelpers::ResolveAliasFile(mSpec, wasAliased2); 
	if (wasAliased2) 
	{ 
		mError = NS_FILE_RESULT(err); 
		wasAliased = PR_TRUE; 
	} 
	else 
		wasAliased = PR_FALSE; 

	return mError;
} 


void nsFileSpec::GetParent(nsFileSpec& outSpec) const

{
	if (NS_SUCCEEDED(mError))
		outSpec.mError = NS_FILE_RESULT(::FSMakeFSSpec(mSpec.vRefNum, mSpec.parID, nsnull, outSpec));
} 


void nsFileSpec::operator += (const char* inRelativePath)

{
	NS_ASSERTION(inRelativePath, "Attempt to append relative path with null path");

	
	mPath.SetToEmpty();

	
	if (NS_FAILED(Error()))
	{
	  NS_WARNING("trying to append to a bad nsFileSpec");
	  return;
	}
	
	
	long dirID;
	Boolean isDirectory;
	mError = NS_FILE_RESULT(::FSpGetDirectoryID(&mSpec, &dirID, &isDirectory));
	if (NS_FAILED(mError) || !isDirectory || !inRelativePath)
		return;
   
	mSpec.parID = dirID;

	
	
	
	
	
	
	if (*inRelativePath != ':')
	{
		
		NS_ASSERTION(strchr(inRelativePath, ':') == nsnull, "Can not determine path type");
	   
		mError = NS_FILE_RESULT(
			MacFileHelpers::FSSpecFromUnixPath(inRelativePath, mSpec, false, false, true, true));
	}
	else
	{
		
		mError = NS_FILE_RESULT(MacFileHelpers::FSSpecFromPathname(inRelativePath, mSpec, true));
	}
	if (mError == NS_FILE_RESULT(fnfErr))
		mError = NS_OK;
	
} 


void nsFileSpec::CreateDirectory(int )

{
	long ignoredDirID;
	OSErr	err = ::FSpDirCreate(&mSpec, smCurrentScript, &ignoredDirID);
	
	if (err != noErr && IsDirectory())
		err = noErr;
		
	mError = NS_FILE_RESULT(err);
	
} 


void nsFileSpec::Delete(PRBool inRecursive) const

{
	OSErr anErr;

	nsresult& mutableError = const_cast<nsFileSpec*>(this)->mError;
	if (inRecursive)
	{
		
		anErr = ::DeleteDirectory(
					mSpec.vRefNum,
					mSpec.parID,
					const_cast<unsigned char*>(mSpec.name));
	}
	else
		anErr = ::FSpDelete(&mSpec);
	
	if (anErr == fnfErr) 
		anErr = noErr;
	
	mutableError = NS_FILE_RESULT(anErr);
   
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
	else
	{
		nsFileSpec& filePath = (nsFileSpec&) *this;

		if (!(newDir.Exists()))
		{
			newDir.CreateDirectory();
		}

		filePath.CopyToDir(newDir);
	}
} 


nsresult nsFileSpec::Truncate(PRInt32 aNewLength) const

{
    short   refNum;
    OSErr   err;
        
    
    if (NS_FAILED(mError))
        return mError;
        
    
    if (::FSpOpenDF(&mSpec, fsWrPerm, &refNum) != noErr)
        return NS_FILE_FAILURE;

    err = ::SetEOF(refNum, aNewLength);
        
    
    if (err != fnOpnErr)
        (void)::FSClose(refNum);
        
    if (err != noErr)
        return NS_FILE_FAILURE;
        
    return NS_OK;
} 


nsresult nsFileSpec::Rename(const char* inNewName)

{
	NS_ASSERTION(inNewName, "Attempt to rename with null new name");

	if (strchr(inNewName, '/'))
		return NS_FILE_FAILURE; 
	
	Str255 pName;
	MacFileHelpers::PLstrcpy(pName, inNewName);
	if (::FSpRename(&mSpec, pName) != noErr)
		return NS_FILE_FAILURE;
	SetLeafName(inNewName);
	return NS_OK;
} 


nsresult nsFileSpec::CopyToDir(const nsFileSpec& newParentDir) const

{
	

	if (!newParentDir.IsDirectory() || (IsDirectory() ) )
		return NS_FILE_FAILURE;
	
	nsresult rv = NS_FILE_RESULT(::FSpFileCopy(&mSpec,
							&newParentDir.mSpec,
							const_cast<StringPtr>(GetLeafPName()),
							nsnull,
							0,
							true));

	return rv;

} 


nsresult nsFileSpec::MoveToDir(const nsFileSpec& newParentDir) 

{
	
	
	if (!newParentDir.IsDirectory())
		return NS_FILE_FAILURE;
 
	nsresult result = NS_FILE_RESULT(::FSpMoveRenameCompat(&mSpec,
									&newParentDir.mSpec,
									const_cast<StringPtr>(GetLeafPName())));

	if ( NS_SUCCEEDED(result) )
	{
		char* leafName = GetLeafName();
		*this = newParentDir + leafName;
		nsCRT::free(leafName);
	}
	return result;
} 


nsresult nsFileSpec::Execute(const char*  ) const

{
	if (IsDirectory())
		return NS_FILE_FAILURE;

	LaunchParamBlockRec launchThis;
	launchThis.launchAppSpec = const_cast<FSSpec*>(&mSpec);
	launchThis.launchAppParameters = nsnull; 
	
	launchThis.launchBlockID	= extendedBlock;
	launchThis.launchEPBLength	= extendedBlockLen;
	launchThis.launchFileFlags	= nsnull;
	launchThis.launchControlFlags = launchContinue + launchNoFileFlags + launchUseMinimum;
	launchThis.launchControlFlags += launchDontSwitch;

	nsresult result = NS_FILE_RESULT(::LaunchApplication(&launchThis));
	return result;
  
} 


OSErr nsFileSpec::GetCatInfo(CInfoPBRec& outInfo) const

{
	DirInfo	   *dipb=(DirInfo *)&outInfo;
	dipb->ioCompletion = nsnull;
	dipb->ioFDirIndex = 0; 
	dipb->ioVRefNum = mSpec.vRefNum;
	dipb->ioDrDirID = mSpec.parID;
	dipb->ioNamePtr = const_cast<nsFileSpec*>(this)->mSpec.name;
	return PBGetCatInfoSync(&outInfo);
} 


OSErr nsFileSpec::SetFileTypeAndCreator(OSType type, OSType creator)

{
	FInfo info;
	OSErr err = ::FSpGetFInfo(&mSpec, &info);
	if (err != noErr)
		return err;
	info.fdType = type;
	info.fdCreator = creator;
	err = ::FSpSetFInfo(&mSpec, &info);
	return err;
}


OSErr nsFileSpec::GetFileTypeAndCreator(OSType* type, OSType* creator)

{
	FInfo info;
	OSErr err = ::FSpGetFInfo(&mSpec, &info);
	if (err != noErr)
		return err;
	*type = info.fdType;
	*creator = info.fdCreator;	
	return noErr;
}


PRInt64 nsFileSpec::GetDiskSpaceAvailable() const

{
	PRInt64 space64Bits;

	LL_I2L(space64Bits , LONG_MAX);

	XVolumeParam	pb;
	pb.ioCompletion = nsnull;
	pb.ioVolIndex = 0;
	pb.ioNamePtr = nsnull;
	pb.ioVRefNum = mSpec.vRefNum;
	
	
	OSErr err = ::PBXGetVolInfoSync(&pb);
	
	if (err == noErr)
	{
#ifdef HAVE_LONG_LONG
		space64Bits = pb.ioVFreeBytes;
#else
		const UnsignedWide& freeBytes = UInt64ToUnsignedWide(pb.ioVFreeBytes);
		space64Bits.lo = freeBytes.lo;
		space64Bits.hi = freeBytes.hi;
#endif
	}
		
	return space64Bits;
} 


const char* nsFileSpec::GetCString() const





{
	if (mPath.IsEmpty())
	{
		char* path = MacFileHelpers::PathNameFromFSSpec(mSpec);
		if (path != NULL) {
			const_cast<nsFileSpec*>(this)->mPath = path;	
			delete[] path;
		} else {
			const_cast<nsFileSpec*>(this)->mError = NS_ERROR_OUT_OF_MEMORY;
		}
	}
	return mPath;
}

#pragma mark -






static void AssignFromPath(nsFilePath& ioPath, const char* inString, PRBool inCreateDirs)

{
	NS_ASSERTION(inString, "AssignFromPath called with null inString");
	NS_ASSERTION(strstr(inString, kFileURLPrefix) != inString, "URL passed as path");

	FSSpec spec;
	spec.vRefNum = 0;
	spec.parID = 0;
	spec.name[0] = 0;
	MacFileHelpers::FSSpecFromUnixPath(
		inString,
		spec,
		false,
		true, 
		true,
		inCreateDirs);
	
	
	
	
	ioPath = spec;
}


nsFilePath::nsFilePath(const char* inString, PRBool inCreateDirs)

{
	AssignFromPath(*this, inString, inCreateDirs);
} 


nsFilePath::nsFilePath(const nsString& inString, PRBool inCreateDirs)

{
	AssignFromPath(*this, NS_LossyConvertUTF16toASCII(inString).get(),
	               inCreateDirs);
}


void nsFilePath::operator = (const char* inString)

{
	AssignFromPath(*this, inString, PR_FALSE);
}


nsFilePath::nsFilePath(const nsFileSpec& inSpec)

{
	*this = inSpec;
}


nsFilePath::nsFilePath(const nsFileURL& inOther)

{
	*this = inOther;
}


void nsFilePath::operator = (const nsFileSpec& inSpec)

{
	char * path = MacFileHelpers::PathNameFromFSSpec(inSpec);
	path = MacFileHelpers::EncodeMacPath(path, true, false);
	mPath = path;
	nsCRT::free(path);
	mFileSpec = inSpec;
} 


void nsFilePath::operator = (const nsFileURL& inOther)

{
	char * path = MacFileHelpers::PathNameFromFSSpec(inOther.mFileSpec);
	path = MacFileHelpers::EncodeMacPath(path, true, false);
	mPath = path;
	nsCRT::free(path);
	mFileSpec = inOther.GetFileSpec();
}

#pragma mark -






nsFileURL::nsFileURL(const char* inString, PRBool inCreateDirs)

:	 mURL(inString)
{	 
	NS_ASSERTION(inString, "nsFileURL constructed with null inString");
	NS_ASSERTION(strstr(inString, kFileURLPrefix) == inString, "Not a URL!");
	mFileSpec.mError = NS_FILE_RESULT(MacFileHelpers::FSSpecFromUnixPath(
		inString + kFileURLPrefixLength,
		mFileSpec.mSpec,
		true, 
		false, 
		false, 
		inCreateDirs));
	if (mFileSpec.mError == NS_FILE_RESULT(fnfErr))
		mFileSpec.mError = NS_OK;
} 


nsFileURL::nsFileURL(const nsString& inString, PRBool inCreateDirs)

:	 mURL(nsnull)
{
	NS_LossyConvertUTF16toASCII cstring(inString);
	mURL = cstring.get();
	NS_ASSERTION(strstr(cstring.get(), kFileURLPrefix) == cstring.get(),
	             "Not a URL!");
	mFileSpec.mError = NS_FILE_RESULT(MacFileHelpers::FSSpecFromUnixPath(
		cstring.get() + kFileURLPrefixLength,
		mFileSpec.mSpec,
		true, 
		false, 
		false, 
		inCreateDirs));
	if (mFileSpec.mError == NS_FILE_RESULT(fnfErr))
		mFileSpec.mError = NS_OK;
} 


nsFileURL::nsFileURL(const nsFilePath& inOther)

{
	*this = inOther.GetFileSpec();
} 


nsFileURL::nsFileURL(const nsFileSpec& inOther)

{
	*this = inOther;
} 


void nsFileURL::operator = (const nsFilePath& inOther)

{
	*this = inOther.GetFileSpec();
} 


void nsFileURL::operator = (const nsFileSpec& inOther)

{
	mFileSpec  = inOther;
	char* path = MacFileHelpers::PathNameFromFSSpec( mFileSpec );
	char* encodedPath = MacFileHelpers::EncodeMacPath(path, true, true);
	nsSimpleCharString encodedURL(kFileURLPrefix);
	encodedURL += encodedPath;
	nsCRT::free(encodedPath);
	mURL = encodedURL;
	if (encodedURL[encodedURL.Length() - 1] != '/' && inOther.IsDirectory())
		mURL += "/";
} 


void nsFileURL::operator = (const char* inString)

{
	NS_ASSERTION(inString, "nsFileURL operator= constructed with null inString");

	mURL = inString;
	NS_ASSERTION(strstr(inString, kFileURLPrefix) == inString, "Not a URL!");
	mFileSpec.mError = NS_FILE_RESULT(MacFileHelpers::FSSpecFromUnixPath(
		inString + kFileURLPrefixLength,
		mFileSpec.mSpec,
		true, 
		true, 
		false, 
		false)); 
	if (mFileSpec.mError == NS_FILE_RESULT(fnfErr))
		mFileSpec.mError = NS_OK;
} 

#pragma mark -






nsDirectoryIterator::nsDirectoryIterator(
	const nsFileSpec& inDirectory
,	PRBool resolveSymLinks)

	: mCurrent(inDirectory)
	, mExists(false)
	, mResoveSymLinks(resolveSymLinks)
	, mIndex(-1)
{
	CInfoPBRec pb;
	OSErr err = inDirectory.GetCatInfo(pb);
	
	
	DirInfo* dipb = (DirInfo*)&pb;
	if (err != noErr  || !( dipb->ioFlAttrib & 0x0010))
		return;
	
	FSSpec& currentSpec = mCurrent.nsFileSpec::operator FSSpec&();
	mVRefNum = currentSpec.vRefNum;
	mParID = dipb->ioDrDirID;
	mMaxIndex = pb.dirInfo.ioDrNmFls;
	mIndex = 0; 
	++(*this); 
	
} 


OSErr nsDirectoryIterator::SetToIndex()

{
	CInfoPBRec cipb;
	DirInfo	   *dipb=(DirInfo *)&cipb;
	Str255 objectName;
	dipb->ioCompletion = nsnull;
	dipb->ioFDirIndex = mIndex;
	
	FSSpec& currentSpec = mCurrent.nsFileSpec::operator FSSpec&();
	dipb->ioVRefNum = mVRefNum; 
	dipb->ioDrDirID = mParID;
	dipb->ioNamePtr = objectName;
	OSErr err = PBGetCatInfoSync(&cipb);
	FSSpec temp;
	if (err == noErr)
		err = FSMakeFSSpec(mVRefNum, mParID, objectName, &temp);
	mCurrent = temp; 
	mExists = err == noErr;

	if (mExists && mResoveSymLinks)
	{	
		PRBool ignore;
		mCurrent.ResolveSymlink(ignore);
	}
	return err;
} 


nsDirectoryIterator& nsDirectoryIterator::operator -- ()

{
	mExists = false;
	while (--mIndex > 0)
	{
		OSErr err = SetToIndex();
		if (err == noErr)
			break;
	}
	return *this;
} 


nsDirectoryIterator& nsDirectoryIterator::operator ++ ()

{
	mExists = false;
	if (mIndex >= 0) 
		while (++mIndex <= mMaxIndex)
		{
			OSErr err = SetToIndex();
			if (err == noErr)
				break;
		}
	return *this;
} 

