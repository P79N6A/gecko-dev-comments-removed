






































#include "InstallCleanup.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>




int NativeDeleteFile(const char* aFileToDelete)
{
    struct stat fileStack;
    if (stat(aFileToDelete, &fileStack) != 0)
    {
      return DONE;
    }
    else 
    {
        if(unlink(aFileToDelete) != 0)
          return TRY_LATER;
    }
    return DONE;
}




int NativeReplaceFile(const char* replacementFile, const char* doomedFile )
{
    struct stat fileStack;

    
    if (stat(replacementFile, &fileStack) != 0)
        return DONE;

    
    if (strcmp(replacementFile, doomedFile) == 0)
        return DONE;

    if (unlink(doomedFile) != 0)
    {
        if (stat(doomedFile, &fileStack) == 0)    
            return TRY_LATER;
    }
    else
    {
      
      if (rename(replacementFile, doomedFile) != 0)
          return TRY_LATER; 
    }

    return DONE;
}

int main(int argc,char* argv[])
{
    HREG reg;
    int status = DONE;
    struct stat fileStack;

    if ( REGERR_OK == NR_StartupRegistry())
    {
        char regFilePath[256];
        strcpy(regFilePath, argv[0]);
        strcat(regFilePath, ".dat");
        if ( stat(regFilePath, &fileStack) != 0)
            regFilePath[0] =  '\0';
        if ( REGERR_OK == NR_RegOpen(regFilePath, &reg) )
        {
            do {
                status = PerformScheduledTasks(reg);
                if (status != DONE)
                   sleep(1);
            } while (status == TRY_LATER);
            NR_RegClose(&reg);
        }
        NR_ShutdownRegistry();
        unlink(regFilePath);
    }
    return(0);
}

