




























#include <sys/ucontext.h>

#include "breakpad_googletest_includes.h"
#include "common/android/ucontext_constants.h"

TEST(AndroidUContext, GRegsOffset) {
#ifdef __arm__
  
  
  ASSERT_EQ(static_cast<size_t>(MCONTEXT_GREGS_OFFSET),
            offsetof(ucontext_t,uc_mcontext.arm_r0));
#elif defined(__i386__)
  ASSERT_EQ(static_cast<size_t>(MCONTEXT_GREGS_OFFSET),
            offsetof(ucontext_t,uc_mcontext.gregs));
#define CHECK_REG(x) \
  ASSERT_EQ(static_cast<size_t>(MCONTEXT_##x##_OFFSET),         \
            offsetof(ucontext_t,uc_mcontext.gregs[REG_##x]))
  CHECK_REG(GS);
  CHECK_REG(FS);
  CHECK_REG(ES);
  CHECK_REG(DS);
  CHECK_REG(EDI);
  CHECK_REG(ESI);
  CHECK_REG(EBP);
  CHECK_REG(ESP);
  CHECK_REG(EBX);
  CHECK_REG(EDX);
  CHECK_REG(ECX);
  CHECK_REG(EAX);
  CHECK_REG(TRAPNO);
  CHECK_REG(ERR);
  CHECK_REG(EIP);
  CHECK_REG(CS);
  CHECK_REG(EFL);
  CHECK_REG(UESP);
  CHECK_REG(SS);

  ASSERT_EQ(static_cast<size_t>(UCONTEXT_FPREGS_OFFSET),
            offsetof(ucontext_t,uc_mcontext.fpregs));

  ASSERT_EQ(static_cast<size_t>(UCONTEXT_FPREGS_MEM_OFFSET),
            offsetof(ucontext_t,__fpregs_mem));
#else
  ASSERT_EQ(static_cast<size_t>(MCONTEXT_GREGS_OFFSET),
            offsetof(ucontext_t,uc_mcontext.gregs));
#endif
}

TEST(AndroidUContext, SigmakOffset) {
  ASSERT_EQ(static_cast<size_t>(UCONTEXT_SIGMASK_OFFSET),
            offsetof(ucontext_t,uc_sigmask));
}
