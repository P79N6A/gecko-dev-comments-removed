

























#include "mozilla/DebugOnly.h"

#include "jit/arm64/vixl/Debugger-vixl.h"
#include "jit/arm64/vixl/Simulator-vixl.h"
#include "jit/IonTypes.h"
#include "vm/Runtime.h"

namespace vixl {


using mozilla::DebugOnly;
using js::jit::ABIFunctionType;


Simulator::Simulator() {
  decoder_ = js_new<Decoder>();
  if (!decoder_) {
    MOZ_ReportAssertionFailure("[unhandlable oom] Decoder", __FILE__, __LINE__);
    MOZ_CRASH();
  }

  
  
  
  this->init(decoder_, stdout);
}


Simulator::Simulator(Decoder* decoder, FILE* stream) {
  this->init(decoder, stream);
}


void Simulator::ResetState() {
  
  nzcv_ = SimSystemRegister::DefaultValueFor(NZCV);
  fpcr_ = SimSystemRegister::DefaultValueFor(FPCR);

  
  pc_ = NULL;
  pc_modified_ = false;
  for (unsigned i = 0; i < kNumberOfRegisters; i++) {
    set_xreg(i, 0xbadbeef);
  }
  
  uint64_t nan_bits = UINT64_C(0x7ff0dead7f8beef1);
  VIXL_ASSERT(IsSignallingNaN(rawbits_to_double(nan_bits & kDRegMask)));
  VIXL_ASSERT(IsSignallingNaN(rawbits_to_float(nan_bits & kSRegMask)));
  for (unsigned i = 0; i < kNumberOfFPRegisters; i++) {
    set_dreg_bits(i, nan_bits);
  }
  
  set_lr(kEndOfSimAddress);
  set_resume_pc(nullptr);
}


void Simulator::init(Decoder* decoder, FILE* stream) {
  
  VIXL_ASSERT((static_cast<int32_t>(-1) >> 1) == -1);
  VIXL_ASSERT((static_cast<uint32_t>(-1) >> 1) == 0x7FFFFFFF);

  
  decoder_ = decoder;
  decoder_->AppendVisitor(this);

  stream_ = stream;
  print_disasm_ = new PrintDisassembler(stream_);
  set_coloured_trace(false);
  trace_parameters_ = LOG_NONE;

  ResetState();

  
  stack_ = new byte[stack_size_];
  stack_limit_ = stack_ + stack_protection_size_;
  
  
  byte * tos = stack_ + stack_size_;
  
  tos -= stack_protection_size_;
  
  tos = AlignDown(tos, 16);
  set_sp(tos);

  
  instrumentation_ = new Instrument("vixl_stats.csv", 10);

  
  
  
  print_exclusive_access_warning_ = true;

  lock_ = PR_NewLock();
  if (!lock_)
    MOZ_CRASH("Could not allocate simulator lock.");
  lockOwner_ = nullptr;
  redirection_ = nullptr;
}


Simulator* Simulator::Current() {
  return js::TlsPerThreadData.get()->simulator();
}


Simulator* Simulator::Create() {
  Decoder* decoder = js_new<vixl::Decoder>();
  if (!decoder) {
    MOZ_ReportAssertionFailure("[unhandlable oom] Decoder", __FILE__, __LINE__);
    MOZ_CRASH();
  }

  
  
  
  if (getenv("USE_DEBUGGER") != nullptr) {
    Debugger* debugger = js_new<Debugger>(decoder, stdout);
    if (!debugger) {
      MOZ_ReportAssertionFailure("[unhandlable oom] Decoder", __FILE__, __LINE__);
      MOZ_CRASH();
    }
    return debugger;
  }

  Simulator* sim = js_new<Simulator>();
  if (!sim) {
    MOZ_CRASH("NEED SIMULATOR");
    return nullptr;
  }
  sim->init(decoder, stdout);

  return sim;
}


void Simulator::Destroy(Simulator* sim) {
  js_delete(sim);
}


void Simulator::ExecuteInstruction() {
  
  VIXL_ASSERT(IsWordAligned(pc_));
  decoder_->Decode(pc_);
  const Instruction* rpc = resume_pc_;
  increment_pc();

  if (MOZ_UNLIKELY(rpc)) {
    JSRuntime::innermostAsmJSActivation()->setResumePC((void*)pc());
    set_pc(rpc);
    
    
    
    pc_modified_ = false;
    resume_pc_ = nullptr;
  }
}


uintptr_t Simulator::stackLimit() const {
  return reinterpret_cast<uintptr_t>(stack_limit_);
}


uintptr_t* Simulator::addressOfStackLimit() {
  return (uintptr_t*)&stack_limit_;
}


bool Simulator::overRecursed(uintptr_t newsp) const {
  if (newsp)
    newsp = xreg(31, Reg31IsStackPointer);
  return newsp <= stackLimit();
}


bool Simulator::overRecursedWithExtra(uint32_t extra) const {
  uintptr_t newsp = xreg(31, Reg31IsStackPointer) - extra;
  return newsp <= stackLimit();
}


void Simulator::set_resume_pc(void* new_resume_pc) {
  resume_pc_ = AddressUntag(reinterpret_cast<Instruction*>(new_resume_pc));
}


int64_t Simulator::call(uint8_t* entry, int argument_count, ...) {
  va_list parameters;
  va_start(parameters, argument_count);

  
  VIXL_ASSERT(argument_count <= 8);
  
  
  
  
  if (argument_count == 8) {
      
      set_xreg(0, va_arg(parameters, int64_t));
      
      set_xreg(1, va_arg(parameters, unsigned));
      
      set_xreg(2, va_arg(parameters, int64_t));
      
      set_xreg(3, va_arg(parameters, int64_t));
      
      set_xreg(4, va_arg(parameters, int64_t));
      
      set_xreg(5, va_arg(parameters, int64_t));
      
      set_xreg(6, va_arg(parameters, unsigned));
      
      set_xreg(7, va_arg(parameters, int64_t));
  } else if (argument_count == 2) {
      
      set_xreg(0, va_arg(parameters, int64_t));
      
      set_xreg(1, va_arg(parameters, int64_t));
  } else if (argument_count == 1) { 
      
      set_xreg(0, va_arg(parameters, int64_t));
  } else {
      MOZ_CRASH("Unknown number of arguments");
  }

  va_end(parameters);

  
  VIXL_ASSERT(xreg(30) == int64_t(kEndOfSimAddress));

  
  DebugOnly<int64_t> entryStack = xreg(31, Reg31IsStackPointer);
  RunFrom((Instruction*)entry);
  DebugOnly<int64_t> exitStack = xreg(31, Reg31IsStackPointer);
  VIXL_ASSERT(entryStack == exitStack);

  int64_t result = xreg(0);
  if (getenv("USE_DEBUGGER"))
      printf("LEAVE\n");
  return result;
}



class AutoLockSimulatorCache
{
  friend class Simulator;

 public:
  explicit AutoLockSimulatorCache(Simulator* sim) : sim_(sim) {
    PR_Lock(sim_->lock_);
    VIXL_ASSERT(!sim_->lockOwner_);
#ifdef DEBUG
    sim_->lockOwner_ = PR_GetCurrentThread();
#endif
  }

  ~AutoLockSimulatorCache() {
#ifdef DEBUG
    VIXL_ASSERT(sim_->lockOwner_ == PR_GetCurrentThread());
    sim_->lockOwner_ = nullptr;
#endif
    PR_Unlock(sim_->lock_);
  }

 private:
   Simulator* const sim_;
};








class Redirection
{
  friend class Simulator;

  Redirection(void* nativeFunction, ABIFunctionType type, Simulator* sim)
    : nativeFunction_(nativeFunction),
    type_(type),
    next_(nullptr)
  {
    next_ = sim->redirection();
    
    sim->setRedirection(this);

    Instruction* instr = (Instruction*)(&svcInstruction_);
    vixl::Assembler::svc(instr, kCallRtRedirected);
  }

 public:
  void* addressOfSvcInstruction() { return &svcInstruction_; }
  void* nativeFunction() const { return nativeFunction_; }
  ABIFunctionType type() const { return type_; }

  static Redirection* Get(void* nativeFunction, ABIFunctionType type) {
    Simulator* sim = Simulator::Current();
    AutoLockSimulatorCache alsr(sim);

    
    

    Redirection* current = sim->redirection();
    for (; current != nullptr; current = current->next_) {
      if (current->nativeFunction_ == nativeFunction) {
        VIXL_ASSERT(current->type() == type);
        return current;
      }
    }

    Redirection* redir = (Redirection*)js_malloc(sizeof(Redirection));
    if (!redir) {
      MOZ_ReportAssertionFailure("[unhandlable oom] Simulator redirection", __FILE__, __LINE__);
      MOZ_CRASH();
    }
    new(redir) Redirection(nativeFunction, type, sim);
    return redir;
  }

  static const Redirection* FromSvcInstruction(const Instruction* svcInstruction) {
    const uint8_t* addrOfSvc = reinterpret_cast<const uint8_t*>(svcInstruction);
    const uint8_t* addrOfRedirection = addrOfSvc - offsetof(Redirection, svcInstruction_);
    return reinterpret_cast<const Redirection*>(addrOfRedirection);
  }

 private:
  void* nativeFunction_;
  uint32_t svcInstruction_;
  ABIFunctionType type_;
  Redirection* next_;
};


void Simulator::setRedirection(Redirection* redirection) {
  
  redirection_ = redirection;
}


Redirection* Simulator::redirection() const {
  return redirection_;
}


void* Simulator::RedirectNativeFunction(void* nativeFunction, ABIFunctionType type) {
  Redirection* redirection = Redirection::Get(nativeFunction, type);
  return redirection->addressOfSvcInstruction();
}


void Simulator::VisitException(const Instruction* instr) {
  switch (instr->Mask(ExceptionMask)) {
    case BRK: {
      int lowbit  = ImmException_offset;
      int highbit = ImmException_offset + ImmException_width - 1;
      HostBreakpoint(instr->Bits(highbit, lowbit));
      break;
    }
    case HLT:
      switch (instr->ImmException()) {
        case kUnreachableOpcode:
          DoUnreachable(instr);
          return;
        case kTraceOpcode:
          DoTrace(instr);
          return;
        case kLogOpcode:
          DoLog(instr);
          return;
        case kPrintfOpcode:
          DoPrintf(instr);
          return;
        default:
          HostBreakpoint();
          return;
      }
    case SVC:
      
      
      switch (instr->ImmException()) {
        case kCallRtRedirected:
          VisitCallRedirection(instr);
          return;
        case kMarkStackPointer:
          spStack_.append(xreg(31, Reg31IsStackPointer));
          return;
        case kCheckStackPointer: {
          int64_t current = xreg(31, Reg31IsStackPointer);
          int64_t expected = spStack_.popCopy();
          VIXL_ASSERT(current == expected);
          return;
        }
        default:
          VIXL_UNIMPLEMENTED();
      }
      break;
    default:
      VIXL_UNIMPLEMENTED();
  }
}


void Simulator::setGPR32Result(int32_t result) {
    set_wreg(0, result);
}


void Simulator::setGPR64Result(int64_t result) {
    set_xreg(0, result);
}


void Simulator::setFP32Result(float result) {
    set_sreg(0, result);
}


void Simulator::setFP64Result(double result) {
    set_dreg(0, result);
}


typedef int64_t (*Prototype_General0)();
typedef int64_t (*Prototype_General1)(int64_t arg0);
typedef int64_t (*Prototype_General2)(int64_t arg0, int64_t arg1);
typedef int64_t (*Prototype_General3)(int64_t arg0, int64_t arg1, int64_t arg2);
typedef int64_t (*Prototype_General4)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3);
typedef int64_t (*Prototype_General5)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3,
                                      int64_t arg4);
typedef int64_t (*Prototype_General6)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3,
                                      int64_t arg4, int64_t arg5);
typedef int64_t (*Prototype_General7)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3,
                                      int64_t arg4, int64_t arg5, int64_t arg6);
typedef int64_t (*Prototype_General8)(int64_t arg0, int64_t arg1, int64_t arg2, int64_t arg3,
                                      int64_t arg4, int64_t arg5, int64_t arg6, int64_t arg7);

typedef int64_t (*Prototype_Int_Double)(double arg0);
typedef int64_t (*Prototype_Int_IntDouble)(int32_t arg0, double arg1);

typedef float (*Prototype_Float32_Float32)(float arg0);

typedef double (*Prototype_Double_None)();
typedef double (*Prototype_Double_Double)(double arg0);
typedef double (*Prototype_Double_Int)(int32_t arg0);
typedef double (*Prototype_Double_DoubleInt)(double arg0, int64_t arg1);
typedef double (*Prototype_Double_IntDouble)(int64_t arg0, double arg1);
typedef double (*Prototype_Double_DoubleDouble)(double arg0, double arg1);
typedef double (*Prototype_Double_DoubleDoubleDouble)(double arg0, double arg1, double arg2);
typedef double (*Prototype_Double_DoubleDoubleDoubleDouble)(double arg0, double arg1,
                                                            double arg2, double arg3);



void
Simulator::VisitCallRedirection(const Instruction* instr)
{
  VIXL_ASSERT(instr->Mask(ExceptionMask) == SVC);
  VIXL_ASSERT(instr->ImmException() == kCallRtRedirected);

  const Redirection* redir = Redirection::FromSvcInstruction(instr);
  uintptr_t nativeFn = reinterpret_cast<uintptr_t>(redir->nativeFunction());

  
  
  

  
  DebugOnly<int64_t> x19 = xreg(19);
  DebugOnly<int64_t> x20 = xreg(20);
  DebugOnly<int64_t> x21 = xreg(21);
  DebugOnly<int64_t> x22 = xreg(22);
  DebugOnly<int64_t> x23 = xreg(23);
  DebugOnly<int64_t> x24 = xreg(24);
  DebugOnly<int64_t> x25 = xreg(25);
  DebugOnly<int64_t> x26 = xreg(26);
  DebugOnly<int64_t> x27 = xreg(27);
  DebugOnly<int64_t> x28 = xreg(28);
  DebugOnly<int64_t> x29 = xreg(29);
  DebugOnly<int64_t> savedSP = xreg(31, Reg31IsStackPointer);

  
  int64_t savedLR = xreg(30);

  
  
  set_xreg(30, int64_t(kEndOfSimAddress));

  
  int64_t x0 = xreg(0);
  int64_t x1 = xreg(1);
  int64_t x2 = xreg(2);
  int64_t x3 = xreg(3);
  int64_t x4 = xreg(4);
  int64_t x5 = xreg(5);
  int64_t x6 = xreg(6);
  int64_t x7 = xreg(7);
  double d0 = dreg(0);
  double d1 = dreg(1);
  double d2 = dreg(2);
  double d3 = dreg(3);
  float s0 = sreg(0);

  
  switch (redir->type()) {
    
    case js::jit::Args_General0: {
      int64_t ret = reinterpret_cast<Prototype_General0>(nativeFn)();
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General1: {
      int64_t ret = reinterpret_cast<Prototype_General1>(nativeFn)(x0);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General2: {
      int64_t ret = reinterpret_cast<Prototype_General2>(nativeFn)(x0, x1);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General3: {
      int64_t ret = reinterpret_cast<Prototype_General3>(nativeFn)(x0, x1, x2);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General4: {
      int64_t ret = reinterpret_cast<Prototype_General4>(nativeFn)(x0, x1, x2, x3);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General5: {
      int64_t ret = reinterpret_cast<Prototype_General5>(nativeFn)(x0, x1, x2, x3, x4);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General6: {
      int64_t ret = reinterpret_cast<Prototype_General6>(nativeFn)(x0, x1, x2, x3, x4, x5);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General7: {
      int64_t ret = reinterpret_cast<Prototype_General7>(nativeFn)(x0, x1, x2, x3, x4, x5, x6);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_General8: {
      int64_t ret = reinterpret_cast<Prototype_General8>(nativeFn)(x0, x1, x2, x3, x4, x5, x6, x7);
      setGPR64Result(ret);
      break;
    }

    
    case js::jit::Args_Int_Double: {
      int64_t ret = reinterpret_cast<Prototype_Int_Double>(nativeFn)(d0);
      setGPR64Result(ret);
      break;
    }
    case js::jit::Args_Int_IntDouble: {
      int64_t ret = reinterpret_cast<Prototype_Int_IntDouble>(nativeFn)(x0, d0);
      setGPR64Result(ret);
      break;
    }

    
    case js::jit::Args_Float32_Float32: {
      float ret = reinterpret_cast<Prototype_Float32_Float32>(nativeFn)(s0);
      setFP32Result(ret);
      break;
    }

    
    case js::jit::Args_Double_None: {
      double ret = reinterpret_cast<Prototype_Double_None>(nativeFn)();
      setFP64Result(ret);
      break;
    }
    case js::jit::Args_Double_Double: {
      double ret = reinterpret_cast<Prototype_Double_Double>(nativeFn)(d0);
      setFP64Result(ret);
      break;
    }
    case js::jit::Args_Double_Int: {
      double ret = reinterpret_cast<Prototype_Double_Int>(nativeFn)(x0);
      setFP64Result(ret);
      break;
    }
    case js::jit::Args_Double_DoubleInt: {
      double ret = reinterpret_cast<Prototype_Double_DoubleInt>(nativeFn)(d0, x0);
      setFP64Result(ret);
      break;
    }
    case js::jit::Args_Double_DoubleDouble: {
      double ret = reinterpret_cast<Prototype_Double_DoubleDouble>(nativeFn)(d0, d1);
      setFP64Result(ret);
      break;
    }
    case js::jit::Args_Double_DoubleDoubleDouble: {
      double ret = reinterpret_cast<Prototype_Double_DoubleDoubleDouble>(nativeFn)(d0, d1, d2);
      setFP64Result(ret);
      break;
    }
    case js::jit::Args_Double_DoubleDoubleDoubleDouble: {
      double ret = reinterpret_cast<Prototype_Double_DoubleDoubleDoubleDouble>(nativeFn)(d0, d1, d2, d3);
      setFP64Result(ret);
      break;
    }

    case js::jit::Args_Double_IntDouble: {
      double ret = reinterpret_cast<Prototype_Double_IntDouble>(nativeFn)(x0, d0);
      setFP64Result(ret);
      break;
    }

    default:
      MOZ_CRASH("Unknown function type.");
  }

  

  
  VIXL_ASSERT(xreg(19) == x19);
  VIXL_ASSERT(xreg(20) == x20);
  VIXL_ASSERT(xreg(21) == x21);
  VIXL_ASSERT(xreg(22) == x22);
  VIXL_ASSERT(xreg(23) == x23);
  VIXL_ASSERT(xreg(24) == x24);
  VIXL_ASSERT(xreg(25) == x25);
  VIXL_ASSERT(xreg(26) == x26);
  VIXL_ASSERT(xreg(27) == x27);
  VIXL_ASSERT(xreg(28) == x28);
  VIXL_ASSERT(xreg(29) == x29);

  
  VIXL_ASSERT(savedSP == xreg(31, Reg31IsStackPointer));

  
  set_lr(savedLR);
  set_pc((Instruction*)savedLR);
  if (getenv("USE_DEBUGGER"))
    printf("SVCRET\n");
}


}  


vixl::Simulator* js::PerThreadData::simulator() const {
  return runtime_->simulator();
}


vixl::Simulator* JSRuntime::simulator() const {
  return simulator_;
}


uintptr_t* JSRuntime::addressOfSimulatorStackLimit() {
  return simulator_->addressOfStackLimit();
}

