




































#include "primpl.h"



















static struct {
    PSID owner;
    PSID group;
    PSID everyone;
} _pr_nt_sids;







void _PR_NT_InitSids(void)
{
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    HANDLE hToken = NULL; 

    UCHAR infoBuffer[1024];
    PTOKEN_OWNER pTokenOwner = (PTOKEN_OWNER) infoBuffer;
    PTOKEN_PRIMARY_GROUP pTokenPrimaryGroup
            = (PTOKEN_PRIMARY_GROUP) infoBuffer;
    DWORD dwLength;
    BOOL rv;

    



    rv = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    if (rv == 0) {
        








        PR_LOG(_pr_io_lm, PR_LOG_DEBUG,
                ("_PR_NT_InitSids: OpenProcessToken() failed. Error: %d",
                GetLastError()));
        return;
    }

    rv = GetTokenInformation(hToken, TokenOwner, infoBuffer,
            sizeof(infoBuffer), &dwLength);
    PR_ASSERT(rv != 0);
    dwLength = GetLengthSid(pTokenOwner->Owner);
    _pr_nt_sids.owner = (PSID) PR_Malloc(dwLength);
    PR_ASSERT(_pr_nt_sids.owner != NULL);
    rv = CopySid(dwLength, _pr_nt_sids.owner, pTokenOwner->Owner);
    PR_ASSERT(rv != 0);

    rv = GetTokenInformation(hToken, TokenPrimaryGroup, infoBuffer,
            sizeof(infoBuffer), &dwLength);
    PR_ASSERT(rv != 0);
    dwLength = GetLengthSid(pTokenPrimaryGroup->PrimaryGroup);
    _pr_nt_sids.group = (PSID) PR_Malloc(dwLength);
    PR_ASSERT(_pr_nt_sids.group != NULL);
    rv = CopySid(dwLength, _pr_nt_sids.group,
            pTokenPrimaryGroup->PrimaryGroup);
    PR_ASSERT(rv != 0);

    rv = CloseHandle(hToken);
    PR_ASSERT(rv != 0);

    
    rv = AllocateAndInitializeSid(&SIDAuthWorld, 1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &_pr_nt_sids.everyone);
    PR_ASSERT(rv != 0);
}







void
_PR_NT_FreeSids(void)
{
    if (_pr_nt_sids.owner) {
        PR_Free(_pr_nt_sids.owner);
    }
    if (_pr_nt_sids.group) {
        PR_Free(_pr_nt_sids.group);
    }
    if (_pr_nt_sids.everyone) {
        FreeSid(_pr_nt_sids.everyone);
    }
}












PRStatus
_PR_NT_MakeSecurityDescriptorACL(
    PRIntn mode,
    DWORD accessTable[],
    PSECURITY_DESCRIPTOR *resultSD,
    PACL *resultACL)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    PACL pACL = NULL;
    DWORD cbACL;  
    DWORD accessMask;

    if (_pr_nt_sids.owner == NULL) {
        PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
        return PR_FAILURE;
    }

    pSD = (PSECURITY_DESCRIPTOR) PR_Malloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (pSD == NULL) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!SetSecurityDescriptorOwner(pSD, _pr_nt_sids.owner, FALSE)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!SetSecurityDescriptorGroup(pSD, _pr_nt_sids.group, FALSE)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }

    





    cbACL = sizeof(ACL)
          + 3 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))
          + GetLengthSid(_pr_nt_sids.owner)
          + GetLengthSid(_pr_nt_sids.group)
          + GetLengthSid(_pr_nt_sids.everyone);
    pACL = (PACL) PR_Malloc(cbACL);
    if (pACL == NULL) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    if (!InitializeAcl(pACL, cbACL, ACL_REVISION)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    accessMask = 0;
    if (mode & 00400) accessMask |= accessTable[0];
    if (mode & 00200) accessMask |= accessTable[1];
    if (mode & 00100) accessMask |= accessTable[2];
    if (accessMask && !AddAccessAllowedAce(pACL, ACL_REVISION, accessMask,
            _pr_nt_sids.owner)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    accessMask = 0;
    if (mode & 00040) accessMask |= accessTable[0];
    if (mode & 00020) accessMask |= accessTable[1];
    if (mode & 00010) accessMask |= accessTable[2];
    if (accessMask && !AddAccessAllowedAce(pACL, ACL_REVISION, accessMask,
            _pr_nt_sids.group)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }
    accessMask = 0;
    if (mode & 00004) accessMask |= accessTable[0];
    if (mode & 00002) accessMask |= accessTable[1];
    if (mode & 00001) accessMask |= accessTable[2];
    if (accessMask && !AddAccessAllowedAce(pACL, ACL_REVISION, accessMask,
            _pr_nt_sids.everyone)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }

    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE)) {
        _PR_MD_MAP_DEFAULT_ERROR(GetLastError());
        goto failed;
    }

    *resultSD = pSD;
    *resultACL = pACL;
    return PR_SUCCESS;

failed:
    if (pSD) {
        PR_Free(pSD);
    }
    if (pACL) {
        PR_Free(pACL);
    }
    return PR_FAILURE;
}





void
_PR_NT_FreeSecurityDescriptorACL(PSECURITY_DESCRIPTOR pSD, PACL pACL)
{
    if (pSD) {
        PR_Free(pSD);
    }
    if (pACL) {
        PR_Free(pACL);
    }
}
