





#include "PluginHelperQt.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>

static const int kMaxtimeToProcessEvents = 30;

bool
PluginHelperQt::AnswerProcessSomeEvents()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents, kMaxtimeToProcessEvents);
    return true;
}
