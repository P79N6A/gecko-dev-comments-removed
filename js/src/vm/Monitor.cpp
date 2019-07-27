





#include "vm/Monitor.h"

using namespace js;

bool
Monitor::init()
{
    lock_ = PR_NewLock();
    if (!lock_)
        return false;

    condVar_ = PR_NewCondVar(lock_);
    if (!condVar_)
        return false;

    return true;
}
