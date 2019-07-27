



#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {
namespace bpf_dsl {

class SANDBOX_EXPORT DumpBPF {
 public:
  
  static void PrintProgram(const CodeGen::Program& program);
};

}  
}  
