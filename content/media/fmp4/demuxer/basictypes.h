



#ifndef MEDIA_MP4_BASIC_TYPES_H_
#define MEDIA_MP4_BASIC_TYPES_H_

#include <iostream>
#include <limits>
#include <stdint.h>
#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetDemuxerLog();
#endif

namespace mp4_demuxer {




#define kint32max std::numeric_limits<int32_t>::max()
#define kuint64max std::numeric_limits<uint64_t>::max()
#define kint64max std::numeric_limits<int64_t>::max()



#define OVERRIDE MOZ_OVERRIDE
#define WARN_UNUSED_RESULT

#define DCHECK(condition) \
{ \
  if (!(condition)) {\
    DMX_LOG("DCHECK Failed (%s) %s:%d\n", #condition, __FILE__, __LINE__); \
  } \
}

#define CHECK(condition) { \
  if (!(condition)) {\
    DMX_LOG("CHECK Failed %s %s:%d\n", #condition, __FILE__, __LINE__); \
  } \
}

#define DCHECK_LE(variable, value) DCHECK(variable <= value)
#define DCHECK_LT(variable, value) DCHECK(variable < value)
#define DCHECK_EQ(variable, value) DCHECK(variable == value)
#define DCHECK_GT(variable, value) DCHECK(variable > value)
#define DCHECK_GE(variable, value) DCHECK(variable >= value)

#define RCHECK(x) \
    do { \
      if (!(x)) { \
        DMX_LOG("Failure while parsing MP4: %s %s:%d\n", #x, __FILE__, __LINE__); \
        return false; \
      } \
    } while (0)

#define arraysize(f) (sizeof(f) / sizeof(*f))

#ifdef LOG_DEMUXER
#ifdef PR_LOGGING
#define DMX_LOG(...) PR_LOG(GetDemuxerLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define DMX_LOG(...) (void)0
#endif
#else


#define DMX_LOG(...) (void)0
#endif



#define DISALLOW_COPY_AND_ASSIGN(TypeName)  \
  TypeName(const TypeName&);      \
  void operator=(const TypeName&)

typedef int64_t Microseconds;
typedef int64_t Milliseconds;

#define MicrosecondsPerSecond (int64_t(1000000))
#define InfiniteMicroseconds (int64_t(-1))
#define InfiniteMilliseconds (int64_t(-1))

inline Microseconds MicrosecondsFromRational(int64_t numer, int64_t denom) {
  DCHECK_LT((numer > 0 ? numer : -numer),
            kint64max / MicrosecondsPerSecond);
  return MicrosecondsPerSecond * numer / denom;
}

inline Milliseconds ToMilliseconds(Microseconds us) {
  return (us == InfiniteMicroseconds) ? InfiniteMilliseconds : us / 1000;
}

class IntSize {
public:
  IntSize() : w_(0), h_(0) {}
  IntSize(const IntSize& i) : w_(i.w_), h_(i.h_) {}
  IntSize(int32_t w, int32_t h) : w_(w), h_(h) {}
  ~IntSize() {};
  int32_t width() const { return w_; }
  int32_t height() const { return h_; }
  int32_t GetArea() const { return w_ * h_; }
  bool IsEmpty() const { return (w_ == 0) || (h_ == 0); }
private:
  int32_t w_;
  int32_t h_;
};

inline bool operator==(const IntSize& lhs, const IntSize& rhs) {
  return lhs.width() == rhs.width() &&
         lhs.height() == rhs.height();
}

class IntRect {
public:
  IntRect() : x_(0), y_(0), w_(0), h_(0) {}
  IntRect(const IntRect& i) : x_(i.x_), y_(i.y_), w_(i.w_), h_(i.h_) {}
  IntRect(int32_t x, int32_t y, int32_t w, int32_t h) : x_(x), y_(y), w_(w), h_(h) {}
  ~IntRect() {};
  IntSize size() const { return IntSize(w_, h_); }
  int32_t x() const { return x_; }
  int32_t y() const { return y_; }
  int32_t width() const { return w_; }
  int32_t height() const { return h_; }
  int32_t GetArea() const { return w_ * h_; }
  bool IsEmpty() const { return (w_ == 0) || (h_ == 0); }
  int32_t right() const { return x() + width(); }
  int32_t bottom() const { return y() + height(); }
private:
  int32_t x_;
  int32_t y_;
  int32_t w_;
  int32_t h_;
};

inline bool operator==(const IntRect& lhs, const IntRect& rhs) {
  return lhs.x() == rhs.x() &&
         lhs.y() == rhs.y() &&
         lhs.width() == rhs.width() &&
         lhs.height() == rhs.height();
}

enum {
  
  kMaxDimension = (1 << 15) - 1,  

  
  kMaxCanvas = (1 << (14 * 2)),  

  
  kMaxVideoFrames = 4,

  
  
  
  
  
  
  
  
  kMaxSampleRate = 192000,
  kMinSampleRate = 3000,
  kMaxChannels = 32,
  kMaxBitsPerSample = 32,
  kMaxSamplesPerPacket = kMaxSampleRate,
  kMaxPacketSizeInBytes =
      (kMaxBitsPerSample / 8) * kMaxChannels * kMaxSamplesPerPacket,

  
  kMaxFramesPerSecond = 1000,
};

}  

#endif 
