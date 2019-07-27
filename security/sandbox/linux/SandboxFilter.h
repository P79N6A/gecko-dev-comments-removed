





#ifndef mozilla_SandboxFilter_h
#define mozilla_SandboxFilter_h

#include "mozilla/Atomics.h"
#include "mozilla/UniquePtr.h"

namespace sandbox {
namespace bpf_dsl {
class Policy;
}
}

namespace mozilla {

#ifdef MOZ_CONTENT_SANDBOX
UniquePtr<sandbox::bpf_dsl::Policy> GetContentSandboxPolicy();
#endif

#ifdef MOZ_GMP_SANDBOX
struct SandboxOpenedFile {
  const char *mPath;
  Atomic<int> mFd;
};

UniquePtr<sandbox::bpf_dsl::Policy> GetMediaSandboxPolicy(SandboxOpenedFile* aPlugin);
#endif

} 

#endif
