



#include "sandbox/linux/seccomp-bpf/codegen.h"

#include <linux/filter.h>

#include <set>
#include <string>
#include <vector>

#include "sandbox/linux/seccomp-bpf/basicblock.h"
#include "sandbox/linux/seccomp-bpf/errorcode.h"
#include "sandbox/linux/seccomp-bpf/instruction.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {



class CodeGenUnittestHelper : public CodeGen {
 public:
  using CodeGen::CutGraphIntoBasicBlocks;
  using CodeGen::FindBranchTargets;
  using CodeGen::MergeTails;
};

namespace {

enum {
  NO_FLAGS = 0x0000,
  HAS_MERGEABLE_TAILS = 0x0001,
};

using ProgramTestFunc = void (*)(CodeGenUnittestHelper* gen,
                                 Instruction* head,
                                 int flags);

class ProgramTest : public ::testing::TestWithParam<ProgramTestFunc> {
 protected:
  ProgramTest() : gen_() {}

  
  
  void RunTest(Instruction* head, int flags) { GetParam()(&gen_, head, flags); }

  Instruction* MakeInstruction(uint16_t code,
                               uint32_t k,
                               Instruction* next = nullptr) {
    Instruction* ret = gen_.MakeInstruction(code, k, next);
    EXPECT_NE(nullptr, ret);
    EXPECT_EQ(code, ret->code);
    EXPECT_EQ(k, ret->k);
    if (code == BPF_JMP + BPF_JA) {
      
      EXPECT_EQ(nullptr, ret->next);
      EXPECT_EQ(next, ret->jt_ptr);
    } else {
      EXPECT_EQ(next, ret->next);
      EXPECT_EQ(nullptr, ret->jt_ptr);
    }
    EXPECT_EQ(nullptr, ret->jf_ptr);
    return ret;
  }

  Instruction* MakeInstruction(uint16_t code,
                               uint32_t k,
                               Instruction* jt,
                               Instruction* jf) {
    Instruction* ret = gen_.MakeInstruction(code, k, jt, jf);
    EXPECT_NE(nullptr, ret);
    EXPECT_EQ(code, ret->code);
    EXPECT_EQ(k, ret->k);
    EXPECT_EQ(nullptr, ret->next);
    EXPECT_EQ(jt, ret->jt_ptr);
    EXPECT_EQ(jf, ret->jf_ptr);
    return ret;
  }

 private:
  CodeGenUnittestHelper gen_;
};

TEST_P(ProgramTest, OneInstruction) {
  
  
  Instruction* head = MakeInstruction(BPF_RET + BPF_K, 0);
  RunTest(head, NO_FLAGS);
}

TEST_P(ProgramTest, SimpleBranch) {
  
  
  
  
  Instruction* head = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K,
                                      42,
                                      MakeInstruction(BPF_RET + BPF_K, 1),
                                      MakeInstruction(BPF_RET + BPF_K, 0));
  RunTest(head, NO_FLAGS);
}

TEST_P(ProgramTest, AtypicalBranch) {
  
  
  

  Instruction* ret = MakeInstruction(BPF_RET + BPF_K, 0);
  Instruction* head = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 42, ret, ret);

  
  
  
  RunTest(head, NO_FLAGS);
}

TEST_P(ProgramTest, Complex) {
  
  
  
  
  
  
  
  
  
  Instruction* insn0 = MakeInstruction(BPF_RET + BPF_K, 42);
  Instruction* insn1 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 42, insn0);
  Instruction* insn2 = MakeInstruction(BPF_JMP + BPF_JA, 0, insn1);

  
  
  Instruction* insn3 = MakeInstruction(
      BPF_LD + BPF_W + BPF_ABS, 42, MakeInstruction(BPF_RET + BPF_K, 42));

  Instruction* insn4 =
      MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 42, insn2, insn3);
  Instruction* insn5 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 23, insn4);

  
  
  
  
  
  
  Instruction* insn6 =
      MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 42, insn5, insn4);

  RunTest(insn6, HAS_MERGEABLE_TAILS);
}

TEST_P(ProgramTest, ConfusingTails) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  Instruction* i7 = MakeInstruction(BPF_RET + BPF_K, 1);
  Instruction* i6 = MakeInstruction(BPF_RET + BPF_K, 0);
  Instruction* i5 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i6, i7);
  Instruction* i4 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i5);
  Instruction* i3 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 2, i4, i5);
  Instruction* i2 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i3);
  Instruction* i1 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i2, i3);
  Instruction* i0 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 1, i1);

  RunTest(i0, NO_FLAGS);
}

TEST_P(ProgramTest, ConfusingTailsBasic) {
  
  
  
  
  
  
  
  
  
  

  Instruction* i5 = MakeInstruction(BPF_RET + BPF_K, 1);
  Instruction* i4 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i5);
  Instruction* i3 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 2, i4, i5);
  Instruction* i2 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 0, i3);
  Instruction* i1 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i2, i3);
  Instruction* i0 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 1, i1);

  RunTest(i0, NO_FLAGS);
}

TEST_P(ProgramTest, ConfusingTailsMergeable) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  Instruction* i7 = MakeInstruction(BPF_RET + BPF_K, 1);
  Instruction* i6 = MakeInstruction(BPF_RET + BPF_K, 0);
  Instruction* i5 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i6, i7);
  Instruction* i4 = MakeInstruction(BPF_RET + BPF_K, 42);
  Instruction* i3 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 2, i4, i5);
  Instruction* i2 = MakeInstruction(BPF_RET + BPF_K, 42);
  Instruction* i1 = MakeInstruction(BPF_JMP + BPF_JEQ + BPF_K, 1, i2, i3);
  Instruction* i0 = MakeInstruction(BPF_LD + BPF_W + BPF_ABS, 1, i1);

  RunTest(i0, HAS_MERGEABLE_TAILS);
}

void MakeInstruction(CodeGenUnittestHelper* codegen,
                     Instruction* program, int) {
  
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
      ASSERT_TRUE(insn->jt_ptr != NULL);
      ASSERT_TRUE(branch_targets.find(insn->jt_ptr) != end);
      if (BPF_OP(insn->code) != BPF_JA) {
        target_instructions.insert(insn->jf_ptr);
        ASSERT_TRUE(insn->jf_ptr != NULL);
        ASSERT_TRUE(branch_targets.find(insn->jf_ptr) != end);
        stack.push_back(insn->jf_ptr);
      }
      insn = insn->jt_ptr;
    } else if (BPF_CLASS(insn->code) == BPF_RET) {
      ASSERT_TRUE(insn->next == NULL);
      if (stack.empty()) {
        break;
      }
      insn = stack.back();
      stack.pop_back();
    } else {
      ASSERT_TRUE(insn->next != NULL);
      insn = insn->next;
    }
  }
  ASSERT_TRUE(target_instructions.size() == branch_targets.size());

  
  
  
  
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
    ASSERT_TRUE(branch_targets.find(*iter) == end);
  }
}

void CutGraphIntoBasicBlocks(CodeGenUnittestHelper* codegen,
                             Instruction* prg,
                             int) {
  BranchTargets branch_targets;
  codegen->FindBranchTargets(*prg, &branch_targets);
  TargetsToBlocks all_blocks;
  BasicBlock* first_block =
      codegen->CutGraphIntoBasicBlocks(prg, branch_targets, &all_blocks);
  ASSERT_TRUE(first_block != NULL);
  ASSERT_TRUE(first_block->instructions.size() > 0);
  Instruction* first_insn = first_block->instructions[0];

  
  
  
  
  for (TargetsToBlocks::const_iterator bb_iter = all_blocks.begin();
       bb_iter != all_blocks.end();
       ++bb_iter) {
    BasicBlock* bb = bb_iter->second;
    ASSERT_TRUE(bb != NULL);
    ASSERT_TRUE(bb->instructions.size() > 0);
    Instruction* insn = bb->instructions[0];
    ASSERT_TRUE(insn == first_insn ||
                branch_targets.find(insn) != branch_targets.end());
    for (Instructions::const_iterator insn_iter = bb->instructions.begin();;) {
      insn = *insn_iter;
      if (++insn_iter != bb->instructions.end()) {
        ASSERT_TRUE(BPF_CLASS(insn->code) != BPF_JMP);
        ASSERT_TRUE(BPF_CLASS(insn->code) != BPF_RET);
      } else {
        ASSERT_TRUE(BPF_CLASS(insn->code) == BPF_JMP ||
                    BPF_CLASS(insn->code) == BPF_RET ||
                    branch_targets.find(insn->next) != branch_targets.end());
        break;
      }
      ASSERT_TRUE(branch_targets.find(*insn_iter) == branch_targets.end());
    }
  }
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
  ASSERT_TRUE(graph[0] == graph[1]);
  if (flags & HAS_MERGEABLE_TAILS) {
    ASSERT_TRUE(edges[0] != edges[1]);
  } else {
    ASSERT_TRUE(edges[0] == edges[1]);
  }
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

  
  CodeGen::Program bpf;
  codegen->Compile(prg, &bpf);

  
  std::string assembly;
  std::vector<int> assembly_stack;
  for (int idx = 0; idx >= 0;) {
    ASSERT_TRUE(idx < (int)bpf.size());
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
  ASSERT_TRUE(source == assembly);
}

const ProgramTestFunc kProgramTestFuncs[] = {
    MakeInstruction,
    FindBranchTargets,
    CutGraphIntoBasicBlocks,
    MergeTails,
    CompileAndCompare,
};

INSTANTIATE_TEST_CASE_P(CodeGen,
                        ProgramTest,
                        ::testing::ValuesIn(kProgramTestFuncs));

}  

}  
