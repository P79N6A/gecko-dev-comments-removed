




































#include "nsRegisterGRE.h"

#include "nsXPCOM.h"
#include "nsIFile.h"
#include "nsILocalFile.h"

#include "nsAppRunner.h" 
#include "nsStringAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"

#include "prio.h"

#include <windows.h>

static const char kRegKeyRoot[] = "Software\\mozilla.org\\GRE";
static const char kRegFileGlobal[] = "global.reginfo";
static const char kRegFileUser[] = "user.reginfo";

static nsresult
MakeVersionKey(HKEY root, const char* keyname, const nsCString &grehome,
               const GREProperty *aProperties, PRUint32 aPropertiesLen,
               const char *aGREMilestone)
{
  HKEY  subkey;
  DWORD disp;
  if (::RegCreateKeyEx(root, keyname, NULL, NULL, 0, KEY_WRITE, NULL,
                       &subkey, &disp) != ERROR_SUCCESS)
    return NS_ERROR_FAILURE;

  if (disp != REG_CREATED_NEW_KEY) {
    ::RegCloseKey(subkey);
    return NS_ERROR_FAILURE;
  }

  PRBool failed = PR_FALSE;
  failed |= ::RegSetValueEx(subkey, "Version", NULL, REG_SZ,
                            (BYTE*) aGREMilestone,
                            strlen(aGREMilestone)) != ERROR_SUCCESS;
  failed |= ::RegSetValueEx(subkey, "GreHome", NULL, REG_SZ,
                            (BYTE*) grehome.get(),
                            grehome.Length()) != ERROR_SUCCESS;

  for (PRUint32 i = 0; i < aPropertiesLen; ++i) {
    failed |= ::RegSetValueEx(subkey, aProperties[i].property, NULL, REG_SZ,
                              (BYTE*) aProperties[i].value,
                              strlen(aProperties[i].value)) != ERROR_SUCCESS;
  }

  ::RegCloseKey(subkey);

  if (failed) {
    
    ::RegDeleteKey(root, keyname);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

int
RegisterXULRunner(PRBool aRegisterGlobally, nsIFile* aLocation,
                  const GREProperty *aProperties, PRUint32 aPropertiesLen,
                  const char *aGREMilestone)
{
  
  

  nsresult rv;
  PRBool irv;
  int i;

  nsCString greHome;
  rv = aLocation->GetNativePath(greHome);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIFile> savedInfoFile;
  aLocation->Clone(getter_AddRefs(savedInfoFile));
  nsCOMPtr<nsILocalFile> localSaved(do_QueryInterface(savedInfoFile));
  if (!localSaved)
    return PR_FALSE;

  const char *infoname = aRegisterGlobally ? kRegFileGlobal : kRegFileUser;
  localSaved->AppendNative(nsDependentCString(infoname));

  PRFileDesc* fd = nsnull;
  rv = localSaved->OpenNSPRFileDesc(PR_CREATE_FILE | PR_RDWR, 0664, &fd);
  if (NS_FAILED(rv)) {
    
    return PR_FALSE;
  }

  HKEY rootKey = NULL;
  char keyName[MAXPATHLEN];
  PRInt32 r;

  if (::RegCreateKeyEx(aRegisterGlobally ? HKEY_LOCAL_MACHINE :
                                           HKEY_CURRENT_USER,
                       kRegKeyRoot, NULL, NULL, 0, KEY_WRITE,
                       NULL, &rootKey, NULL) != ERROR_SUCCESS) {
    irv = PR_FALSE;
    goto reg_end;
  }

  r = PR_Read(fd, keyName, MAXPATHLEN);
  if (r < 0) {
    irv = PR_FALSE;
    goto reg_end;
  }

  if (r > 0) {
    keyName[r] = '\0';

    
    
    HKEY existing = NULL;
    if (::RegOpenKeyEx(rootKey, keyName, NULL, KEY_QUERY_VALUE, &existing) ==
        ERROR_SUCCESS) {
      fprintf(stderr, "Warning: Registry key Software\\mozilla.org\\GRE\\%s already exists.\n"
              "No action was performed.\n",
              keyName);
      irv = PR_FALSE;
      goto reg_end;
    }

    PR_Close(fd);
    fd = nsnull;
    
    rv = localSaved->OpenNSPRFileDesc(PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE, 0664, &fd);
    if (NS_FAILED(rv)) {
      
      irv = PR_FALSE;
      goto reg_end;
    }
  }

  strcpy(keyName, aGREMilestone);
  rv = MakeVersionKey(rootKey, keyName, greHome, aProperties, aPropertiesLen,
                      aGREMilestone);
  if (NS_SUCCEEDED(rv)) {
    PR_Write(fd, keyName, strlen(keyName));
    irv = PR_TRUE;
    goto reg_end;
  }
  
  for (i = 0; i < 1000; ++i) {
    sprintf(keyName, "%s_%i", aGREMilestone,  i);
    rv = MakeVersionKey(rootKey, keyName, greHome,
                        aProperties, aPropertiesLen,
                        aGREMilestone);
    if (NS_SUCCEEDED(rv)) {
      PR_Write(fd, keyName, strlen(keyName));
      irv = PR_TRUE;
      goto reg_end;
    }
  }

  irv = PR_FALSE;

reg_end:
  if (fd)
    PR_Close(fd);

  if (rootKey)
    ::RegCloseKey(rootKey);

  return irv;
}

void
UnregisterXULRunner(PRBool aGlobal, nsIFile* aLocation,
                    const char *aGREMilestone)
{
  nsCOMPtr<nsIFile> savedInfoFile;
  aLocation->Clone(getter_AddRefs(savedInfoFile));
  nsCOMPtr<nsILocalFile> localSaved (do_QueryInterface(savedInfoFile));
  if (!localSaved)
    return;

  const char *infoname = aGlobal ? kRegFileGlobal : kRegFileUser;
  localSaved->AppendNative(nsDependentCString(infoname));

  PRFileDesc* fd = nsnull;
  nsresult rv = localSaved->OpenNSPRFileDesc(PR_RDONLY, 0, &fd);
  if (NS_FAILED(rv)) {
    
    return;
  }

  char keyName[MAXPATHLEN];
  PRInt32 r = PR_Read(fd, keyName, MAXPATHLEN);
  PR_Close(fd);

  localSaved->Remove(PR_FALSE);

  if (r <= 0)
    return;

  keyName[r] = '\0';

  HKEY rootKey = NULL;
  if (::RegOpenKeyEx(aGlobal ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                     kRegKeyRoot, 0, KEY_READ, &rootKey) != ERROR_SUCCESS)
    return;

  HKEY subKey = NULL;
  if (::RegOpenKeyEx(rootKey, keyName, 0, KEY_READ, &subKey) == ERROR_SUCCESS) {

    char regpath[MAXPATHLEN];
    DWORD reglen = MAXPATHLEN;

    if (::RegQueryValueEx(subKey, "GreHome", NULL, NULL,
                          (BYTE*) regpath, &reglen) == ERROR_SUCCESS) {

      nsCOMPtr<nsILocalFile> regpathfile;
      rv = NS_NewNativeLocalFile(nsDependentCString(regpath), PR_FALSE,
                                 getter_AddRefs(regpathfile));

      PRBool eq;
      if (NS_SUCCEEDED(rv) && 
          NS_SUCCEEDED(aLocation->Equals(regpathfile, &eq)) && !eq) {
        
        
        fprintf(stderr, "Warning: Registry key Software\\mozilla.org\\GRE\\%s points to\n"
                        "alternate path '%s'; unregistration was not successful.\n",
                keyName, regpath);

        ::RegCloseKey(subKey);
        ::RegCloseKey(rootKey);

        return;
      }
    }

    ::RegCloseKey(subKey);
  }

  ::RegDeleteKey(rootKey, keyName);
  ::RegCloseKey(rootKey);
}
