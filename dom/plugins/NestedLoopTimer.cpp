





































#include <QtCore/QTimer>

#include "NestedLoopTimer.h"
#include "mozilla/plugins/PluginModuleChild.h"

namespace mozilla {
namespace plugins {

NestedLoopTimer::NestedLoopTimer(PluginModuleChild *pmc):
     QObject(), mModule(pmc), mQTimer(NULL)
{
}

NestedLoopTimer::~NestedLoopTimer()
{
    if (mQTimer) {
        mQTimer->stop();
        delete mQTimer;
        mQTimer = NULL;
    }
}

void NestedLoopTimer::timeOut()
{
    
    
    
    mQTimer = new QTimer(this);
    QObject::connect(mQTimer, SIGNAL(timeout()), this,
                     SLOT(processSomeEvents()));
    mQTimer->setInterval(kNestedLoopDetectorIntervalMs);
    mQTimer->start();
}

void NestedLoopTimer::processSomeEvents()
{
    if (mModule)
        mModule->CallProcessSomeEvents();
}

} 
} 
