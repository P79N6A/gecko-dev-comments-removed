



#ifndef BASE_EVENT_RECORDER_H_
#define BASE_EVENT_RECORDER_H_

#include <string>
#if defined(OS_WIN)
#include <windows.h>
#endif
#include "base/basictypes.h"

class FilePath;

namespace base {














class EventRecorder {
 public:
  
  
  static EventRecorder* current() {
    if (!current_)
      current_ = new EventRecorder();
    return current_;
  }

  
  
  
  bool StartRecording(const FilePath& filename);

  
  void StopRecording();

  
  bool is_recording() const { return is_recording_; }

  
  
  bool StartPlayback(const FilePath& filename);

  
  void StopPlayback();

  
  bool is_playing() const { return is_playing_; }

#if defined(OS_WIN)
  
  
  LRESULT RecordWndProc(int nCode, WPARAM wParam, LPARAM lParam);
  LRESULT PlaybackWndProc(int nCode, WPARAM wParam, LPARAM lParam);
#endif

 private:
  
  
  
  explicit EventRecorder()
      : is_recording_(false),
        is_playing_(false),
#if defined(OS_WIN)
        journal_hook_(NULL),
        file_(NULL),
#endif
        playback_first_msg_time_(0),
        playback_start_time_(0) {
  }
  ~EventRecorder();

  static EventRecorder* current_;  

  bool is_recording_;
  bool is_playing_;
#if defined(OS_WIN)
  HHOOK journal_hook_;
  FILE* file_;
  EVENTMSG playback_msg_;
#endif
  int playback_first_msg_time_;
  int playback_start_time_;

  DISALLOW_EVIL_CONSTRUCTORS(EventRecorder);
};

}  

#endif 
