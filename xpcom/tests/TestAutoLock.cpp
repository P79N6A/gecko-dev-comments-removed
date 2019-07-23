










































#include "nsAutoLock.h"
#include "prthread.h"

PRLock* gLock;
int gCount;

static void PR_CALLBACK run(void* arg)
{
    for (int i = 0; i < 1000000; ++i) {
        nsAutoLock guard(gLock);
        ++gCount;
        PR_ASSERT(gCount == 1);
        --gCount;
    }
}


int main(int argc, char** argv)
{
    gLock = PR_NewLock();
    gCount = 0;

    
    
    

    
    {
        nsAutoLock l2(gLock);
    }

    
    PRThread* t1 =
        PR_CreateThread(PR_SYSTEM_THREAD,
                        run,
                        nsnull,
                        PR_PRIORITY_NORMAL,
                        PR_GLOBAL_THREAD,
                        PR_JOINABLE_THREAD,
                        0);

    
    run(nsnull);

    
    PR_JoinThread(t1);
    return 0;
}
