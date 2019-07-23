



#include "base/message_pump_win.h"

#include <math.h>

#include "base/histogram.h"
#include "base/win_util.h"

using base::Time;

namespace base {

static const wchar_t kWndClass[] = L"Chrome_MessagePumpWindow";



static const int kMsgHaveWork = WM_USER + 1;




void MessagePumpWin::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void MessagePumpWin::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void MessagePumpWin::WillProcessMessage(const MSG& msg) {
  FOR_EACH_OBSERVER(Observer, observers_, WillProcessMessage(msg));
}

void MessagePumpWin::DidProcessMessage(const MSG& msg) {
  FOR_EACH_OBSERVER(Observer, observers_, DidProcessMessage(msg));
}

void MessagePumpWin::RunWithDispatcher(
    Delegate* delegate, Dispatcher* dispatcher) {
  RunState s;
  s.delegate = delegate;
  s.dispatcher = dispatcher;
  s.should_quit = false;
  s.run_depth = state_ ? state_->run_depth + 1 : 1;

  RunState* previous_state = state_;
  state_ = &s;

  DoRunLoop();

  state_ = previous_state;
}

void MessagePumpWin::Quit() {
  DCHECK(state_);
  state_->should_quit = true;
}




int MessagePumpWin::GetCurrentDelay() const {
  if (delayed_work_time_.is_null())
    return -1;

  
  
  
  double timeout = ceil((delayed_work_time_ - Time::Now()).InMillisecondsF());

  
  int delay = static_cast<int>(timeout);
  if (delay < 0)
    delay = 0;

  return delay;
}




MessagePumpForUI::MessagePumpForUI() {
  InitMessageWnd();
}

MessagePumpForUI::~MessagePumpForUI() {
  DestroyWindow(message_hwnd_);
  UnregisterClass(kWndClass, GetModuleHandle(NULL));
}

void MessagePumpForUI::ScheduleWork() {
  if (InterlockedExchange(&have_work_, 1))
    return;  

  
  PostMessage(message_hwnd_, kMsgHaveWork, reinterpret_cast<WPARAM>(this), 0);
}

void MessagePumpForUI::ScheduleDelayedWork(const Time& delayed_work_time) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  delayed_work_time_ = delayed_work_time;

  int delay_msec = GetCurrentDelay();
  DCHECK(delay_msec >= 0);
  if (delay_msec < USER_TIMER_MINIMUM)
    delay_msec = USER_TIMER_MINIMUM;

  
  
  SetTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this), delay_msec, NULL);
}

void MessagePumpForUI::PumpOutPendingPaintMessages() {
  
  
  if (!state_)
    return;

  
  
  
  
  const int kMaxPeekCount = 20;
  bool win2k = win_util::GetWinVersion() <= win_util::WINVERSION_2000;
  int peek_count;
  for (peek_count = 0; peek_count < kMaxPeekCount; ++peek_count) {
    MSG msg;
    if (win2k) {
      if (!PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
        break;
    } else {
      if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_QS_PAINT))
        break;
    }
    ProcessMessageHelper(msg);
    if (state_->should_quit)  
      break;
  }
  
  DHISTOGRAM_COUNTS("Loop.PumpOutPendingPaintMessages Peeks", peek_count);
}





LRESULT CALLBACK MessagePumpForUI::WndProcThunk(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  switch (message) {
    case kMsgHaveWork:
      reinterpret_cast<MessagePumpForUI*>(wparam)->HandleWorkMessage();
      break;
    case WM_TIMER:
      reinterpret_cast<MessagePumpForUI*>(wparam)->HandleTimerMessage();
      break;
  }
  return DefWindowProc(hwnd, message, wparam, lparam);
}

void MessagePumpForUI::DoRunLoop() {
  
  
  
  
  
  
  
  
  
  
  

  for (;;) {
    
    
    
    
    
    
    
    

    bool more_work_is_plausible = ProcessNextWindowsMessage();
    if (state_->should_quit)
      break;

    more_work_is_plausible |= state_->delegate->DoWork();
    if (state_->should_quit)
      break;

    more_work_is_plausible |=
        state_->delegate->DoDelayedWork(&delayed_work_time_);
    
    
    
    
    if (more_work_is_plausible && delayed_work_time_.is_null())
      KillTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this));
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = state_->delegate->DoIdleWork();
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    WaitForWork();  
  }
}

void MessagePumpForUI::InitMessageWnd() {
  HINSTANCE hinst = GetModuleHandle(NULL);

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WndProcThunk;
  wc.hInstance = hinst;
  wc.lpszClassName = kWndClass;
  RegisterClassEx(&wc);

  message_hwnd_ =
      CreateWindow(kWndClass, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hinst, 0);
  DCHECK(message_hwnd_);
}

void MessagePumpForUI::WaitForWork() {
  
  
  int delay = GetCurrentDelay();
  if (delay < 0)  
    delay = INFINITE;

  DWORD result;
  result = MsgWaitForMultipleObjectsEx(0, NULL, delay, QS_ALLINPUT,
                                       MWMO_INPUTAVAILABLE);

  if (WAIT_OBJECT_0 == result) {
    
    
    
    
    
    
    
    
    
    
    MSG msg = {0};
    DWORD queue_status = GetQueueStatus(QS_MOUSE);
    if (HIWORD(queue_status) & QS_MOUSE &&
       !PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE)) {
      WaitMessage();
    }
    return;
  }

  DCHECK_NE(WAIT_FAILED, result) << GetLastError();
}

void MessagePumpForUI::HandleWorkMessage() {
  
  
  
  if (!state_) {
    
    InterlockedExchange(&have_work_, 0);
    return;
  }

  
  
  
  ProcessPumpReplacementMessage();

  
  
  if (state_->delegate->DoWork())
    ScheduleWork();
}

void MessagePumpForUI::HandleTimerMessage() {
  KillTimer(message_hwnd_, reinterpret_cast<UINT_PTR>(this));

  
  
  
  if (!state_)
    return;

  state_->delegate->DoDelayedWork(&delayed_work_time_);
  if (!delayed_work_time_.is_null()) {
    
    ScheduleDelayedWork(delayed_work_time_);
  }
}

bool MessagePumpForUI::ProcessNextWindowsMessage() {
  
  
  
  
  bool sent_messages_in_queue = false;
  DWORD queue_status = GetQueueStatus(QS_SENDMESSAGE);
  if (HIWORD(queue_status) & QS_SENDMESSAGE)
    sent_messages_in_queue = true;

  MSG msg;
  if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    return ProcessMessageHelper(msg);

  return sent_messages_in_queue;
}

bool MessagePumpForUI::ProcessMessageHelper(const MSG& msg) {
  if (WM_QUIT == msg.message) {
    
    
    state_->should_quit = true;
    PostQuitMessage(static_cast<int>(msg.wParam));
    return false;
  }

  
  if (msg.message == kMsgHaveWork && msg.hwnd == message_hwnd_)
    return ProcessPumpReplacementMessage();

  WillProcessMessage(msg);

  if (state_->dispatcher) {
    if (!state_->dispatcher->Dispatch(msg))
      state_->should_quit = true;
  } else {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DidProcessMessage(msg);
  return true;
}

bool MessagePumpForUI::ProcessPumpReplacementMessage() {
  
  
  
  
  
  
  
  

  MSG msg;
  bool have_message = (0 != PeekMessage(&msg, NULL, 0, 0, PM_REMOVE));
  DCHECK(!have_message || kMsgHaveWork != msg.message ||
         msg.hwnd != message_hwnd_);

  
  int old_have_work = InterlockedExchange(&have_work_, 0);
  DCHECK(old_have_work);

  
  if (!have_message)
    return false;

  
  
  
  
  ScheduleWork();
  return ProcessMessageHelper(msg);
}




MessagePumpForIO::MessagePumpForIO() {
  port_.Set(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1));
  DCHECK(port_.IsValid());
}

void MessagePumpForIO::ScheduleWork() {
  if (InterlockedExchange(&have_work_, 1))
    return;  

  
  BOOL ret = PostQueuedCompletionStatus(port_, 0,
                                        reinterpret_cast<ULONG_PTR>(this),
                                        reinterpret_cast<OVERLAPPED*>(this));
  DCHECK(ret);
}

void MessagePumpForIO::ScheduleDelayedWork(const Time& delayed_work_time) {
  
  
  
  delayed_work_time_ = delayed_work_time;
}

void MessagePumpForIO::RegisterIOHandler(HANDLE file_handle,
                                         IOHandler* handler) {
  ULONG_PTR key = reinterpret_cast<ULONG_PTR>(handler);
  HANDLE port = CreateIoCompletionPort(file_handle, port_, key, 1);
  DCHECK(port == port_.Get());
}




void MessagePumpForIO::DoRunLoop() {
  for (;;) {
    
    
    
    
    
    
    
    

    bool more_work_is_plausible = state_->delegate->DoWork();
    if (state_->should_quit)
      break;

    more_work_is_plausible |= WaitForIOCompletion(0, NULL);
    if (state_->should_quit)
      break;

    more_work_is_plausible |=
        state_->delegate->DoDelayedWork(&delayed_work_time_);
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = state_->delegate->DoIdleWork();
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    WaitForWork();  
  }
}



void MessagePumpForIO::WaitForWork() {
  
  
  DCHECK(state_->run_depth == 1) << "Cannot nest an IO message loop!";

  int timeout = GetCurrentDelay();
  if (timeout < 0)  
    timeout = INFINITE;

  WaitForIOCompletion(timeout, NULL);
}

bool MessagePumpForIO::WaitForIOCompletion(DWORD timeout, IOHandler* filter) {
  IOItem item;
  if (completed_io_.empty() || !MatchCompletedIOItem(filter, &item)) {
    
    if (!GetIOItem(timeout, &item))
      return false;

    if (ProcessInternalIOItem(item))
      return true;
  }

  if (item.context->handler) {
    if (filter && item.handler != filter) {
      
      completed_io_.push_back(item);
    } else {
      DCHECK(item.context->handler == item.handler);
      item.handler->OnIOCompleted(item.context, item.bytes_transfered,
                                  item.error);
    }
  } else {
    
    delete item.context;
  }
  return true;
}


bool MessagePumpForIO::GetIOItem(DWORD timeout, IOItem* item) {
  memset(item, 0, sizeof(*item));
  ULONG_PTR key = NULL;
  OVERLAPPED* overlapped = NULL;
  if (!GetQueuedCompletionStatus(port_.Get(), &item->bytes_transfered, &key,
                                 &overlapped, timeout)) {
    if (!overlapped)
      return false;  
    item->error = GetLastError();
    item->bytes_transfered = 0;
  }

  item->handler = reinterpret_cast<IOHandler*>(key);
  item->context = reinterpret_cast<IOContext*>(overlapped);
  return true;
}

bool MessagePumpForIO::ProcessInternalIOItem(const IOItem& item) {
  if (this == reinterpret_cast<MessagePumpForIO*>(item.context) &&
      this == reinterpret_cast<MessagePumpForIO*>(item.handler)) {
    
    DCHECK(!item.bytes_transfered);
    InterlockedExchange(&have_work_, 0);
    return true;
  }
  return false;
}


bool MessagePumpForIO::MatchCompletedIOItem(IOHandler* filter, IOItem* item) {
  DCHECK(!completed_io_.empty());
  for (std::list<IOItem>::iterator it = completed_io_.begin();
       it != completed_io_.end(); ++it) {
    if (!filter || it->handler == filter) {
      *item = *it;
      completed_io_.erase(it);
      return true;
    }
  }
  return false;
}

}  
