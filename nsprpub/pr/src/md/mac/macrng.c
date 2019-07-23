






































#include <Events.h>
#include <OSUtils.h>
#include <QDOffscreen.h>
#include <PPCToolbox.h>
#include <Processes.h>
#include <LowMem.h>
#include "primpl.h"

extern PRSize _PR_MD_GetRandomNoise( buf, size )
{
    uint32 c = TickCount();
    return _pr_CopyLowBits((void *)buf, size,  &c, sizeof(c));
}
