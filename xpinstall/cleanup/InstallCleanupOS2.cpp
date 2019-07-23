








































#include "InstallCleanup.h"
#include "InstallCleanupDefines.h"
#define INCL_DOSERRORS
#define INCL_DOS
#include <os2.h>
#include <string.h>
#include <sys/stat.h>



int NativeDeleteFile(const char* aFileToDelete)
{
    struct stat st;
    if (stat(aFileToDelete, &st) != 0)
    {
      return DONE;
    }
    else 
    {
        if(DosDelete(aFileToDelete) != NO_ERROR)
          return TRY_LATER;
    }
    return DONE;
}




int NativeReplaceFile(const char* replacementFile, const char* doomedFile )
{
    
    struct stat st;
    if (stat(replacementFile, &st) != 0)
        return DONE;

    
    if (stricmp(replacementFile, doomedFile) == 0)
        return DONE;

    if (DosDelete(doomedFile) != NO_ERROR)
    {
        if (stat(doomedFile, &st) == 0)
            return TRY_LATER;
    }
    
    
    if (DosMove(replacementFile, doomedFile) != NO_ERROR)
        return TRY_LATER; 

    return DONE;
}

int main(int argc, char *argv[], char *envp[])
{
    HREG  reg;
    BOOL foundSpecialFile = FALSE;
    struct stat st;

    int status = DONE;

    if ( REGERR_OK == NR_StartupRegistry())
    {
        char regFilePath[CCHMAXPATH];

        strcpy(regFilePath, argv[0]);
        char* lastSlash = strrchr(regFilePath, '\\');
        if (lastSlash) {
          
          lastSlash++; 
          *lastSlash = 0; 
        } else {
          
          *regFilePath = 0;
        }
        strcat(regFilePath, CLEANUP_REGISTRY); 
    
        if (stat(regFilePath, &st) != 0)
          strcpy(regFilePath, ""); 
        else
          foundSpecialFile = TRUE;

        if ( REGERR_OK == NR_RegOpen(regFilePath, &reg) )
        {
            int numAttempts = 0;
            do 
            {
                status = PerformScheduledTasks(reg);
                if (status != DONE) {
                    DosSleep(1000); 
                    numAttempts++;
                }
            } while ((status == TRY_LATER) && numAttempts <= 5);

            NR_RegClose(&reg);
        }
        NR_ShutdownRegistry();
        if (status == DONE) {
            DosDelete(regFilePath);
        }
    }
    return(0);
}

