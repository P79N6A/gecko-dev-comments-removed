







#ifndef mozilla_ReorderQueue_h
#define mozilla_ReorderQueue_h

#include <MediaData.h>
#include <nsTPriorityQueue.h>

namespace mozilla {

struct ReorderQueueComparator
{
  bool LessThan(VideoData* const& a, VideoData* const& b) const
  {
    return a->mTime < b->mTime;
  }
};

typedef nsTPriorityQueue<VideoData*, ReorderQueueComparator> ReorderQueue;

} 

#endif 
