



#include "build/build_config.h"

#include <windows.h>
#include <mmsystem.h>

#include "base/event_recorder.h"
#include "base/file_util.h"
#include "base/logging.h"









namespace base {

EventRecorder* EventRecorder::current_ = NULL;

LRESULT CALLBACK StaticRecordWndProc(int nCode, WPARAM wParam,
                                     LPARAM lParam) {
  CHECK(EventRecorder::current());
  return EventRecorder::current()->RecordWndProc(nCode, wParam, lParam);
}

LRESULT CALLBACK StaticPlaybackWndProc(int nCode, WPARAM wParam,
                                       LPARAM lParam) {
  CHECK(EventRecorder::current());
  return EventRecorder::current()->PlaybackWndProc(nCode, wParam, lParam);
}

EventRecorder::~EventRecorder() {
  
  
  DCHECK(!journal_hook_);
  DCHECK(!is_recording_ && !is_playing_);
}

bool EventRecorder::StartRecording(const FilePath& filename) {
  if (journal_hook_ != NULL)
    return false;
  if (is_recording_ || is_playing_)
    return false;

  
  DCHECK(file_ == NULL);
  file_ = file_util::OpenFile(filename, "wb+");
  if (!file_) {
    DLOG(ERROR) << "EventRecorder could not open log file";
    return false;
  }

  
  ::timeBeginPeriod(1);

  
  journal_hook_ = ::SetWindowsHookEx(WH_JOURNALRECORD, StaticRecordWndProc,
                                     GetModuleHandle(NULL), 0);
  if (!journal_hook_) {
    DLOG(ERROR) << "EventRecorder Record Hook failed";
    file_util::CloseFile(file_);
    return false;
  }

  is_recording_ = true;
  return true;
}

void EventRecorder::StopRecording() {
  if (is_recording_) {
    DCHECK(journal_hook_ != NULL);

    if (!::UnhookWindowsHookEx(journal_hook_)) {
      DLOG(ERROR) << "EventRecorder Unhook failed";
      
      return;
    }

    ::timeEndPeriod(1);

    DCHECK(file_ != NULL);
    file_util::CloseFile(file_);
    file_ = NULL;

    journal_hook_ = NULL;
    is_recording_ = false;
  }
}

bool EventRecorder::StartPlayback(const FilePath& filename) {
  if (journal_hook_ != NULL)
    return false;
  if (is_recording_ || is_playing_)
    return false;

  
  DCHECK(file_ == NULL);
  file_ = file_util::OpenFile(filename, "rb");
  if (!file_) {
    DLOG(ERROR) << "EventRecorder Playback could not open log file";
    return false;
  }
  
  if (fread(&playback_msg_, sizeof(EVENTMSG), 1, file_) != 1) {
    DLOG(ERROR) << "EventRecorder Playback has no records!";
    file_util::CloseFile(file_);
    return false;
  }

  
  ::timeBeginPeriod(1);

  
  
  
  
  
  
  
  
  
  playback_start_time_ = timeGetTime();
  playback_first_msg_time_ = playback_msg_.time;

  
  journal_hook_ = ::SetWindowsHookEx(WH_JOURNALPLAYBACK, StaticPlaybackWndProc,
                                     GetModuleHandle(NULL), 0);
  if (!journal_hook_) {
    DLOG(ERROR) << "EventRecorder Playback Hook failed";
    return false;
  }

  is_playing_ = true;

  return true;
}

void EventRecorder::StopPlayback() {
  if (is_playing_) {
    DCHECK(journal_hook_ != NULL);

    if (!::UnhookWindowsHookEx(journal_hook_)) {
      DLOG(ERROR) << "EventRecorder Unhook failed";
      
    }

    DCHECK(file_ != NULL);
    file_util::CloseFile(file_);
    file_ = NULL;

    ::timeEndPeriod(1);

    journal_hook_ = NULL;
    is_playing_ = false;
  }
}


LRESULT EventRecorder::RecordWndProc(int nCode, WPARAM wParam, LPARAM lParam) {
  static bool recording_enabled = true;
  EVENTMSG* msg_ptr = NULL;

  
  
  if (nCode < 0)
    return ::CallNextHookEx(journal_hook_, nCode, wParam, lParam);

  
  if (::GetKeyState(VK_CANCEL) & 0x8000) {
    StopRecording();
    return ::CallNextHookEx(journal_hook_, nCode, wParam, lParam);
  }

  
  
  switch(nCode) {
    case HC_SYSMODALON:
      recording_enabled = false;
      break;
    case HC_SYSMODALOFF:
      recording_enabled = true;
      break;
  }

  if (nCode == HC_ACTION && recording_enabled) {
    
    msg_ptr = reinterpret_cast<EVENTMSG*>(lParam);
    msg_ptr->time = timeGetTime();
    fwrite(msg_ptr, sizeof(EVENTMSG), 1, file_);
    fflush(file_);
  }

  return CallNextHookEx(journal_hook_, nCode, wParam, lParam);
}


LRESULT EventRecorder::PlaybackWndProc(int nCode, WPARAM wParam,
                                       LPARAM lParam) {
  static bool playback_enabled = true;
  int delay = 0;

  switch(nCode) {
    
    
    case HC_SYSMODALON:
      playback_enabled = false;
      break;

    
    
    case HC_SYSMODALOFF:
      playback_enabled = true;
      break;

    
    case HC_SKIP:
      if (!playback_enabled)
        break;

      
      if (fread(&playback_msg_, sizeof(EVENTMSG), 1, file_) != 1)
        this->StopPlayback();
      break;

    
    case HC_GETNEXT:
      if (!playback_enabled)
        break;

      memcpy(reinterpret_cast<void*>(lParam), &playback_msg_,
             sizeof(playback_msg_));

      
      
      
      
      delay = (playback_msg_.time - playback_first_msg_time_) -
              (timeGetTime() - playback_start_time_);
      if (delay < 0)
        delay = 0;
      return delay;

    
    
    
    case HC_NOREMOVE:
      break;
  }

  return CallNextHookEx(journal_hook_, nCode, wParam, lParam);
}

}  
