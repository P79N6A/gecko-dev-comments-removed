





































#ifndef NESTEDLOOPTIMER_H
#define NESTEDLOOPTIMER_H

#include <QtCore/QObject>

class QTimer;

namespace mozilla {
namespace plugins {

class PluginModuleChild;

class NestedLoopTimer: public QObject
{
    Q_OBJECT
public:
    NestedLoopTimer(PluginModuleChild *pmc);

    virtual ~NestedLoopTimer();

public Q_SLOTS:
    virtual void timeOut();
    virtual void processSomeEvents();
   
private:
    PluginModuleChild *mModule;
    QTimer *mQTimer;
};

} 
} 

#endif
