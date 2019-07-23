








































#include "xptiprivate.h"
#include "nsManifestLineReader.h"
#include "nsString.h"

static const char g_Disclaimer[] = "# Generated file. ** DO NOT EDIT! **";

static const char g_TOKEN_Files[]          = "Files";
static const char g_TOKEN_ArchiveItems[]   = "ArchiveItems";
static const char g_TOKEN_Interfaces[]     = "Interfaces";
static const char g_TOKEN_Header[]         = "Header";
static const char g_TOKEN_Version[]        = "Version";
static const char g_TOKEN_AppDir[]         = "AppDir";
static const char g_TOKEN_Directories[]    = "Directories";

static const int  g_VERSION_MAJOR          = 2;
static const int  g_VERSION_MINOR          = 0;



static PRBool 
GetCurrentAppDirString(xptiInterfaceInfoManager* aMgr, nsACString &aStr)
{
    nsCOMPtr<nsILocalFile> appDir;
    aMgr->GetApplicationDir(getter_AddRefs(appDir));
    if(appDir)
        return NS_SUCCEEDED(appDir->GetPersistentDescriptor(aStr));
    return PR_FALSE;
}

static PRBool 
CurrentAppDirMatchesPersistentDescriptor(xptiInterfaceInfoManager* aMgr, 
                                         const char *inStr)
{
    nsCOMPtr<nsILocalFile> appDir;
    aMgr->GetApplicationDir(getter_AddRefs(appDir));

    nsCOMPtr<nsILocalFile> descDir;
    nsresult rv = NS_NewNativeLocalFile(EmptyCString(), PR_FALSE, getter_AddRefs(descDir));
    if(NS_FAILED(rv))
        return PR_FALSE;

    rv = descDir->SetPersistentDescriptor(nsDependentCString(inStr));
    if(NS_FAILED(rv))
        return PR_FALSE;
    
    PRBool matches;
    rv = appDir->Equals(descDir, &matches);
    return NS_SUCCEEDED(rv) && matches;
}

PR_STATIC_CALLBACK(PLDHashOperator)
xpti_InterfaceWriter(PLDHashTable *table, PLDHashEntryHdr *hdr,
                     PRUint32 number, void *arg)
{
    xptiInterfaceEntry* entry = ((xptiHashEntry*)hdr)->value;
    PRFileDesc* fd = (PRFileDesc*)  arg;

    char* iidStr = entry->GetTheIID()->ToString();
    if(!iidStr)
        return PL_DHASH_STOP;

    const xptiTypelib& typelib = entry->GetTypelibRecord();

    PRBool success =  PR_fprintf(fd, "%d,%s,%s,%d,%d,%d\n",
                                 (int) number,
                                 entry->GetTheName(),
                                 iidStr,
                                 (int) typelib.GetFileIndex(),
                                 (int) (typelib.IsZip() ? 
                                 typelib.GetZipItemIndex() : -1),
                                 (int) entry->GetScriptableFlag());

    nsCRT::free(iidStr);

    return success ? PL_DHASH_NEXT : PL_DHASH_STOP;
}



PRBool xptiManifest::Write(xptiInterfaceInfoManager* aMgr,
                           xptiWorkingSet*           aWorkingSet)
{

    PRBool succeeded = PR_FALSE;
    PRFileDesc* fd = nsnull;
    PRUint32 i;
    PRUint32 size32;
    PRIntn interfaceCount = 0;
    nsCAutoString appDirString;
    
    nsCOMPtr<nsILocalFile> tempFile;
    if(!aMgr->GetCloneOfManifestLocation(getter_AddRefs(tempFile)) || !tempFile)
        return PR_FALSE;

    nsCAutoString originalLeafName;
    tempFile->GetNativeLeafName(originalLeafName);

    nsCAutoString leafName;
    leafName.Assign(originalLeafName + NS_LITERAL_CSTRING(".tmp"));

    tempFile->SetNativeLeafName(leafName);

    
    if(NS_FAILED(tempFile->
                 OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
                                  0666, &fd)) || !fd)
    {
        goto out;
    }

    

    if(!PR_fprintf(fd, "%s\n", g_Disclaimer))
        goto out;

    

    if(!PR_fprintf(fd, "\n[%s,%d]\n", g_TOKEN_Header, 2))
        goto out;

    if(!PR_fprintf(fd, "%d,%s,%d,%d\n", 
                       0, g_TOKEN_Version, g_VERSION_MAJOR, g_VERSION_MINOR))
        goto out;

    GetCurrentAppDirString(aMgr, appDirString);
    if(appDirString.IsEmpty())
        goto out;

    if(!PR_fprintf(fd, "%d,%s,%s\n", 
                       1, g_TOKEN_AppDir, appDirString.get()))
        goto out;

    

    if(!PR_fprintf(fd, "\n[%s,%d]\n", 
                       g_TOKEN_Directories, 
                       (int) aWorkingSet->GetDirectoryCount()))
        goto out;

    for(i = 0; i < aWorkingSet->GetDirectoryCount(); i++)
    {
        nsCOMPtr<nsILocalFile> dir;        
        nsCAutoString str;

        aWorkingSet->GetDirectoryAt(i, getter_AddRefs(dir));
        if(!dir)
            goto out;

        dir->GetPersistentDescriptor(str);
        if(str.IsEmpty())
            goto out;
        
        if(!PR_fprintf(fd, "%d,%s\n", (int) i, str.get()))
            goto out;
    }

    

    if(!PR_fprintf(fd, "\n[%s,%d]\n", 
                       g_TOKEN_Files, 
                       (int) aWorkingSet->GetFileCount()))
        goto out;

    for(i = 0; i < aWorkingSet->GetFileCount(); i++)
    {
        const xptiFile& file = aWorkingSet->GetFileAt(i);

        LL_L2UI(size32, file.GetSize());
    
        if(!PR_fprintf(fd, "%d,%s,%d,%u,%lld\n",
                           (int) i,
                           file.GetName(),
                           (int) file.GetDirectory(),
                           size32, PRInt64(file.GetDate())))
        goto out;
    }

    

    if(!PR_fprintf(fd, "\n[%s,%d]\n", 
                       g_TOKEN_ArchiveItems, 
                       (int) aWorkingSet->GetZipItemCount()))
        goto out;

    for(i = 0; i < aWorkingSet->GetZipItemCount(); i++)
    {
        if(!PR_fprintf(fd, "%d,%s\n",
                           (int) i,
                           aWorkingSet->GetZipItemAt(i).GetName()))
        goto out;
    }

    

    interfaceCount = aWorkingSet->mNameTable->entryCount;

    if(!PR_fprintf(fd, "\n[%s,%d]\n", 
                       g_TOKEN_Interfaces, 
                       (int) interfaceCount))
        goto out;

    if(interfaceCount != (PRIntn)
        PL_DHashTableEnumerate(aWorkingSet->mNameTable, 
                               xpti_InterfaceWriter, fd))
        goto out;


    if(PR_SUCCESS == PR_Close(fd))
    {
        succeeded = PR_TRUE;
    }
    fd = nsnull;

out:
    if(fd)
        PR_Close(fd);
    
    if(succeeded)
    {
        
        nsCOMPtr<nsILocalFile> mainFile;
        if(!aMgr->GetCloneOfManifestLocation(getter_AddRefs(mainFile)) || !mainFile)
            return PR_FALSE;
    
        PRBool exists;
        if(NS_FAILED(mainFile->Exists(&exists)))
            return PR_FALSE;

        if(exists && NS_FAILED(mainFile->Remove(PR_FALSE)))
            return PR_FALSE;
    
        nsCOMPtr<nsIFile> parent;
        mainFile->GetParent(getter_AddRefs(parent));
            
        
        if(NS_FAILED(tempFile->MoveToNative(parent, originalLeafName)))
            return PR_FALSE;
    }

    return succeeded;
}        




static char* 
ReadManifestIntoMemory(xptiInterfaceInfoManager* aMgr,
                       PRUint32* pLength)
{
    PRFileDesc* fd = nsnull;
    PRInt32 flen;
    PRInt64 fileSize;
    char* whole = nsnull;
    PRBool success = PR_FALSE;

    nsCOMPtr<nsILocalFile> aFile;
    if(!aMgr->GetCloneOfManifestLocation(getter_AddRefs(aFile)) || !aFile)
        return nsnull;

#ifdef DEBUG
    {
        static PRBool shown = PR_FALSE;
        
        nsCAutoString path;
        if(!shown && NS_SUCCEEDED(aFile->GetNativePath(path)) && !path.IsEmpty())
        {
            printf("Type Manifest File: %s\n", path.get());
            shown = PR_TRUE;        
        } 
    }            
#endif

    if(NS_FAILED(aFile->GetFileSize(&fileSize)) || !(flen = nsInt64(fileSize)))
        return nsnull;

    whole = new char[flen];
    if (!whole)
        return nsnull;

    

    if(NS_FAILED(aFile->OpenNSPRFileDesc(PR_RDONLY, 0444, &fd)) || !fd)
        goto out;

    if(flen > PR_Read(fd, whole, flen))
        goto out;

    success = PR_TRUE;

 out:
    if(fd)
        PR_Close(fd);

    if(!success)     
    {
        delete [] whole;
        return nsnull;
    }

    *pLength = flen;
    return whole;    
}

static
PRBool ReadSectionHeader(nsManifestLineReader& reader, 
                         const char *token, int minCount, int* count)
{
    while(1)
    {
        if(!reader.NextLine())
            break;
        if(*reader.LinePtr() == '[')
        {
            char* p = reader.LinePtr() + (reader.LineLength() - 1);
            if(*p != ']')
                break;
            *p = 0;

            char* values[2];
            int lengths[2];
            if(2 != reader.ParseLine(values, lengths, 2))
                break;

            
            if(0 != PL_strcmp(values[0]+1, token))
                break;

            if((*count = atoi(values[1])) < minCount)
                break;
            
            return PR_TRUE;
        }
    }
    return PR_FALSE;
}



PRBool xptiManifest::Read(xptiInterfaceInfoManager* aMgr,
                          xptiWorkingSet*           aWorkingSet)
{
    int i;
    char* whole = nsnull;
    PRBool succeeded = PR_FALSE;
    PRUint32 flen;
    nsManifestLineReader reader;
    xptiHashEntry* hashEntry;
    int headerCount = 0;
    int dirCount = 0;
    int fileCount = 0;
    int zipItemCount = -1;
    int interfaceCount = 0;
    int dir;
    int flags;
    char* values[6];    
    int lengths[6];
    PRUint32 size32;
    PRInt64 size;
    PRInt64 date;

    whole = ReadManifestIntoMemory(aMgr, &flen);
    if(!whole)
        return PR_FALSE;

    reader.Init(whole, flen);

    
    
    

    
    
    
    
    

    if(!ReadSectionHeader(reader, g_TOKEN_Header, 2, &headerCount))
        goto out;

    if(headerCount != 2)
        goto out;

    

    if(!reader.NextLine())
        goto out;

    
    if(4 != reader.ParseLine(values, lengths, 4))
        goto out;

    
    if(0 != atoi(values[0]))
        goto out;

    
    if(0 != PL_strcmp(values[1], g_TOKEN_Version))
        goto out;

    
    if(g_VERSION_MAJOR != atoi(values[2]))
        goto out;

    
    if(g_VERSION_MINOR != atoi(values[3]))
        goto out;

    

    if(!reader.NextLine())
        goto out;

    
    if(3 != reader.ParseLine(values, lengths, 3))
        goto out;

    
    if(1 != atoi(values[0]))
        goto out;

    
    if(0 != PL_strcmp(values[1], g_TOKEN_AppDir))
        goto out;

    if(!CurrentAppDirMatchesPersistentDescriptor(aMgr, values[2]))
        goto out;

    

    if(!ReadSectionHeader(reader, g_TOKEN_Directories, 1, &dirCount))
        goto out;
    else
    {
        
        

        nsCOMPtr<nsISupportsArray> searchPath;
        aMgr->GetSearchPath(getter_AddRefs(searchPath));

        PRUint32 searchPathCount;
        searchPath->Count(&searchPathCount);
        
        if(dirCount != (int) searchPathCount)
            goto out;
    }

    

    for(i = 0; i < dirCount; ++i)
    {
        if(!reader.NextLine())
            goto out;
       
        
        if(2 != reader.ParseLine(values, lengths, 2))
            goto out;

        
        if(i != atoi(values[0]))
            goto out;

        
        if(!aWorkingSet->DirectoryAtMatchesPersistentDescriptor(i, values[1]))
            goto out;    
    }

    

    if(!ReadSectionHeader(reader, g_TOKEN_Files, 1, &fileCount))
        goto out;


    

    if(!aWorkingSet->NewFileArray(fileCount))   
        goto out;    

    

    for(i = 0; i < fileCount; ++i)
    {
        if(!reader.NextLine())
            goto out;

        
        if(5 != reader.ParseLine(values, lengths, 5))
            goto out;

        
        if(i != atoi(values[0]))
            goto out;

        
        if(!*values[1])
            goto out;

        
        dir = atoi(values[2]);
        if(dir < 0 || dir > dirCount)
            goto out;

        
        size32 = atoi(values[3]);
        if(size32 <= 0)
            goto out;
        LL_UI2L(size, size32);

        
        date = nsCRT::atoll(values[4]);
        if(LL_IS_ZERO(date))
            goto out;
        
        

        aWorkingSet->AppendFile(
            xptiFile(nsInt64(size), nsInt64(date), dir, values[1], aWorkingSet));
    }

    

    if(!ReadSectionHeader(reader, g_TOKEN_ArchiveItems, 0, &zipItemCount))
        goto out;

    

    if(zipItemCount)
        if(!aWorkingSet->NewZipItemArray(zipItemCount))   
            goto out;    

    

    for(i = 0; i < zipItemCount; ++i)
    {
        if(!reader.NextLine())
            goto out;

        
        if(2 != reader.ParseLine(values, lengths, 2))
            goto out;

        
        if(i != atoi(values[0]))
            goto out;

        
        if(!*values[1])
            goto out;
        
        

        aWorkingSet->AppendZipItem(xptiZipItem(values[1], aWorkingSet));
    }

    

    if(!ReadSectionHeader(reader, g_TOKEN_Interfaces, 1, &interfaceCount))
        goto out;

    

    for(i = 0; i < interfaceCount; ++i)
    {
        int fileIndex;
        int zipItemIndex;
        nsIID iid;
        xptiInterfaceEntry* entry;
        xptiTypelib typelibRecord;

        if(!reader.NextLine())
            goto out;

        
        if(6 != reader.ParseLine(values, lengths, 6))
            goto out;

        
        if(i != atoi(values[0]))
            goto out;

        
        if(!*values[1])
            goto out;

        
        if(!iid.Parse(values[2]))
            goto out;

        
        fileIndex = atoi(values[3]);
        if(fileIndex < 0 || fileIndex >= fileCount)
            goto out;

        
        zipItemIndex = atoi(values[4]);
        if(zipItemIndex < -1 || zipItemIndex >= zipItemCount)
            goto out;

        
        flags = atoi(values[5]);
        if(flags != 0 && flags != 1)
            goto out;
        
        

        if(zipItemIndex == -1)
            typelibRecord.Init(fileIndex);
        else
            typelibRecord.Init(fileIndex, zipItemIndex);
        
        entry = xptiInterfaceEntry::NewEntry(values[1], lengths[1],
                                             iid, typelibRecord, 
                                             aWorkingSet);
        if(!entry)
            goto out;    
        
        entry->SetScriptableFlag(flags==1);

        

        hashEntry = (xptiHashEntry*)
            PL_DHashTableOperate(aWorkingSet->mNameTable, 
                                 entry->GetTheName(), PL_DHASH_ADD);
        if(hashEntry)
            hashEntry->value = entry;
    
        

        hashEntry = (xptiHashEntry*)
            PL_DHashTableOperate(aWorkingSet->mIIDTable, 
                                 entry->GetTheIID(), PL_DHASH_ADD);
        if(hashEntry)
            hashEntry->value = entry;
    }

    

    succeeded = PR_TRUE;

 out:
    if(whole)
        delete [] whole;

    if(!succeeded)
    {
        
        aWorkingSet->InvalidateInterfaceInfos();
        aWorkingSet->ClearHashTables();
        aWorkingSet->ClearFiles();
    }
    return succeeded;
}        


PRBool xptiManifest::Delete(xptiInterfaceInfoManager* aMgr)
{
    nsCOMPtr<nsILocalFile> aFile;
    if(!aMgr->GetCloneOfManifestLocation(getter_AddRefs(aFile)) || !aFile)
        return PR_FALSE;

    PRBool exists;
    if(NS_FAILED(aFile->Exists(&exists)))
        return PR_FALSE;

    if(exists && NS_FAILED(aFile->Remove(PR_FALSE)))
        return PR_FALSE;
    
    return PR_TRUE;
}

