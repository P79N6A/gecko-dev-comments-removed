





#ifndef test_io_h_
#define test_io_h_

#include <string.h>
#include <map>
#include <memory>
#include <queue>
#include <string>

namespace nss_test {

struct Packet;
class DummyPrSocket;  


class Inspector {
 public:
  virtual ~Inspector() {}

  virtual void Inspect(DummyPrSocket* adapter, const void* data,
                       size_t len) = 0;
};

enum Mode { STREAM, DGRAM };

class DummyPrSocket {
 public:
  ~DummyPrSocket() { delete inspector_; }

  static PRFileDesc* CreateFD(const std::string& name,
                              Mode mode);  
  static DummyPrSocket* GetAdapter(PRFileDesc* fd);

  void SetPeer(DummyPrSocket* peer) { peer_ = peer; }

  void SetInspector(Inspector* inspector) { inspector_ = inspector; }

  void PacketReceived(const void* data, int32_t len);
  int32_t Read(void* data, int32_t len);
  int32_t Recv(void* buf, int32_t buflen);
  int32_t Write(const void* buf, int32_t length);
  int32_t WriteDirect(const void* buf, int32_t length);

  Mode mode() const { return mode_; }
  bool readable() { return !input_.empty(); }
  bool writable() { return true; }

 private:
  DummyPrSocket(const std::string& name, Mode mode)
      : name_(name),
        mode_(mode),
        peer_(nullptr),
        input_(),
        inspector_(nullptr) {}

  const std::string name_;
  Mode mode_;
  DummyPrSocket* peer_;
  std::queue<Packet*> input_;
  Inspector* inspector_;
};


class PollTarget {};

enum Event { READABLE_EVENT, TIMER_EVENT  };

typedef void (*PollCallback)(PollTarget*, Event);

class Poller {
 public:
  static Poller* Instance();  
  static void Shutdown();     

  class Timer {
   public:
    Timer(PRTime deadline, PollTarget* target, PollCallback callback)
        : deadline_(deadline), target_(target), callback_(callback) {}
    void Cancel() { callback_ = nullptr; }

    PRTime deadline_;
    PollTarget* target_;
    PollCallback callback_;
  };

  void Wait(Event event, DummyPrSocket* adapter, PollTarget* target,
            PollCallback cb);
  void SetTimer(uint32_t timer_ms, PollTarget* target, PollCallback cb,
                Timer** handle);
  bool Poll();

 private:
  Poller() : waiters_(), timers_() {}

  class Waiter {
   public:
    Waiter(DummyPrSocket* io) : io_(io) {
      memset(&callbacks_[0], 0, sizeof(callbacks_));
    }

    void WaitFor(Event event, PollCallback callback);

    DummyPrSocket* io_;
    PollTarget* targets_[TIMER_EVENT];
    PollCallback callbacks_[TIMER_EVENT];
  };

  class TimerComparator {
   public:
    bool operator()(const Timer* lhs, const Timer* rhs) {
      return lhs->deadline_ > rhs->deadline_;
    }
  };

  static Poller* instance;
  std::map<DummyPrSocket*, Waiter*> waiters_;
  std::priority_queue<Timer*, std::vector<Timer*>, TimerComparator> timers_;
};

}  

#endif
