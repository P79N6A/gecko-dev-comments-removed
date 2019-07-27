

























#include "jit/arm64/vixl/Instrument-vixl.h"

namespace vixl {

Counter::Counter(const char* name, CounterType type)
    : count_(0), enabled_(false), type_(type) {
  VIXL_ASSERT(name != NULL);
  strncpy(name_, name, kCounterNameMaxLength);
}


void Counter::Enable() {
  enabled_ = true;
}


void Counter::Disable() {
  enabled_ = false;
}


bool Counter::IsEnabled() {
  return enabled_;
}


void Counter::Increment() {
  if (enabled_) {
    count_++;
  }
}


uint64_t Counter::count() {
  uint64_t result = count_;
  if (type_ == Gauge) {
    
    count_ = 0;
  }
  return result;
}


const char* Counter::name() {
  return name_;
}


CounterType Counter::type() {
  return type_;
}


typedef struct {
  const char* name;
  CounterType type;
} CounterDescriptor;


static const CounterDescriptor kCounterList[] = {
  {"Instruction", Cumulative},

  {"Move Immediate", Gauge},
  {"Add/Sub DP", Gauge},
  {"Logical DP", Gauge},
  {"Other Int DP", Gauge},
  {"FP DP", Gauge},

  {"Conditional Select", Gauge},
  {"Conditional Compare", Gauge},

  {"Unconditional Branch", Gauge},
  {"Compare and Branch", Gauge},
  {"Test and Branch", Gauge},
  {"Conditional Branch", Gauge},

  {"Load Integer", Gauge},
  {"Load FP", Gauge},
  {"Load Pair", Gauge},
  {"Load Literal", Gauge},

  {"Store Integer", Gauge},
  {"Store FP", Gauge},
  {"Store Pair", Gauge},

  {"PC Addressing", Gauge},
  {"Other", Gauge},
};


Instrument::Instrument(const char* datafile, uint64_t sample_period)
    : output_stream_(stdout), sample_period_(sample_period) {

  
  
  if (datafile != NULL) {
    output_stream_ = fopen(datafile, "w");
    if (output_stream_ == NULL) {
      printf("Can't open output file %s. Using stdout.\n", datafile);
      output_stream_ = stdout;
    }
  }

  static const int num_counters =
    sizeof(kCounterList) / sizeof(CounterDescriptor);

  
  fprintf(output_stream_, "# counters=%d\n", num_counters);
  fprintf(output_stream_, "# sample_period=%" PRIu64 "\n", sample_period_);

  
  for (int i = 0; i < num_counters; i++) {
    Counter* counter = new Counter(kCounterList[i].name, kCounterList[i].type);
    counters_.push_back(counter);
  }

  DumpCounterNames();
}


Instrument::~Instrument() {
  
  DumpCounters();

  
  std::list<Counter*>::iterator it;
  for (it = counters_.begin(); it != counters_.end(); it++) {
    delete *it;
  }

  if (output_stream_ != stdout) {
    fclose(output_stream_);
  }
}


void Instrument::Update() {
  
  
  static Counter* counter = GetCounter("Instruction");
  VIXL_ASSERT(counter->type() == Cumulative);
  counter->Increment();

  if (counter->IsEnabled() && (counter->count() % sample_period_) == 0) {
    DumpCounters();
  }
}


void Instrument::DumpCounters() {
  
  
  std::list<Counter*>::const_iterator it;
  for (it = counters_.begin(); it != counters_.end(); it++) {
    fprintf(output_stream_, "%" PRIu64 ",", (*it)->count());
  }
  fprintf(output_stream_, "\n");
  fflush(output_stream_);
}


void Instrument::DumpCounterNames() {
  
  
  std::list<Counter*>::const_iterator it;
  for (it = counters_.begin(); it != counters_.end(); it++) {
    fprintf(output_stream_, "%s,", (*it)->name());
  }
  fprintf(output_stream_, "\n");
  fflush(output_stream_);
}


void Instrument::HandleInstrumentationEvent(unsigned event) {
  switch (event) {
    case InstrumentStateEnable: Enable(); break;
    case InstrumentStateDisable: Disable(); break;
    default: DumpEventMarker(event);
  }
}


void Instrument::DumpEventMarker(unsigned marker) {
  
  
  static Counter* counter = GetCounter("Instruction");

  fprintf(output_stream_, "# %c%c @ %" PRId64 "\n", marker & 0xff,
          (marker >> 8) & 0xff, counter->count());
}


Counter* Instrument::GetCounter(const char* name) {
  
  std::list<Counter*>::const_iterator it;
  for (it = counters_.begin(); it != counters_.end(); it++) {
    if (strcmp((*it)->name(), name) == 0) {
      return *it;
    }
  }

  
  
  static const char* error_message =
    "# Error: Unknown counter \"%s\". Exiting.\n";
  fprintf(stderr, error_message, name);
  fprintf(output_stream_, error_message, name);
  exit(1);
}


void Instrument::Enable() {
  std::list<Counter*>::iterator it;
  for (it = counters_.begin(); it != counters_.end(); it++) {
    (*it)->Enable();
  }
}


void Instrument::Disable() {
  std::list<Counter*>::iterator it;
  for (it = counters_.begin(); it != counters_.end(); it++) {
    (*it)->Disable();
  }
}


void Instrument::VisitPCRelAddressing(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("PC Addressing");
  counter->Increment();
}


void Instrument::VisitAddSubImmediate(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Add/Sub DP");
  counter->Increment();
}


void Instrument::VisitLogicalImmediate(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Logical DP");
  counter->Increment();
}


void Instrument::VisitMoveWideImmediate(const Instruction* instr) {
  Update();
  static Counter* counter = GetCounter("Move Immediate");

  if (instr->IsMovn() && (instr->Rd() == kZeroRegCode)) {
    unsigned imm = instr->ImmMoveWide();
    HandleInstrumentationEvent(imm);
  } else {
    counter->Increment();
  }
}


void Instrument::VisitBitfield(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other Int DP");
  counter->Increment();
}


void Instrument::VisitExtract(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other Int DP");
  counter->Increment();
}


void Instrument::VisitUnconditionalBranch(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Unconditional Branch");
  counter->Increment();
}


void Instrument::VisitUnconditionalBranchToRegister(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Unconditional Branch");
  counter->Increment();
}


void Instrument::VisitCompareBranch(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Compare and Branch");
  counter->Increment();
}


void Instrument::VisitTestBranch(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Test and Branch");
  counter->Increment();
}


void Instrument::VisitConditionalBranch(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Conditional Branch");
  counter->Increment();
}


void Instrument::VisitSystem(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other");
  counter->Increment();
}


void Instrument::VisitException(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other");
  counter->Increment();
}


void Instrument::InstrumentLoadStorePair(const Instruction* instr) {
  static Counter* load_pair_counter = GetCounter("Load Pair");
  static Counter* store_pair_counter = GetCounter("Store Pair");

  if (instr->Mask(LoadStorePairLBit) != 0) {
    load_pair_counter->Increment();
  } else {
    store_pair_counter->Increment();
  }
}


void Instrument::VisitLoadStorePairPostIndex(const Instruction* instr) {
  Update();
  InstrumentLoadStorePair(instr);
}


void Instrument::VisitLoadStorePairOffset(const Instruction* instr) {
  Update();
  InstrumentLoadStorePair(instr);
}


void Instrument::VisitLoadStorePairPreIndex(const Instruction* instr) {
  Update();
  InstrumentLoadStorePair(instr);
}


void Instrument::VisitLoadStorePairNonTemporal(const Instruction* instr) {
  Update();
  InstrumentLoadStorePair(instr);
}


void Instrument::VisitLoadStoreExclusive(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other");
  counter->Increment();
}


void Instrument::VisitLoadLiteral(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Load Literal");
  counter->Increment();
}


void Instrument::InstrumentLoadStore(const Instruction* instr) {
  static Counter* load_int_counter = GetCounter("Load Integer");
  static Counter* store_int_counter = GetCounter("Store Integer");
  static Counter* load_fp_counter = GetCounter("Load FP");
  static Counter* store_fp_counter = GetCounter("Store FP");

  switch (instr->Mask(LoadStoreOpMask)) {
    case STRB_w:    
    case STRH_w:    
    case STR_w:     
    case STR_x:     store_int_counter->Increment(); break;
    case STR_s:     
    case STR_d:     store_fp_counter->Increment(); break;
    case LDRB_w:    
    case LDRH_w:    
    case LDR_w:     
    case LDR_x:     
    case LDRSB_x:   
    case LDRSH_x:   
    case LDRSW_x:   
    case LDRSB_w:   
    case LDRSH_w:   load_int_counter->Increment(); break;
    case LDR_s:     
    case LDR_d:     load_fp_counter->Increment(); break;
  }
}


void Instrument::VisitLoadStoreUnscaledOffset(const Instruction* instr) {
  Update();
  InstrumentLoadStore(instr);
}


void Instrument::VisitLoadStorePostIndex(const Instruction* instr) {
  USE(instr);
  Update();
  InstrumentLoadStore(instr);
}


void Instrument::VisitLoadStorePreIndex(const Instruction* instr) {
  Update();
  InstrumentLoadStore(instr);
}


void Instrument::VisitLoadStoreRegisterOffset(const Instruction* instr) {
  Update();
  InstrumentLoadStore(instr);
}


void Instrument::VisitLoadStoreUnsignedOffset(const Instruction* instr) {
  Update();
  InstrumentLoadStore(instr);
}


void Instrument::VisitLogicalShifted(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Logical DP");
  counter->Increment();
}


void Instrument::VisitAddSubShifted(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Add/Sub DP");
  counter->Increment();
}


void Instrument::VisitAddSubExtended(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Add/Sub DP");
  counter->Increment();
}


void Instrument::VisitAddSubWithCarry(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Add/Sub DP");
  counter->Increment();
}


void Instrument::VisitConditionalCompareRegister(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Conditional Compare");
  counter->Increment();
}


void Instrument::VisitConditionalCompareImmediate(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Conditional Compare");
  counter->Increment();
}


void Instrument::VisitConditionalSelect(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Conditional Select");
  counter->Increment();
}


void Instrument::VisitDataProcessing1Source(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other Int DP");
  counter->Increment();
}


void Instrument::VisitDataProcessing2Source(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other Int DP");
  counter->Increment();
}


void Instrument::VisitDataProcessing3Source(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other Int DP");
  counter->Increment();
}


void Instrument::VisitFPCompare(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitFPConditionalCompare(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Conditional Compare");
  counter->Increment();
}


void Instrument::VisitFPConditionalSelect(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Conditional Select");
  counter->Increment();
}


void Instrument::VisitFPImmediate(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitFPDataProcessing1Source(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitFPDataProcessing2Source(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitFPDataProcessing3Source(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitFPIntegerConvert(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitFPFixedPointConvert(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("FP DP");
  counter->Increment();
}


void Instrument::VisitUnallocated(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other");
  counter->Increment();
}


void Instrument::VisitUnimplemented(const Instruction* instr) {
  USE(instr);
  Update();
  static Counter* counter = GetCounter("Other");
  counter->Increment();
}


}  
