



#include <errno.h>

#include <algorithm>
#include <set>
#include <vector>

#include "sandbox/linux/seccomp-bpf/codegen.h"
#include "sandbox/linux/seccomp-bpf/sandbox_bpf.h"
#include "sandbox/linux/tests/unit_tests.h"

namespace sandbox {

class SandboxUnittestHelper : public SandboxBPF {
 public:
  typedef SandboxBPF::Program Program;
};



class CodeGenUnittestHelper : public CodeGen {
 public:
  void FindBranchTargets(const Instruction& instructions,
                         BranchTargets* branch_targets) {
    CodeGen::FindBranchTargets(instructions, branch_targets);
  }

  BasicBlock* CutGraphIntoBasicBlocks(Instruction* insns,
                                      const BranchTargets& branch_targets,
                                      TargetsToBlocks* blocks) {
    return CodeGen::CutGraphIntoBasicBlocks(insns, branch_targets, blocks);
  }

  void MergeTails(TargetsToBlocks* blocks) { CodeGen::MergeTails(blocks); }
};

enum { NO_FLAGS = 0x0000, HAS_MERGEABLE_TAILS = 0x0001, };

Instruction* SampleProgramOneInstruction(CodeGen* codegen, int* flags) {
  
  
  *flags = NO_FLAGS;
  return codegen->MakeInstruction(BPF_RET + BPF_K,
                                  ErrorCode(ErrorCode::ERR_ALLOWED));
}

Instruction* SampleProgramSimpleBranch(CodeGen* codegen, int* flags) {
  
  
  
  
  *flags = NO_FLAGS;
  return codegen->MakeInstruction(
      BPF_JMP + BPF_JEQ + BPF_K,
      42,
      codegen->MakeInstruction(BPF_RET + BPF_K, ErrorCode(EPERM)),
      codegen->MakeInstruction(BPF_RET + BPF_K,
                               ErrorCode(ErrorCode::ERR_ALLOWED)));
}

Instruction* SampleProgramAtypicalBranch(CodeGen* codegen, int* flags) {
  
  
  

  
  
  
  *flags = NO_FLAGS;

  Instruction* ret = codegen->MakeInstruction(
      BPF_RET + BPF_K, ErrorCode(ErrorCode::ERR_ALLOWED));
  return codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 42, ret, ret);
}

Instruction* SampleProgramComplex(CodeGen* codegen, int* flags) {
  
  
  
  
  
  
  
  
  
  *flags = HAS_MERGEABLE_TAILS;

  Instruction* insn0 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 42);
  SANDBOX_ASSERT(insn0);
  SANDBOX_ASSERT(insn0->code == BPF_LD + BPF_W + BPF_ABS);
  SANDBOX_ASSERT(insn0->k == 42);
  SANDBOX_ASSERT(insn0->next == NULL);

  Instruction* insn1 = codegen->MakeInstruction(BPF_JMP + BPF_JA, 0, insn0);
  SANDBOX_ASSERT(insn1);
  SANDBOX_ASSERT(insn1->code == BPF_JMP + BPF_JA);
  SANDBOX_ASSERT(insn1->jt_ptr == insn0);

  Instruction* insn2 = codegen->MakeInstruction(BPF_RET + BPF_K, ErrorCode(42));
  SANDBOX_ASSERT(insn2);
  SANDBOX_ASSERT(insn2->code == BPF_RET + BPF_K);
  SANDBOX_ASSERT(insn2->next == NULL);

  
  
  Instruction* insn3 = codegen->MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS,
      42,
      codegen->MakeInstruction(BPF_RET + BPF_K, ErrorCode(42)));

  Instruction* insn4 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 42, insn1, insn3);
  SANDBOX_ASSERT(insn4);
  SANDBOX_ASSERT(insn4->code == BPF_JMP + BPF_JEQ + BPF_K);
  SANDBOX_ASSERT(insn4->k == 42);
  SANDBOX_ASSERT(insn4->jt_ptr == insn1);
  SANDBOX_ASSERT(insn4->jf_ptr == insn3);

  codegen->JoinInstructions(insn0, insn2);
  SANDBOX_ASSERT(insn0->next == insn2);

  Instruction* insn5 =
      codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 23, insn4);
  SANDBOX_ASSERT(insn5);
  SANDBOX_ASSERT(insn5->code == BPF_LD + BPF_W + BPF_ABS);
  SANDBOX_ASSERT(insn5->k == 23);
  SANDBOX_ASSERT(insn5->next == insn4);

  
  
  
  
  
  
  Instruction* insn6 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 42, insn5, insn4);

  return insn6;
}

Instruction* SampleProgramConfusingTails(CodeGen* codegen, int* flags) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  *flags = NO_FLAGS;

  Instruction* i7 = codegen->MakeInstruction(BPF_RET, ErrorCode(1));
  Instruction* i6 = codegen->MakeInstruction(BPF_RET, ErrorCode(0));
  Instruction* i5 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i6, i7);
  Instruction* i4 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i5);
  Instruction* i3 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 2, i4, i5);
  Instruction* i2 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i3);
  Instruction* i1 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i2, i3);
  Instruction* i0 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 1, i1);

  return i0;
}

Instruction* SampleProgramConfusingTailsBasic(CodeGen* codegen, int* flags) {
  
  
  
  
  
  
  
  
  
  
  *flags = NO_FLAGS;

  Instruction* i5 = codegen->MakeInstruction(BPF_RET, ErrorCode(1));
  Instruction* i4 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i5);
  Instruction* i3 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 2, i4, i5);
  Instruction* i2 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i3);
  Instruction* i1 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i2, i3);
  Instruction* i0 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 1, i1);

  return i0;
}

Instruction* SampleProgramConfusingTailsMergeable(CodeGen* codegen,
                                                  int* flags) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  *flags = HAS_MERGEABLE_TAILS;

  Instruction* i7 = codegen->MakeInstruction(BPF_RET, ErrorCode(1));
  Instruction* i6 = codegen->MakeInstruction(BPF_RET, ErrorCode(0));
  Instruction* i5 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i6, i7);
  Instruction* i4 = codegen->MakeInstruction(BPF_RET, ErrorCode(42));
  Instruction* i3 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 2, i4, i5);
  Instruction* i2 = codegen->MakeInstruction(BPF_RET, ErrorCode(42));
  Instruction* i1 =
      codegen->MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i2, i3);
  Instruction* i0 = codegen->MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 1, i1);

  return i0;
}
void ForAllPrograms(void (*test)(CodeGenUnittestHelper*, Instruction*, int)) {
  Instruction* (*function_table[])(CodeGen* codegen, int* flags) = {
    SampleProgramOneInstruction,
    SampleProgramSimpleBranch,
    SampleProgramAtypicalBranch,
    SampleProgramComplex,
    SampleProgramConfusingTails,
    SampleProgramConfusingTailsBasic,
    SampleProgramConfusingTailsMergeable,
  };

  for (size_t i = 0; i < arraysize(function_table); ++i) {
    CodeGenUnittestHelper codegen;
    int flags = NO_FLAGS;
    Instruction *prg = function_table[i](&codegen, &flags);
    test(&codegen, prg, flags);
  }
}

void MakeInstruction(CodeGenUnittestHelper* codegen,
                     Instruction* program, int) {
  
}

SANDBOX_TEST(CodeGen, MakeInstruction) {
  ForAllPrograms(MakeInstruction);
}

void FindBranchTargets(CodeGenUnittestHelper* codegen, Instruction* prg, int) {
  BranchTargets branch_targets;
  codegen->FindBranchTargets(*prg, &branch_targets);

  
  
  
  
  
  
  std::vector<Instruction*> stack;
  std::set<Instruction*> all_instructions;
  std::set<Instruction*> target_instructions;
  BranchTargets::const_iterator end = branch_targets.end();
  for (Instruction* insn = prg;;) {
    all_instructions.insert(insn);
    if (BPF_CLASS(insn->code) == BPF_JMP) {
      target_instructions.insert(insn->jt_ptr);
      SANDBOX_ASSERT(insn->jt_ptr != NULL);
      SANDBOX_ASSERT(branch_targets.find(insn->jt_ptr) != end);
      if (BPF_OP(insn->code) != BPF_JA) {
        target_instructions.insert(insn->jf_ptr);
        SANDBOX_ASSERT(insn->jf_ptr != NULL);
        SANDBOX_ASSERT(branch_targets.find(insn->jf_ptr) != end);
        stack.push_back(insn->jf_ptr);
      }
      insn = insn->jt_ptr;
    } else if (BPF_CLASS(insn->code) == BPF_RET) {
      SANDBOX_ASSERT(insn->next == NULL);
      if (stack.empty()) {
        break;
      }
      insn = stack.back();
      stack.pop_back();
    } else {
      SANDBOX_ASSERT(insn->next != NULL);
      insn = insn->next;
    }
  }
  SANDBOX_ASSERT(target_instructions.size() == branch_targets.size());

  
  
  
  
  Instructions non_target_instructions(all_instructions.size() -
                                       target_instructions.size());
  set_difference(all_instructions.begin(),
                 all_instructions.end(),
                 target_instructions.begin(),
                 target_instructions.end(),
                 non_target_instructions.begin());
  for (Instructions::const_iterator iter = non_target_instructions.begin();
       iter != non_target_instructions.end();
       ++iter) {
    SANDBOX_ASSERT(branch_targets.find(*iter) == end);
  }
}

SANDBOX_TEST(CodeGen, FindBranchTargets) { ForAllPrograms(FindBranchTargets); }

void CutGraphIntoBasicBlocks(CodeGenUnittestHelper* codegen,
                             Instruction* prg,
                             int) {
  BranchTargets branch_targets;
  codegen->FindBranchTargets(*prg, &branch_targets);
  TargetsToBlocks all_blocks;
  BasicBlock* first_block =
      codegen->CutGraphIntoBasicBlocks(prg, branch_targets, &all_blocks);
  SANDBOX_ASSERT(first_block != NULL);
  SANDBOX_ASSERT(first_block->instructions.size() > 0);
  Instruction* first_insn = first_block->instructions[0];

  
  
  
  
  for (TargetsToBlocks::const_iterator bb_iter = all_blocks.begin();
       bb_iter != all_blocks.end();
       ++bb_iter) {
    BasicBlock* bb = bb_iter->second;
    SANDBOX_ASSERT(bb != NULL);
    SANDBOX_ASSERT(bb->instructions.size() > 0);
    Instruction* insn = bb->instructions[0];
    SANDBOX_ASSERT(insn == first_insn ||
                   branch_targets.find(insn) != branch_targets.end());
    for (Instructions::const_iterator insn_iter = bb->instructions.begin();;) {
      insn = *insn_iter;
      if (++insn_iter != bb->instructions.end()) {
        SANDBOX_ASSERT(BPF_CLASS(insn->code) != BPF_JMP);
        SANDBOX_ASSERT(BPF_CLASS(insn->code) != BPF_RET);
      } else {
        SANDBOX_ASSERT(BPF_CLASS(insn->code) == BPF_JMP ||
                       BPF_CLASS(insn->code) == BPF_RET ||
                       branch_targets.find(insn->next) != branch_targets.end());
        break;
      }
      SANDBOX_ASSERT(branch_targets.find(*insn_iter) == branch_targets.end());
    }
  }
}

SANDBOX_TEST(CodeGen, CutGraphIntoBasicBlocks) {
  ForAllPrograms(CutGraphIntoBasicBlocks);
}

void MergeTails(CodeGenUnittestHelper* codegen, Instruction* prg, int flags) {
  BranchTargets branch_targets;
  codegen->FindBranchTargets(*prg, &branch_targets);
  TargetsToBlocks all_blocks;
  BasicBlock* first_block =
      codegen->CutGraphIntoBasicBlocks(prg, branch_targets, &all_blocks);

  
  
  
  
  
  std::string graph[2];
  std::string edges[2];

  
  
  for (int i = 0;;) {
    
    std::vector<BasicBlock*> stack;
    for (BasicBlock* bb = first_block;;) {
      
      
      
      
      for (Instructions::const_iterator iter = bb->instructions.begin();
           iter != bb->instructions.end();
           ++iter) {
        graph[i].append(reinterpret_cast<char*>(&(*iter)->code),
                        sizeof((*iter)->code));
        if (BPF_CLASS((*iter)->code) != BPF_JMP ||
            BPF_OP((*iter)->code) != BPF_JA) {
          graph[i].append(reinterpret_cast<char*>(&(*iter)->k),
                          sizeof((*iter)->k));
        }
      }

      
      
      edges[i].append(reinterpret_cast<char*>(&bb), sizeof(bb));

      
      
      
      Instruction* insn = bb->instructions.back();
      if (BPF_CLASS(insn->code) == BPF_JMP) {
        
        
        
        if (BPF_OP(insn->code) != BPF_JA) {
          stack.push_back(all_blocks[insn->jf_ptr]);
        }
        bb = all_blocks[insn->jt_ptr];
      } else if (BPF_CLASS(insn->code) == BPF_RET) {
        
        if (stack.empty()) {
          break;
        }
        bb = stack.back();
        stack.pop_back();
      } else {
        
        bb = all_blocks[insn->next];
      }
    }

    
    if (++i > 1) {
      break;
    }
    codegen->MergeTails(&all_blocks);
  }
  SANDBOX_ASSERT(graph[0] == graph[1]);
  if (flags & HAS_MERGEABLE_TAILS) {
    SANDBOX_ASSERT(edges[0] != edges[1]);
  } else {
    SANDBOX_ASSERT(edges[0] == edges[1]);
  }
}

SANDBOX_TEST(CodeGen, MergeTails) {
  ForAllPrograms(MergeTails);
}

void CompileAndCompare(CodeGenUnittestHelper* codegen, Instruction* prg, int) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  std::string source;
  Instructions source_stack;
  for (const Instruction* insn = prg, *next; insn; insn = next) {
    if (BPF_CLASS(insn->code) == BPF_JMP) {
      if (BPF_OP(insn->code) == BPF_JA) {
        
        next = insn->jt_ptr;
        continue;
      } else {
        source_stack.push_back(insn->jf_ptr);
        next = insn->jt_ptr;
      }
    } else if (BPF_CLASS(insn->code) == BPF_RET) {
      if (source_stack.empty()) {
        next = NULL;
      } else {
        next = source_stack.back();
        source_stack.pop_back();
      }
    } else {
      next = insn->next;
    }
    
    
    
    source.append(reinterpret_cast<const char*>(&insn->code),
                  sizeof(insn->code));
    source.append(reinterpret_cast<const char*>(&insn->k), sizeof(insn->k));
  }

  
  SandboxUnittestHelper::Program bpf;
  codegen->Compile(prg, &bpf);

  
  std::string assembly;
  std::vector<int> assembly_stack;
  for (int idx = 0; idx >= 0;) {
    SANDBOX_ASSERT(idx < (int)bpf.size());
    struct sock_filter& insn = bpf[idx];
    if (BPF_CLASS(insn.code) == BPF_JMP) {
      if (BPF_OP(insn.code) == BPF_JA) {
        
        idx += insn.k + 1;
        continue;
      } else {
        assembly_stack.push_back(idx + insn.jf + 1);
        idx += insn.jt + 1;
      }
    } else if (BPF_CLASS(insn.code) == BPF_RET) {
      if (assembly_stack.empty()) {
        idx = -1;
      } else {
        idx = assembly_stack.back();
        assembly_stack.pop_back();
      }
    } else {
      ++idx;
    }
    
    assembly.append(reinterpret_cast<char*>(&insn.code), sizeof(insn.code));
    assembly.append(reinterpret_cast<char*>(&insn.k), sizeof(insn.k));
  }
  SANDBOX_ASSERT(source == assembly);
}

SANDBOX_TEST(CodeGen, All) {
  ForAllPrograms(CompileAndCompare);
}

}  
