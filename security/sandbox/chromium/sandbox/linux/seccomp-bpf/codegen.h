



#ifndef SANDBOX_LINUX_SECCOMP_BPF_CODEGEN_H__
#define SANDBOX_LINUX_SECCOMP_BPF_CODEGEN_H__

#include <stdint.h>

#include <map>
#include <vector>

#include "sandbox/sandbox_export.h"

struct sock_filter;

namespace sandbox {
struct BasicBlock;
struct Instruction;

typedef std::vector<Instruction*> Instructions;
typedef std::vector<BasicBlock*> BasicBlocks;
typedef std::map<const Instruction*, int> BranchTargets;
typedef std::map<const Instruction*, BasicBlock*> TargetsToBlocks;
typedef std::map<const BasicBlock*, int> IncomingBranches;































class SANDBOX_EXPORT CodeGen {
 public:
  
  
  typedef std::vector<struct sock_filter> Program;

  CodeGen();
  ~CodeGen();

  
  
  
  
  Instruction* MakeInstruction(uint16_t code,
                               uint32_t k,
                               Instruction* next = nullptr);
  Instruction* MakeInstruction(uint16_t code,
                               uint32_t k,
                               Instruction* jt,
                               Instruction* jf);

  
  
  
  void Compile(Instruction* instructions, Program* program);

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

  
  
  void ConcatenateBasicBlocks(const BasicBlocks&, Program* program);

  
  
  
  
  Instructions instructions_;
  BasicBlocks basic_blocks_;

  
  
  bool compiled_;
};

}  

#endif  
