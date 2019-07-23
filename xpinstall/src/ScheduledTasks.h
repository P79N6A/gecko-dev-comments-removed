








































#ifndef __SCHEDULEDTASKS_H__
#define __SCHEDULEDTASKS_H__


#include "NSReg.h"
#include "nsIFile.h"

PR_BEGIN_EXTERN_C

PRInt32 DeleteFileNowOrSchedule(nsIFile* filename);
PRInt32 ReplaceFileNowOrSchedule(nsIFile* tmpfile, nsIFile* target, PRInt32 aMode);
PRInt32 ScheduleFileForDeletion(nsIFile* filename);
char*   GetRegFilePath();


void PerformScheduledTasks(HREG reg);

PR_END_EXTERN_C

#endif
