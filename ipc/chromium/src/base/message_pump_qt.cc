



#include "base/message_pump_qt.h"

#include <qabstracteventdispatcher.h>
#include <qevent.h>
#include <qapplication.h>

#include <fcntl.h>
#include <math.h>

#include "base/eintr_wrapper.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/platform_thread.h"

namespace {

static int sPokeEvent;
}  

namespace base {

MessagePumpForUI::MessagePumpForUI()
  : qt_pump(*this)
{
}

MessagePumpForUI::~MessagePumpForUI() {
}

MessagePumpQt::MessagePumpQt(MessagePumpForUI &aPump)
  : pump(aPump)
{
  
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
  sPokeEvent = QEvent::registerEventType();
#else
  sPokeEvent = QEvent::User+5000;
#endif
}

MessagePumpQt::~MessagePumpQt()
{
}

bool
MessagePumpQt::event(QEvent *e)
{
  if (e->type() == sPokeEvent) {
    pump.HandleDispatch();
    return true;
  }
  return false;
}

void MessagePumpForUI::Run(Delegate* delegate) {
  RunState state;
  state.delegate = delegate;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;
  
  
  
  
  
  state.more_work_is_plausible = true;

  RunState* previous_state = state_;
  state_ = &state;

  
  
  

  while (!state_->should_quit) {
    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance(qApp->thread());
    if (!dispatcher)
      return;
    dispatcher->processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
  }

  state_ = previous_state;
}

void MessagePumpForUI::HandleDispatch() {
  
  
  
  if (state_->should_quit)
    return;

  state_->more_work_is_plausible = false;

  if (state_->delegate->DoWork())
    state_->more_work_is_plausible = true;

  if (state_->should_quit)
    return;

  if (state_->delegate->DoDelayedWork(&delayed_work_time_))
    state_->more_work_is_plausible = true;
  if (state_->should_quit)
    return;

  
  
  if (state_->more_work_is_plausible)
    return;

  if (state_->delegate->DoIdleWork())
    state_->more_work_is_plausible = true;
  if (state_->should_quit)
    return;
}

void MessagePumpForUI::Quit() {
  if (state_) {
    state_->should_quit = true;
  } else {
    NOTREACHED() << "Quit called outside Run!";
  }
}

void MessagePumpForUI::ScheduleWork() {
  
  
  
  QCoreApplication::postEvent(&qt_pump,
                              new QEvent((QEvent::Type) sPokeEvent));
}

void MessagePumpForUI::ScheduleDelayedWork(const Time& delayed_work_time) {
  
  
  delayed_work_time_ = delayed_work_time;
  ScheduleWork();
}

}  
