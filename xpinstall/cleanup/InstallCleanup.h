



































#ifndef INSTALL_CLEANUP_H
#define INSTALL_CLEANUP_H

#include <stdlib.h>
#include <stdio.h>

#include "prtypes.h"
#include "VerReg.h"

#define DONE 0
#define TRY_LATER -1

int PerformScheduledTasks(HREG);
int DeleteScheduledFiles(HREG);
int ReplaceScheduledFiles(HREG);
int NativeReplaceFile(const char* replacementFile, const char* doomedFile );
int NativeDeleteFile(const char* aFileToDelete);

#endif 

