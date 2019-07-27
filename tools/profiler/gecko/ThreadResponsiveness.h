




#ifndef ThreadResponsiveness_h
#define ThreadResponsiveness_h

#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "mozilla/TimeStamp.h"

class ThreadProfile;
class CheckResponsivenessTask;

class ThreadResponsiveness {
public:
  explicit ThreadResponsiveness(ThreadProfile *aThreadProfile);

  ~ThreadResponsiveness();

  void Update();

  mozilla::TimeDuration GetUnresponsiveDuration(const mozilla::TimeStamp& now) const {
    return now - mLastTracerTime;
  }

  bool HasData() const {
    return !mLastTracerTime.IsNull();
  }
private:
  ThreadProfile* mThreadProfile;
  nsRefPtr<CheckResponsivenessTask> mActiveTracerEvent;
  mozilla::TimeStamp mLastTracerTime;
};

#endif

