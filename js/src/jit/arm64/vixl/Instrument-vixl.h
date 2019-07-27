

























#ifndef VIXL_A64_INSTRUMENT_A64_H_
#define VIXL_A64_INSTRUMENT_A64_H_

#include "jit/arm64/vixl/Constants-vixl.h"
#include "jit/arm64/vixl/Decoder-vixl.h"
#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Utils-vixl.h"

namespace vixl {

const int kCounterNameMaxLength = 256;
const uint64_t kDefaultInstrumentationSamplingPeriod = 1 << 22;


enum InstrumentState {
  InstrumentStateDisable = 0,
  InstrumentStateEnable = 1
};


enum CounterType {
  Gauge = 0,      
  Cumulative = 1  
};


class Counter {
 public:
  explicit Counter(const char* name, CounterType type = Gauge);

  void Increment();
  void Enable();
  void Disable();
  bool IsEnabled();
  uint64_t count();
  const char* name();
  CounterType type();

 private:
  char name_[kCounterNameMaxLength];
  uint64_t count_;
  bool enabled_;
  CounterType type_;
};


class Instrument: public DecoderVisitor {
 public:
  explicit Instrument(const char* datafile = NULL,
    uint64_t sample_period = kDefaultInstrumentationSamplingPeriod);
  ~Instrument();

  void Enable();
  void Disable();

  
  #define DECLARE(A) void Visit##A(const Instruction* instr);
  VISITOR_LIST(DECLARE)
  #undef DECLARE

 private:
  void Update();
  void DumpCounters();
  void DumpCounterNames();
  void DumpEventMarker(unsigned marker);
  void HandleInstrumentationEvent(unsigned event);
  Counter* GetCounter(const char* name);

  void InstrumentLoadStore(const Instruction* instr);
  void InstrumentLoadStorePair(const Instruction* instr);

  std::list<Counter*> counters_;

  FILE *output_stream_;
  uint64_t sample_period_;
};

}  

#endif  
