









































#include "../TestHarness.h"
#include "nsEmbedString.h"
#include "nsILocalFile.h"
#include <windows.h>
#include <aclapi.h>

#define BUFFSIZE 512



nsresult TestPermissions()
{

    nsresult rv; 

    
    HANDLE tempFileHandle;
    nsCOMPtr<nsILocalFile> tempFile;
    nsCOMPtr<nsILocalFile> tempDirectory1;
    nsCOMPtr<nsILocalFile> tempDirectory2;
    WCHAR filePath[MAX_PATH];
    WCHAR dir1Path[MAX_PATH];
    WCHAR dir2Path[MAX_PATH];

    
    DWORD result;
    PSID everyoneSID = NULL, adminSID = NULL;
    PACL dirACL = NULL, fileACL = NULL;
    PSECURITY_DESCRIPTOR dirSD = NULL, fileSD = NULL;
    EXPLICIT_ACCESS ea[2];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld =
            SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SECURITY_ATTRIBUTES sa;
    TRUSTEE everyoneTrustee;
    ACCESS_MASK everyoneRights;

    
    if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
                     SECURITY_WORLD_RID,
                     0, 0, 0, 0, 0, 0, 0,
                     &everyoneSID))
    {
        fail("NTFS Permissions: AllocateAndInitializeSid Error");
        return NS_ERROR_FAILURE;
    }

    
    if(! AllocateAndInitializeSid(&SIDAuthNT, 2,
                     SECURITY_BUILTIN_DOMAIN_RID,
                     DOMAIN_ALIAS_RID_ADMINS,
                     0, 0, 0, 0, 0, 0,
                     &adminSID)) 
    {
        fail("NTFS Permissions: AllocateAndInitializeSid Error");
        return NS_ERROR_FAILURE; 
    }

    
    
    ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
    ea[0].grfAccessPermissions = GENERIC_READ;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) everyoneSID;

    
    
    ea[1].grfAccessPermissions = GENERIC_ALL | STANDARD_RIGHTS_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) adminSID;

    
    result = SetEntriesInAcl(2, ea, NULL, &dirACL);
    if (ERROR_SUCCESS != result) 
    {
        fail("NTFS Permissions: SetEntriesInAcl Error");
        return NS_ERROR_FAILURE; 
    }

    
    dirSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                             SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (NULL == dirSD) 
    { 
        fail("NTFS Permissions: LocalAlloc Error");
        return NS_ERROR_FAILURE; 
    }

    if (!InitializeSecurityDescriptor(dirSD,
            SECURITY_DESCRIPTOR_REVISION)) 
    {  
        fail("NTFS Permissions: InitializeSecurityDescriptor Error");
        return NS_ERROR_FAILURE; 
    } 

    
    if (!SetSecurityDescriptorDacl(dirSD, true, dirACL, false)) 
    {  
        fail("NTFS Permissions: SetSecurityDescriptorDacl Error");
        return NS_ERROR_FAILURE;  
    } 

    
    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = dirSD;
    sa.bInheritHandle = false;

    
    if(!CreateDirectoryW(L".\\NTFSPERMTEMP1", &sa))
    {
        fail("NTFS Permissions: Creating Temporary Directory");
        return NS_ERROR_FAILURE;
    }

    GetFullPathNameW((LPCWSTR)L".\\NTFSPERMTEMP1", MAX_PATH, dir1Path, NULL);


    rv = NS_NewLocalFile(nsEmbedString(dir1Path), false,
                         getter_AddRefs(tempDirectory1));
    if (NS_FAILED(rv))
    {
        fail("NTFS Permissions: Opening Temporary Directory 1");
        return rv;
    }


    
    tempFileHandle = CreateFileW(L".\\NTFSPERMTEMP1\\NTFSPerm.tmp", 
                            GENERIC_READ | GENERIC_WRITE,
                            0, 
                            NULL, 
                            CREATE_ALWAYS,        
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);  

    if(tempFileHandle == INVALID_HANDLE_VALUE)
    {
        fail("NTFS Permissions: Creating Temporary File");
        return NS_ERROR_FAILURE;
    }

    CloseHandle(tempFileHandle);

    GetFullPathNameW((LPCWSTR)L".\\NTFSPERMTEMP1\\NTFSPerm.tmp", 
                        MAX_PATH, filePath, NULL);

    rv = NS_NewLocalFile(nsEmbedString(filePath), false,
                         getter_AddRefs(tempFile));
    if (NS_FAILED(rv))
    {
        fail("NTFS Permissions: Opening Temporary File");
                return rv;
    }

    
    ea[0].grfAccessPermissions = GENERIC_ALL | STANDARD_RIGHTS_ALL;

    
    result = SetEntriesInAcl(2, ea, NULL, &dirACL);
    if (ERROR_SUCCESS != result) 
    {
        fail("NTFS Permissions: SetEntriesInAcl 2 Error");
        return NS_ERROR_FAILURE; 
    }

    
    if (!SetSecurityDescriptorDacl(dirSD, true, dirACL, false)) 
    {  
        fail("NTFS Permissions: SetSecurityDescriptorDacl 2 Error");
        return NS_ERROR_FAILURE;  
    } 

    
    if(!CreateDirectoryW(L".\\NTFSPERMTEMP2", &sa))
    {
        fail("NTFS Permissions: Creating Temporary Directory 2");
        return NS_ERROR_FAILURE;
    }

    GetFullPathNameW((LPCWSTR)L".\\NTFSPERMTEMP2", MAX_PATH, dir2Path, NULL);


    rv = NS_NewLocalFile(nsEmbedString(dir2Path), false,
                         getter_AddRefs(tempDirectory2));
    if (NS_FAILED(rv))
    {
        fail("NTFS Permissions: Opening Temporary Directory 2");
        return rv;
    }

    
    rv = tempFile->MoveTo(tempDirectory2, EmptyString());

    if (NS_FAILED(rv))
    {
        fail("NTFS Permissions: Moving");
        return rv;
    }

    
    result = GetNamedSecurityInfoW(L".\\NTFSPERMTEMP2\\NTFSPerm.tmp", 
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION | 
                                        UNPROTECTED_DACL_SECURITY_INFORMATION,
                                        NULL, NULL, &fileACL, NULL, &fileSD);
    if (ERROR_SUCCESS != result) 
    {
        fail("NTFS Permissions: GetNamedSecurityDescriptor Error");
        return NS_ERROR_FAILURE; 
    }

    
    BuildTrusteeWithSid(&everyoneTrustee, everyoneSID);

    
    result = GetEffectiveRightsFromAcl(fileACL, &everyoneTrustee, 
                                        &everyoneRights);
    if (ERROR_SUCCESS != result) 
    {
        fail("NTFS Permissions: GetEffectiveRightsFromAcl Error");
        return NS_ERROR_FAILURE; 
    }

    
    
    if((everyoneRights & DELETE) == (DELETE))
    {
        passed("NTFS Permissions Test");
        rv = NS_OK;
    }
    else
    {
        fail("NTFS Permissions: Access check.");
        rv = NS_ERROR_FAILURE;
    }

    
    if (everyoneSID) 
        FreeSid(everyoneSID);
    if (adminSID) 
        FreeSid(adminSID);
    if (dirACL) 
        LocalFree(dirACL);
    if (dirSD) 
        LocalFree(dirSD);
    if(fileACL)
        LocalFree(fileACL);

    tempDirectory1->Remove(true);
    tempDirectory2->Remove(true);
    
    return rv;
}

int main(int argc, char** argv)
{
    ScopedXPCOM xpcom("NTFSPermissionsTests"); 
    if (xpcom.failed())
        return 1;

    int rv = 0;

    if(NS_FAILED(TestPermissions()))
        rv = 1;

    return rv;

}

