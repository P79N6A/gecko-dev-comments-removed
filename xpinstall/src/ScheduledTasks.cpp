







































#include "nscore.h"
#include "nsXPIDLString.h"
#include "nsInstall.h" 
#include "prmem.h"
#include "ScheduledTasks.h"
#include "InstallCleanupDefines.h"

#include "nsDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"

static nsresult 
GetPersistentStringFromSpec(nsIFile* inSpec, nsACString &string)
{
    nsresult rv;

    nsCOMPtr<nsILocalFile> LocalFile = do_QueryInterface(inSpec, &rv);

    if (NS_SUCCEEDED(rv)) {
        rv = LocalFile->GetNativePath(string);
    } 
    else {
        string.Truncate();
    }
    return rv;
}





#ifdef _WINDOWS
#include <sys/stat.h>
#include <windows.h>

PRInt32 ReplaceWindowsSystemFile(nsIFile* currentSpec, nsIFile* finalSpec)
{
    PRInt32 err = -1;

    
    DWORD dwVersion = GetVersion();

    nsCAutoString final;
    nsCAutoString current;

    finalSpec->GetNativePath(final);
    currentSpec->GetNativePath(current);
 
    

    if (dwVersion > 0x80000000)
    {
        

        
        

        int     strlen;
        char    Src[_MAX_PATH];   
        char    Dest[_MAX_PATH];  
        
        strlen = GetShortPathName( (LPCTSTR)current.get(), (LPTSTR)Src, (DWORD)sizeof(Src) );
        if ( strlen > 0 ) 
        {
            current = Src;
        }

        strlen = GetShortPathName( (LPCTSTR) final.get(), (LPTSTR) Dest, (DWORD) sizeof(Dest));
        if ( strlen > 0 ) 
        {
            final = Dest;
        }
        
        
        
        
        AnsiToOem( final.BeginWriting(), final.BeginWriting() );
        AnsiToOem( current.BeginWriting(), current.BeginWriting() );

        if ( WritePrivateProfileString( "Rename", final.get(), current.get(), "WININIT.INI" ) )
            err = 0;
    }
    else
    {
       
        if ( MoveFileEx(final.get(), current.get(), MOVEFILE_DELAY_UNTIL_REBOOT) )
          err = 0;
    }
    
    return err;
}
#endif

nsresult GetRegFilePath(nsACString &regFilePath)
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> iFileUtilityPath;
    
    nsCOMPtr<nsIProperties> directoryService = 
             do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return nsnull;

    if (nsSoftwareUpdate::GetProgramDirectory()) 
    {
        nsCOMPtr<nsIFile> tmp;
        rv = nsSoftwareUpdate::GetProgramDirectory()->Clone(getter_AddRefs(tmp));

        if (NS_FAILED(rv) || !tmp) 
            return nsnull;

        iFileUtilityPath = do_QueryInterface(tmp);
    }
    else
    {
        rv = directoryService->Get(NS_APP_INSTALL_CLEANUP_DIR,
                                  NS_GET_IID(nsIFile),
                                  getter_AddRefs(iFileUtilityPath));
    }
    if (NS_FAILED(rv) || !iFileUtilityPath) 
        return nsnull;

    iFileUtilityPath->AppendNative(CLEANUP_REGISTRY);

    
    
    return iFileUtilityPath->GetNativePath(regFilePath);
}


PRInt32 DeleteFileNowOrSchedule(nsIFile* filename)
{
    PRBool flagExists;  
    PRInt32 result = nsInstall::SUCCESS;

    filename->Remove(PR_FALSE);
    filename->Exists(&flagExists);
    if (flagExists)
        result = ScheduleFileForDeletion(filename);
 
    return result;
} 

PRInt32 ScheduleFileForDeletion(nsIFile *filename)
{
    

    RKEY newkey;
    HREG reg;
    REGERR  err;
    PRInt32 result = nsInstall::UNEXPECTED_ERROR;

    nsCAutoString path;
    GetRegFilePath(path);
    err = NR_RegOpen(const_cast<char*>(path.get()), &reg);

    if ( err == REGERR_OK )
    {
        err = NR_RegAddKey(reg,ROOTKEY_PRIVATE,REG_DELETE_LIST_KEY,&newkey);
        if ( err == REGERR_OK )
        {
            char    valname[20];

            err = NR_RegGetUniqueName( reg, valname, sizeof(valname) );
            if ( err == REGERR_OK )
            {
                nsCAutoString nameowner;
                nsresult rv = GetPersistentStringFromSpec(filename, nameowner);
                if ( NS_SUCCEEDED(rv) && !nameowner.IsEmpty() )
                {
                    const char *fnamestr = nameowner.get();
                    err = NR_RegSetEntry( reg, newkey, valname, 
                                          REGTYPE_ENTRY_BYTES, 
                                          (void*)fnamestr, 
                                          strlen(fnamestr)+sizeof('\0'));

                    if ( err == REGERR_OK )
                    {
                         result = nsInstall::REBOOT_NEEDED;
                         nsSoftwareUpdate::NeedCleanup();
                    }
                }
            }
        }

        NR_RegClose(reg);
    }

    return result;
}




PRInt32 ReplaceFileNow(nsIFile* aReplacementFile, nsIFile* aDoomedFile )
{
    PRBool flagExists, flagRenamedDoomedFileExists, flagIsEqual;
    nsCOMPtr<nsIFile> replacementFile;
    nsresult rv;

    
    aReplacementFile->Clone(getter_AddRefs(replacementFile));

    
    replacementFile->Exists(&flagExists);
    if ( !flagExists )
        return nsInstall::DOES_NOT_EXIST;

    
    replacementFile->Equals(aDoomedFile, &flagIsEqual);
    if ( flagIsEqual )
        return nsInstall::SUCCESS;


    PRInt32 result = nsInstall::ACCESS_DENIED;

    
    nsCOMPtr<nsIFile>      renamedDoomedFile;
    nsCOMPtr<nsILocalFile> tmpLocalFile;
    
    aDoomedFile->Clone(getter_AddRefs(renamedDoomedFile));
    renamedDoomedFile->Exists(&flagRenamedDoomedFileExists);
    if ( flagRenamedDoomedFileExists )
    {
#ifdef XP_MACOSX
        
        
        
        
        
        nsCOMPtr<nsILocalFile> doomedFileLocal = do_QueryInterface(aDoomedFile);
        nsCAutoString doomedFilePath;
        rv = doomedFileLocal->GetNativePath(doomedFilePath);
        if (NS_FAILED(rv))
            return nsInstall::UNEXPECTED_ERROR;
#endif

        tmpLocalFile = do_QueryInterface(renamedDoomedFile, &rv); 

        
        nsAutoString doomedLeafname;
        nsCAutoString uniqueLeafName;
        tmpLocalFile->GetLeafName(doomedLeafname);

        
        PRInt32 extpos = doomedLeafname.RFindChar('.');
        if (extpos != -1)
        {
            
            doomedLeafname.Truncate(extpos + 1); 
        }
        doomedLeafname.AppendLiteral("old");
        
        
        tmpLocalFile->SetLeafName(doomedLeafname);
        tmpLocalFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0644);
        tmpLocalFile->GetNativeLeafName(uniqueLeafName);

        rv = aDoomedFile->Clone(getter_AddRefs(renamedDoomedFile));
                                                                   
        if (NS_FAILED(rv))
            result = nsInstall::UNEXPECTED_ERROR;
        else
        {
            rv = renamedDoomedFile->MoveToNative(nsnull, uniqueLeafName);        
            if (NS_FAILED(rv))
            {
                
                
                
                
                
                
                tmpLocalFile->Remove(PR_FALSE);
            }
            else
            {
                
                
                
                
                
                
                
                
                rv = renamedDoomedFile->SetNativeLeafName(uniqueLeafName);        
                if (NS_FAILED(rv))
                    result = nsInstall::UNEXPECTED_ERROR;
            }
        }

#ifdef XP_MACOSX
        rv = doomedFileLocal->InitWithNativePath(doomedFilePath);
        if (NS_FAILED(rv))
            result = nsInstall::UNEXPECTED_ERROR;
#endif

        if (result == nsInstall::UNEXPECTED_ERROR)
            return result;
    }


    
    aDoomedFile->Exists(&flagExists);
    if ( flagExists )
        return result;

    nsCOMPtr<nsIFile> parentofDoomedFile;
    nsCAutoString doomedLeafname;

    rv = aDoomedFile->GetParent(getter_AddRefs(parentofDoomedFile));
    if ( NS_SUCCEEDED(rv) )
        rv = aDoomedFile->GetNativeLeafName(doomedLeafname);
    if ( NS_SUCCEEDED(rv) )
    {
        rv = replacementFile->MoveToNative(parentofDoomedFile, doomedLeafname);
        
        
        
        
        
        
        
        
        
    }

    if (NS_SUCCEEDED(rv))
    {
        if (flagRenamedDoomedFileExists)
        {
            
            
            result = DeleteFileNowOrSchedule( renamedDoomedFile );
        }
    }
    else
    {
        
        renamedDoomedFile->MoveToNative(nsnull, doomedLeafname);
        
        
    }

    return result;
}





PRInt32 ReplaceFileNowOrSchedule(nsIFile* aReplacementFile, nsIFile* aDoomedFile, PRInt32 aMode)
{
    PRInt32 result = ReplaceFileNow( aReplacementFile, aDoomedFile );

    if ( result == nsInstall::ACCESS_DENIED )
    {
        
#ifdef _WINDOWS
        if ( (aMode & WIN_SYSTEM_FILE) && 
             (ReplaceWindowsSystemFile(aReplacementFile, aDoomedFile) == 0) )
                return nsInstall::REBOOT_NEEDED;
#endif

        RKEY    listkey;
        RKEY    filekey;
        HREG    reg;
        REGERR  err;

        nsCAutoString regFilePath;
        GetRegFilePath(regFilePath);
        if ( REGERR_OK == NR_RegOpen(const_cast<char*>(regFilePath.get()), &reg) ) 
        {
            err = NR_RegAddKey( reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY, &listkey );
            if ( err == REGERR_OK ) 
            {
                char     valname[20];
                REGERR   err2;

                err = NR_RegGetUniqueName( reg, valname, sizeof(valname) );
                if ( err == REGERR_OK )
                {
                    err = NR_RegAddKey( reg, listkey, valname, &filekey );
                    if ( REGERR_OK == err )
                    {
                        nsCAutoString srcowner;
                        nsCAutoString destowner;
                        nsresult rv = GetPersistentStringFromSpec(aReplacementFile, srcowner);
                        nsresult rv2 = GetPersistentStringFromSpec(aDoomedFile, destowner);
                        if ( NS_SUCCEEDED(rv) && NS_SUCCEEDED(rv2) )
                        {
                            const char *fsrc  = srcowner.get();
                            const char *fdest = destowner.get();
                            err = NR_RegSetEntry( reg, filekey, 
                                                  REG_REPLACE_SRCFILE,
                                                  REGTYPE_ENTRY_BYTES, 
                                                  (void*)fsrc, 
                                                  strlen(fsrc)+sizeof('\0'));

                            err2 = NR_RegSetEntry(reg, filekey,
                                                  REG_REPLACE_DESTFILE,
                                                  REGTYPE_ENTRY_BYTES,
                                                  (void*)fdest,
                                                  strlen(fdest)+sizeof('\0'));

                            if ( err == REGERR_OK && err2 == REGERR_OK )
                            {
                                result = nsInstall::REBOOT_NEEDED;
                                nsSoftwareUpdate::NeedCleanup();
                            }
                            else
                                NR_RegDeleteKey( reg, listkey, valname );
                        }
                    }
                }
            }
            NR_RegClose(reg);
        }
    }

    return result;
}










void DeleteScheduledFiles(HREG);
void ReplaceScheduledFiles(HREG);

void PerformScheduledTasks(HREG reg)
{
    DeleteScheduledFiles( reg );
    ReplaceScheduledFiles( reg );
}



void DeleteScheduledFiles( HREG reg )
{
    REGERR  err;
    RKEY    key;
    REGENUM state = 0;
    nsresult rv = NS_OK;

    
    if (REGERR_OK == NR_RegGetKey(reg,ROOTKEY_PRIVATE,REG_DELETE_LIST_KEY,&key))
    {
        
        

        char    namebuf[MAXREGNAMELEN];
        char    valbuf[MAXREGPATHLEN];

        nsCOMPtr<nsIFile>        doomedFile;
        nsCOMPtr<nsILocalFile>   spec;

        if (NS_SUCCEEDED(rv))
        {
            while (REGERR_OK == NR_RegEnumEntries( reg, key, &state, namebuf,
                                                   sizeof(namebuf), 0 ) )
            {
                uint32 bufsize = sizeof(valbuf); 
                err = NR_RegGetEntry( reg, key, namebuf, valbuf, &bufsize );
                if ( err == REGERR_OK )
                {
                    
                    
                    
                    
                    NS_NewNativeLocalFile(nsDependentCString(valbuf), PR_TRUE, getter_AddRefs(spec));
                    spec->Clone(getter_AddRefs(doomedFile));
                    if (NS_SUCCEEDED(rv)) 
                    {
                        PRBool flagExists;
                        doomedFile->Remove(PR_FALSE);
                        doomedFile->Exists(&flagExists);
                        if ( !flagExists )
                        {
                            
                            NR_RegDeleteEntry( reg, key, namebuf );
                        }
                    }
                }
            }

            
            state = 0;
            err = NR_RegEnumEntries(reg, key, &state, namebuf, sizeof(namebuf), 0);
            if ( err == REGERR_NOMORE )
            {
                NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_DELETE_LIST_KEY);
            }
        }
    }
}



void ReplaceScheduledFiles( HREG reg )
{
    RKEY    key;

    
    if (REGERR_OK == NR_RegGetKey(reg,ROOTKEY_PRIVATE,REG_REPLACE_LIST_KEY,&key))
    {
        char keyname[MAXREGNAMELEN];
        char doomedFile[MAXREGPATHLEN];
        char srcFile[MAXREGPATHLEN];

        nsCOMPtr<nsIFile>       doomedSpec;
        nsCOMPtr<nsIFile>       srcSpec;
        nsCOMPtr<nsILocalFile>       src;
        nsCOMPtr<nsILocalFile>       dest;
        nsresult                rv1, rv2;

        uint32 bufsize;
        REGENUM state = 0;
        while (REGERR_OK == NR_RegEnumSubkeys( reg, key, &state, 
                               keyname, sizeof(keyname), REGENUM_CHILDREN))
        {
            bufsize = sizeof(srcFile);
            REGERR err1 = NR_RegGetEntry( reg, (RKEY)state,
                               REG_REPLACE_SRCFILE, srcFile, &bufsize);

            bufsize = sizeof(doomedFile);
            REGERR err2 = NR_RegGetEntry( reg, (RKEY)state,
                               REG_REPLACE_DESTFILE, doomedFile, &bufsize);

            if ( err1 == REGERR_OK && err2 == REGERR_OK )
            {
                rv1 = NS_NewNativeLocalFile(nsDependentCString(srcFile), PR_TRUE, getter_AddRefs(src));
                rv1 = src->Clone(getter_AddRefs(srcSpec));

                rv2 = NS_NewNativeLocalFile(nsDependentCString(doomedFile), PR_TRUE, getter_AddRefs(dest));
                rv2 = dest->Clone(getter_AddRefs(doomedSpec));

                if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2))
                {
                    
                    PRInt32 result = ReplaceFileNow( srcSpec, doomedSpec );

                    if ( result == nsInstall::DOES_NOT_EXIST ||
                         result == nsInstall::SUCCESS )
                    {
                        
                        NR_RegDeleteKey( reg, key, keyname );
                    }
                }
            }
        }


        
        state = 0;
        if (REGERR_NOMORE == NR_RegEnumSubkeys( reg, key, &state, keyname,
                                     sizeof(keyname), REGENUM_CHILDREN ))
        {
            NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY);
        }
    }
}


