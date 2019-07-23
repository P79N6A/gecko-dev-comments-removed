




































#include "InstallCleanup.h"
#include "InstallCleanupDefines.h"
#include <windows.h>
#include <string.h>



int NativeDeleteFile(const char* aFileToDelete)
{
    if (GetFileAttributes(aFileToDelete) == 0xFFFFFFFF)
    {
      return DONE;
    }
    else 
    {
        if(!DeleteFile(aFileToDelete))
          return TRY_LATER;
    }
    return DONE;
}




int NativeReplaceFile(const char* replacementFile, const char* doomedFile )
{
    
    if (GetFileAttributes(replacementFile) == 0xFFFFFFFF)
        return DONE;

    
    if (CompareString(LOCALE_SYSTEM_DEFAULT,
                      NORM_IGNORECASE | SORT_STRINGSORT,
                      replacementFile, -1,
                      doomedFile, -1) == CSTR_EQUAL)
        return DONE;

    if (!DeleteFile(doomedFile))
    {
        if (GetFileAttributes(doomedFile) != 0xFFFFFFFF) 
            return TRY_LATER;
    }
    
    
    if (!MoveFile(replacementFile, doomedFile))
        return TRY_LATER; 

    return DONE;
}


int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int)
{
    HREG  reg;
    HKEY  hkRunOnceHandle;
    DWORD dwDisp;
    bool foundSpecialFile = FALSE;

    int status = DONE;

    if ( REGERR_OK == NR_StartupRegistry())
    {
        DWORD charsWritten;
        char appPath[_MAX_PATH];
        charsWritten = GetModuleFileName(NULL, appPath, _MAX_PATH);
        if (charsWritten > 0)
        {
            char regFilePath[_MAX_PATH];
            
            strcpy(regFilePath, appPath);
            char* lastSlash = strrchr(regFilePath, '\\');
            if (lastSlash) {
              
              lastSlash++; 
              *lastSlash = 0; 
            } else {
              
              *regFilePath = 0;
            }
            strcat(regFilePath, CLEANUP_REGISTRY); 
    
            if ( GetFileAttributes(regFilePath) == 0xFFFFFFFF ) 
              strcpy(regFilePath, ""); 
            else
              foundSpecialFile = TRUE;

            if ( REGERR_OK == NR_RegOpen(regFilePath, &reg) )
            {
                RegCreateKeyEx(HKEY_CURRENT_USER,
                               "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hkRunOnceHandle,
                               &dwDisp);
                
                LPCTSTR cleanupKeyName = "mozilla_cleanup";
            
                RegSetValueEx(hkRunOnceHandle,
                              cleanupKeyName,
                              0,
                              REG_SZ,
                              (const unsigned char*)appPath,
                              strlen(appPath));

                do 
                {
                    status = PerformScheduledTasks(reg);
                    if (status != DONE)
                        Sleep(1000); 
                } while (status == TRY_LATER);

                RegDeleteValue(hkRunOnceHandle, cleanupKeyName);
                NR_RegClose(&reg);
            }
            NR_ShutdownRegistry();
            DeleteFile(regFilePath);
        }
    }
    return(0);
}

