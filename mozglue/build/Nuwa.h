





#ifndef __NUWA_H_
#define __NUWA_H_

#include <setjmp.h>
#include <unistd.h>

#include "mozilla/Types.h"







#ifndef NUWA_TOPLEVEL_MAX
#define NUWA_TOPLEVEL_MAX 8
#endif

struct NuwaProtoFdInfo {
    int protoId;
    int originFd;
    int newFds[2];
};

#define NUWA_NEWFD_PARENT 0
#define NUWA_NEWFD_CHILD 1

extern "C" {

























MFBT_API void NuwaSpawnPrepare();





MFBT_API void NuwaSpawnWait();







MFBT_API pid_t NuwaSpawn();


















MFBT_API void NuwaMarkCurrentThread(void (*recreate)(void *), void *arg);







MFBT_API void NuwaSkipCurrentThread();




MFBT_API void NuwaFreezeCurrentThread();




MFBT_API jmp_buf* NuwaCheckpointCurrentThread1();

MFBT_API bool NuwaCheckpointCurrentThread2(int setjmpCond);






#define NuwaCheckpointCurrentThread() \
  NuwaCheckpointCurrentThread2(setjmp(*NuwaCheckpointCurrentThread1()))









MFBT_API void PrepareNuwaProcess();




MFBT_API void MakeNuwaProcess();








MFBT_API void NuwaAddConstructor(void (*construct)(void *), void *arg);








MFBT_API void NuwaAddFinalConstructor(void (*construct)(void *), void *arg);








MFBT_API bool IsNuwaProcess();




MFBT_API bool IsNuwaReady();
};

#endif 
