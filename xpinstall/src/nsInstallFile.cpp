







































#include "prprf.h"
#include "nsInstallFile.h"
#include "VerReg.h"
#include "ScheduledTasks.h"
#include "nsInstall.h"
#include "nsIDOMInstallVersion.h"
#include "nsInstallResources.h"
#include "nsInstallLogComment.h"
#include "nsInstallBitwise.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"











nsInstallFile::nsInstallFile(nsInstall* inInstall,
                             const nsString& inComponentName,
                             const nsString& inVInfo,
                             const nsString& inJarLocation,
                             nsInstallFolder *folderSpec,
                             const nsString& inPartialPath,
                             PRInt32 mode,
                             PRBool  aRegister,
                             PRInt32 *error) 
  : nsInstallObject(inInstall),
    mVersionInfo(nsnull),
    mJarLocation(nsnull),
    mExtractedFile(nsnull),
    mFinalFile(nsnull),
    mVersionRegistryName(nsnull),
    mReplaceFile(PR_FALSE),
    mRegister(aRegister),
    mMode(mode)
{
    MOZ_COUNT_CTOR(nsInstallFile);

    PRBool flagExists, flagIsFile;
    mFolderCreateCount = 0;

    if ((folderSpec == nsnull) || (inInstall == NULL))
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    *error = nsInstall::SUCCESS;
    
    nsCOMPtr<nsIFile> tmp = folderSpec->GetFileSpec();
    if (!tmp)
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    tmp->Clone(getter_AddRefs(mFinalFile));
    if (mFinalFile == nsnull)
    {
        *error = nsInstall::OUT_OF_MEMORY;
        return;
    }

    mFinalFile->Exists(&flagExists);
    if (flagExists)
    {
        
        mFinalFile->IsFile(&flagIsFile);
        if ( flagIsFile) 
        {
            *error = nsInstall::ACCESS_DENIED;
            return;
        }
        
    }

    
    PRBool finished = PR_FALSE;
    PRUint32 offset = 0;
    PRInt32 location = 0, nodeLength = 0;
    nsString subString;

    location = inPartialPath.FindChar('/', offset);
    if (location == ((PRInt32)inPartialPath.Length() - 1)) 
    {
        *error = nsInstall::INVALID_ARGUMENTS;
        return;
    }

    while (!finished)
    {
        if (location == kNotFound) 
        {
            nodeLength = inPartialPath.Length() - offset;
            finished = PR_TRUE;
        }
        else
        {
            nodeLength = location - offset;
        }
        
        if (nodeLength > MAX_FILENAME) 
        {
            *error = nsInstall::FILENAME_TOO_LONG;
            return;
        }
        else
        {
            inPartialPath.Mid(subString, offset, nodeLength);
            mFinalFile->Append(subString);
            offset += nodeLength + 1;
            if (!finished)
                location = inPartialPath.FindChar('/', offset);
        }
    }

    mFinalFile->Exists(&mReplaceFile);
    mVersionRegistryName  = new nsString(inComponentName);
    mJarLocation          = new nsString(inJarLocation);
    mVersionInfo	        = new nsString(inVInfo);
     
    if (mVersionRegistryName == nsnull ||
        mJarLocation         == nsnull ||
        mVersionInfo         == nsnull )
    {
        *error = nsInstall::OUT_OF_MEMORY;
        return;
    }
}


nsInstallFile::~nsInstallFile()
{
    if (mVersionRegistryName)
        delete mVersionRegistryName;
  
    if (mJarLocation)
        delete mJarLocation;
  
    if (mVersionInfo)
        delete mVersionInfo;
    
    
    

    
    

    MOZ_COUNT_DTOR(nsInstallFile);
}




void nsInstallFile::CreateAllFolders(nsInstall *aInstall, nsIFile *aFolder, PRInt32 *aError)
{
    PRBool              flagExists;
    nsInstallLogComment *ilc   = nsnull;

    nsresult rv = aFolder->Exists(&flagExists);
    if (NS_FAILED(rv))
        *aError = nsInstall::UNEXPECTED_ERROR;
    else if (flagExists)
        *aError = nsInstall::SUCCESS;
    else
    {
        
        nsCOMPtr<nsIFile> parent;
        rv = aFolder->GetParent(getter_AddRefs(parent));
        if (NS_FAILED(rv))
        {
            
            *aError = nsInstall::ACCESS_DENIED;
            return;
        }

        CreateAllFolders(aInstall, parent, aError);
        if (*aError != nsInstall::SUCCESS)
            return;

        aFolder->Create(nsIFile::DIRECTORY_TYPE, 0755); 
        ++mFolderCreateCount;

        nsAutoString folderPath;
        aFolder->GetPath(folderPath);
        ilc = new nsInstallLogComment(aInstall,
                                      NS_LITERAL_STRING("CreateFolder"),
                                      folderPath,
                                      aError);
        if(ilc == nsnull)
            *aError = nsInstall::OUT_OF_MEMORY;

        if(*aError == nsInstall::SUCCESS)
            *aError = mInstall->ScheduleForInstall(ilc);
    }
}

#ifdef XXX_SSU
void nsInstallFile::RemoveAllFolders()
{
    



    PRUint32   i;
    nsFileSpec nsfsFolder;
    nsFileSpec nsfsParentFolder;
    nsString   nsStrFolder;

    if(mFinalFile != nsnull)
    {
      mFinalFile->GetParent(nsfsFolder);
      for(i = 0; i < mFolderCreateCount; i++)
      {
          nsfsFolder.Remove(PR_FALSE);
          nsfsFolder.GetParent(nsfsParentFolder);
          nsfsFolder = nsfsParentFolder;
      }
    }
}
#endif









PRInt32 nsInstallFile::Prepare()
{
    PRInt32 error = nsInstall::SUCCESS;

    if (mInstall == nsnull || mFinalFile == nsnull || mJarLocation == nsnull )
        return nsInstall::INVALID_ARGUMENTS;

    if (mReplaceFile == PR_FALSE)
    {
       


        nsCOMPtr<nsIFile> parent;
        mFinalFile->GetParent(getter_AddRefs(parent));
        CreateAllFolders(mInstall, parent, &error);
        if(nsInstall::SUCCESS != error)
            return error;
    }

    return mInstall->ExtractFileFromJar(*mJarLocation, mFinalFile, getter_AddRefs(mExtractedFile)); 
}






PRInt32 nsInstallFile::Complete()
{
    PRInt32 err;

    if (mInstall == nsnull || mVersionRegistryName == nsnull || mFinalFile == nsnull ) 
    {
       return nsInstall::INVALID_ARGUMENTS;
    }
   
    err = CompleteFileMove();
    
    if ( mRegister && (0 == err || nsInstall::REBOOT_NEEDED == err) ) 
    {
        nsCAutoString path;
        mFinalFile->GetNativePath(path);
        VR_Install( NS_CONST_CAST(char*, NS_ConvertUTF16toUTF8(*mVersionRegistryName).get()),
                    NS_CONST_CAST(char*, path.get()),
                    NS_CONST_CAST(char*, NS_ConvertUTF16toUTF8(*mVersionInfo).get()),
                    PR_FALSE );
    }
    
    return err;

}

void nsInstallFile::Abort()
{
    if (mExtractedFile != nsnull)
        mExtractedFile->Remove(PR_FALSE);
}

#define RESBUFSIZE 4096
char* nsInstallFile::toString()
{
    char* buffer  = new char[RESBUFSIZE];
    char* rsrcVal = nsnull;

    if (buffer == nsnull || !mInstall)
        return nsnull;
    else
        buffer[0] = '\0';
    
    if (mReplaceFile)
    {
        if(mMode & WIN_SHARED_FILE)
        {
            rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("ReplaceSharedFile"));
        }
        else
        {
            rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("ReplaceFile"));
        }
    }
    else
    {
        if(mMode & WIN_SHARED_FILE)
        {
            rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("InstallSharedFile"));
        }
        else
        {
            rsrcVal = mInstall->GetResourcedString(NS_LITERAL_STRING("InstallFile"));
        }
    }

    if (rsrcVal)
    {
        char*    interimCStr = nsnull;
        nsString interimStr;

        if(mMode & DO_NOT_UNINSTALL)
          interimStr.Assign(NS_LITERAL_STRING("(*dnu*) "));

        interimStr.AppendWithConversion(rsrcVal);
        interimCStr = ToNewCString(interimStr);

        if(interimCStr)
        {
            nsCAutoString fname;
            if (mFinalFile)
                mFinalFile->GetNativePath(fname);

            PR_snprintf( buffer, RESBUFSIZE, interimCStr, fname.get() );
            NS_Free(interimCStr);
        }
        NS_Free(rsrcVal);
    }

    return buffer;
}


PRInt32 nsInstallFile::CompleteFileMove()
{
    int    result         = 0;
    PRBool bIsEqual = PR_FALSE;
    
    if (mExtractedFile == nsnull) 
    {
        return nsInstall::UNEXPECTED_ERROR;
    }
   	
    
    mExtractedFile->Equals(mFinalFile, &bIsEqual);
    if ( bIsEqual ) 
    {
        
        result = nsInstall::SUCCESS;
    } 
    else 
    {
        result = ReplaceFileNowOrSchedule(mExtractedFile, mFinalFile, mMode );
    }

    if(mMode & WIN_SHARED_FILE)
    {
      nsCAutoString path;
      mFinalFile->GetNativePath(path);
      RegisterSharedFile(path.get(), mReplaceFile);
    }

    return result;  
}





PRBool
nsInstallFile::CanUninstall()
{
    return PR_TRUE;
}





PRBool
nsInstallFile::RegisterPackageNode()
{
    return PR_TRUE;
}

