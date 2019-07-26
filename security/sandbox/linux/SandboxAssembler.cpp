





#include "SandboxAssembler.h"
#include "linux_seccomp.h"
#include "mozilla/NullPtr.h"
#include <errno.h>

using namespace sandbox;

namespace mozilla {

SandboxAssembler::SandboxAssembler()
{
  mTail = LoadSyscall(nullptr);
  mTailAlt = nullptr;

  mHead = LoadArch(JmpEq(SECCOMP_ARCH, mTail, RetKill()));
}

void
SandboxAssembler::AppendCheck(Instruction *aCheck,
                              Instruction *aNewTail,
                              Instruction *aNewTailAlt)
{
  mCode.JoinInstructions(mTail, aCheck);
  if (mTailAlt != nullptr) {
    mCode.JoinInstructions(mTailAlt, aCheck);
  }
  mTail = aNewTail;
  mTailAlt = aNewTailAlt;
}

void
SandboxAssembler::Handle(const Condition &aCond, Instruction *aResult)
{
  Instruction *checkArg, *noMatch;

  if (!aCond.mCheckingArg) {
    checkArg = aResult;
    noMatch = nullptr;
  } else {
    const int8_t arg = aCond.mArgChecked;
    noMatch = LoadSyscall(nullptr);
    Instruction *checkArgLo = noMatch;

    
    for (size_t i = aCond.mArgValues.size(); i > 0; --i) {
      checkArgLo = JmpEq(aCond.mArgValues[i - 1], aResult, checkArgLo);
    }
    checkArgLo = LoadArgLo(arg, checkArgLo);

    checkArg = LoadArgHi(arg, JmpEq(0, checkArgLo, RetKill()));
  }
  Instruction *check = JmpEq(aCond.mSyscallNr, checkArg, nullptr);
  AppendCheck(check, check, noMatch);
}

void
SandboxAssembler::Finish()
{
  AppendCheck(RetKill(), nullptr, nullptr);
}

void
SandboxAssembler::Compile(std::vector<struct sock_filter> *aProgram,
                          bool aPrint)
{
  mCode.Compile(mHead, aProgram);
  if (aPrint) {
    mCode.PrintProgram(*aProgram);
  }
}

SandboxAssembler::~SandboxAssembler()
{
  
}

Instruction *
SandboxAssembler::LoadArch(Instruction *aNext)
{
  return mCode.MakeInstruction(BPF_LD + BPF_W + BPF_ABS,
                               SECCOMP_ARCH_IDX,
                               aNext);
}

Instruction *
SandboxAssembler::LoadSyscall(Instruction *aNext)
{
  return mCode.MakeInstruction(BPF_LD + BPF_W + BPF_ABS,
                               SECCOMP_NR_IDX,
                               aNext);
}

Instruction *
SandboxAssembler::LoadArgHi(int aArg, Instruction *aNext)
{
  return mCode.MakeInstruction(BPF_LD + BPF_W + BPF_ABS,
                               SECCOMP_ARG_MSB_IDX(aArg),
                               aNext);
}

Instruction *
SandboxAssembler::LoadArgLo(int aArg, Instruction *aNext)
{
  return mCode.MakeInstruction(BPF_LD + BPF_W + BPF_ABS,
                               SECCOMP_ARG_LSB_IDX(aArg),
                               aNext);
}

Instruction *
SandboxAssembler::JmpEq(uint32_t aValue, Instruction *aThen, Instruction *aElse)
{
  return mCode.MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K,
                               aValue, aThen, aElse);
}

Instruction *
SandboxAssembler::RetAllow()
{
  return mCode.MakeInstruction(BPF_RET + BPF_K,
                               SECCOMP_RET_ALLOW,
                               nullptr);
}

Instruction *
SandboxAssembler::RetDeny(int aErrno)
{
  return mCode.MakeInstruction(BPF_RET + BPF_K,
                               SECCOMP_RET_ERRNO + aErrno,
                               nullptr);
}

Instruction *
SandboxAssembler::RetKill()
{
  return mCode.MakeInstruction(BPF_RET + BPF_K,
#ifdef MOZ_CONTENT_SANDBOX_REPORTER
                               SECCOMP_RET_TRAP,
#else
                               SECCOMP_RET_KILL,
#endif
                               nullptr);
}

} 
