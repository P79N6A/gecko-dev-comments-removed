



#ifndef SANDBOX_LINUX_SECCOMP_BPF_CODEGEN_H__
#define SANDBOX_LINUX_SECCOMP_BPF_CODEGEN_H__

#include <map>
#include <set>
#include <vector>

#include "sandbox/linux/sandbox_export.h"
#include "sandbox/linux/seccomp-bpf/basicblock.h"
#include "sandbox/linux/seccomp-bpf/instruction.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"

namespace sandbox {

typedef std::vector<Instruction*> Instructions;
typedef std::vector<BasicBlock*> BasicBlocks;
typedef std::map<const Instruction*, int> BranchTargets;
typedef std::map<const Instruction*, BasicBlock*> TargetsToBlocks;
typedef std::map<const BasicBlock*, int> IncomingBranches;
































class SANDBOX_EXPORT CodeGen {
 public:
  CodeGen();
  ~CodeGen();

  
  
  static void PrintProgram(const SandboxBPF::Program& program);

  
  
  
  
  Instruction* MakeInstruction(uint16_t code,
                               uint32_t k,
                               Instruction* next = NULL);
  Instruction* MakeInstruction(uint16_t code, const ErrorCode& err);
  Instruction* MakeInstruction(uint16_t code,
                               uint32_t k,
                               Instruction* jt,
                               Instruction* jf);

  
  
  
  void JoinInstructions(Instruction* head, Instruction* tail);

  
  
  
  
  
  
  void Traverse(Instruction*, void (*fnc)(Instruction*, void* aux), void* aux);

  
  
  
  void Compile(Instruction* instructions, SandboxBPF::Program* program);

 private:
  friend class CodeGenUnittestHelper;

  
  void FindBranchTargets(const Instruction& instructions,
                         BranchTargets* branch_targets);

  
  
  
  
  BasicBlock* MakeBasicBlock(Instruction* head, Instruction* tail);

  
  
  void AddBasicBlock(Instruction* head,
                     Instruction* tail,
                     const BranchTargets& branch_targets,
                     TargetsToBlocks* basic_blocks,
                     BasicBlock** first_block);

  
  BasicBlock* CutGraphIntoBasicBlocks(Instruction* instructions,
                                      const BranchTargets& branch_targets,
                                      TargetsToBlocks* blocks);

  
  void MergeTails(TargetsToBlocks* blocks);

  
  void ComputeIncomingBranches(BasicBlock* block,
                               const TargetsToBlocks& targets_to_blocks,
                               IncomingBranches* incoming_branches);

  
  
  void TopoSortBasicBlocks(BasicBlock* first_block,
                           const TargetsToBlocks& blocks,
                           BasicBlocks* basic_blocks);

  
  
  
  void ComputeRelativeJumps(BasicBlocks* basic_blocks,
                            const TargetsToBlocks& targets_to_blocks);

  
  
  void ConcatenateBasicBlocks(const BasicBlocks&, SandboxBPF::Program* program);

  
  
  
  
  Instructions instructions_;
  BasicBlocks basic_blocks_;

  
  
  bool compiled_;
};

}  

#endif  
