




#if !defined(NesteggPacketHolder_h_)
#define NesteggPacketHolder_h_

#include <stdint.h>
#include "nsAutoRef.h"
#include "nestegg/nestegg.h"

namespace mozilla {





class NesteggPacketHolder {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NesteggPacketHolder)
  NesteggPacketHolder() : mPacket(nullptr), mOffset(-1), mTimestamp(-1), mIsKeyframe(false) {}

  bool Init(nestegg_packet* aPacket, int64_t aOffset, unsigned aTrack, bool aIsKeyframe)
  {
    uint64_t timestamp_ns;
    if (nestegg_packet_tstamp(aPacket, &timestamp_ns) == -1) {
      return false;
    }

    
    
    mTimestamp = timestamp_ns / 1000;
    mPacket = aPacket;
    mOffset = aOffset;
    mTrack = aTrack;
    mIsKeyframe = aIsKeyframe;

    return true;
  }

  nestegg_packet* Packet() { MOZ_ASSERT(IsInitialized()); return mPacket; }
  int64_t Offset() { MOZ_ASSERT(IsInitialized()); return mOffset; }
  int64_t Timestamp() { MOZ_ASSERT(IsInitialized()); return mTimestamp; }
  unsigned Track() { MOZ_ASSERT(IsInitialized()); return mTrack; }
  bool IsKeyframe() { MOZ_ASSERT(IsInitialized()); return mIsKeyframe; }

private:
  ~NesteggPacketHolder()
  {
    nestegg_free_packet(mPacket);
  }

  bool IsInitialized() { return mOffset >= 0; }

  nestegg_packet* mPacket;

  
  
  int64_t mOffset;

  
  int64_t mTimestamp;

  
  unsigned mTrack;

  
  bool mIsKeyframe;

  
  NesteggPacketHolder(const NesteggPacketHolder &aOther);
  NesteggPacketHolder& operator= (NesteggPacketHolder const& aOther);
};


class WebMPacketQueue {
 public:
  int32_t GetSize() {
    return mQueue.size();
  }

  void Push(NesteggPacketHolder* aItem) {
    mQueue.push_back(aItem);
  }

  void PushFront(NesteggPacketHolder* aItem) {
    mQueue.push_front(Move(aItem));
  }

  nsRefPtr<NesteggPacketHolder> PopFront() {
    nsRefPtr<NesteggPacketHolder> result = mQueue.front();
    mQueue.pop_front();
    return result;
  }

  void Reset() {
    while (!mQueue.empty()) {
      mQueue.pop_front();
    }
  }

private:
  std::deque<nsRefPtr<NesteggPacketHolder>> mQueue;
};


} 

#endif

