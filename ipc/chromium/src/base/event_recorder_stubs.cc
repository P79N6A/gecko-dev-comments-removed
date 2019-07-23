



#include "base/event_recorder.h"




namespace base {

EventRecorder* EventRecorder::current_;  

bool EventRecorder::StartRecording(const FilePath& filename) {
  return true;
}

void EventRecorder::StopRecording() {
}

bool EventRecorder::StartPlayback(const FilePath& filename) {
  return false;
}

void EventRecorder::StopPlayback() {
}

}  
