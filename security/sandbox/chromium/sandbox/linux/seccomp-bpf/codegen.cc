



#include <stdio.h>

#include "base/logging.h"
#include "sandbox/linux/seccomp-bpf/codegen.h"

namespace {


void TraverseRecursively(std::set<sandbox::Instruction*>* visited,
                         sandbox::Instruction* instruction) {
  if (visited->find(instruction) == visited->end()) {
    visited->insert(instruction);
    switch (BPF_CLASS(instruction->code)) {
      case BPF_JMP:
        if (BPF_OP(instruction->code) != BPF_JA) {
          TraverseRecursively(visited, instruction->jf_ptr);
        }
        TraverseRecursively(visited, instruction->jt_ptr);
        break;
      case BPF_RET:
        break;
      default:
        TraverseRecursively(visited, instruction->next);
        break;
    }
  }
}

}  

namespace sandbox {

CodeGen::CodeGen() : compiled_(false) {}

CodeGen::~CodeGen() {
  for (Instructions::iterator iter = instructions_.begin();
       iter != instructions_.end();
       ++iter) {
    delete *iter;
  }
  for (BasicBlocks::iterator iter = basic_blocks_.begin();
       iter != basic_blocks_.end();
       ++iter) {
    delete *iter;
  }
}

void CodeGen::PrintProgram(const SandboxBPF::Program& program) {
  for (SandboxBPF::Program::const_iterator iter = program.begin();
       iter != program.end();
       ++iter) {
    int ip = (int)(iter - program.begin());
    fprintf(stderr, "%3d) ", ip);
    switch (BPF_CLASS(iter->code)) {
      case BPF_LD:
        if (iter->code == BPF_LD + BPF_W + BPF_ABS) {
          fprintf(stderr, "LOAD %d  // ", (int)iter->k);
          if (iter->k == offsetof(struct arch_seccomp_data, nr)) {
            fprintf(stderr, "System call number\n");
          } else if (iter->k == offsetof(struct arch_seccomp_data, arch)) {
            fprintf(stderr, "Architecture\n");
          } else if (iter->k ==
                     offsetof(struct arch_seccomp_data, instruction_pointer)) {
            fprintf(stderr, "Instruction pointer (LSB)\n");
          } else if (iter->k ==
                     offsetof(struct arch_seccomp_data, instruction_pointer) +
                         4) {
            fprintf(stderr, "Instruction pointer (MSB)\n");
          } else if (iter->k >= offsetof(struct arch_seccomp_data, args) &&
                     iter->k < offsetof(struct arch_seccomp_data, args) + 48 &&
                     (iter->k - offsetof(struct arch_seccomp_data, args)) % 4 ==
                         0) {
            fprintf(
                stderr,
                "Argument %d (%cSB)\n",
                (int)(iter->k - offsetof(struct arch_seccomp_data, args)) / 8,
                (iter->k - offsetof(struct arch_seccomp_data, args)) % 8 ? 'M'
                                                                         : 'L');
          } else {
            fprintf(stderr, "???\n");
          }
        } else {
          fprintf(stderr, "LOAD ???\n");
        }
        break;
      case BPF_JMP:
        if (BPF_OP(iter->code) == BPF_JA) {
          fprintf(stderr, "JMP %d\n", ip + iter->k + 1);
        } else {
          fprintf(stderr, "if A %s 0x%x; then JMP %d else JMP %d\n",
              BPF_OP(iter->code) == BPF_JSET ? "&" :
              BPF_OP(iter->code) == BPF_JEQ ? "==" :
              BPF_OP(iter->code) == BPF_JGE ? ">=" :
              BPF_OP(iter->code) == BPF_JGT ? ">"  : "???",
              (int)iter->k,
              ip + iter->jt + 1, ip + iter->jf + 1);
        }
        break;
      case BPF_RET:
        fprintf(stderr, "RET 0x%x  // ", iter->k);
        if ((iter->k & SECCOMP_RET_ACTION) == SECCOMP_RET_TRAP) {
          fprintf(stderr, "Trap #%d\n", iter->k & SECCOMP_RET_DATA);
        } else if ((iter->k & SECCOMP_RET_ACTION) == SECCOMP_RET_ERRNO) {
          fprintf(stderr, "errno = %d\n", iter->k & SECCOMP_RET_DATA);
        } else if (iter->k == SECCOMP_RET_ALLOW) {
          fprintf(stderr, "Allowed\n");
        } else {
          fprintf(stderr, "???\n");
        }
        break;
      case BPF_ALU:
        fprintf(stderr, BPF_OP(iter->code) == BPF_NEG
            ? "A := -A\n" : "A := A %s 0x%x\n",
            BPF_OP(iter->code) == BPF_ADD ? "+"  :
            BPF_OP(iter->code) == BPF_SUB ? "-"  :
            BPF_OP(iter->code) == BPF_MUL ? "*"  :
            BPF_OP(iter->code) == BPF_DIV ? "/"  :
            BPF_OP(iter->code) == BPF_MOD ? "%"  :
            BPF_OP(iter->code) == BPF_OR  ? "|"  :
            BPF_OP(iter->code) == BPF_XOR ? "^"  :
            BPF_OP(iter->code) == BPF_AND ? "&"  :
            BPF_OP(iter->code) == BPF_LSH ? "<<" :
            BPF_OP(iter->code) == BPF_RSH ? ">>" : "???",
            (int)iter->k);
        break;
      default:
        fprintf(stderr, "???\n");
        break;
    }
  }
  return;
}

Instruction* CodeGen::MakeInstruction(uint16_t code,
                                      uint32_t k,
                                      Instruction* next) {
  
  
  
  
  if (BPF_CLASS(code) == BPF_JMP && BPF_OP(code) != BPF_JA) {
    SANDBOX_DIE(
        "Must provide both \"true\" and \"false\" branch "
        "for a BPF_JMP");
  }
  if (next && BPF_CLASS(code) == BPF_RET) {
    SANDBOX_DIE("Cannot append instructions after a return statement");
  }
  if (BPF_CLASS(code) == BPF_JMP) {
    
    Instruction* insn = new Instruction(code, 0, next, NULL);
    instructions_.push_back(insn);
    return insn;
  } else {
    
    Instruction* insn = new Instruction(code, k, next);
    instructions_.push_back(insn);
    return insn;
  }
}

Instruction* CodeGen::MakeInstruction(uint16_t code, const ErrorCode& err) {
  if (BPF_CLASS(code) != BPF_RET) {
    SANDBOX_DIE("ErrorCodes can only be used in return expressions");
  }
  if (err.error_type_ != ErrorCode::ET_SIMPLE &&
      err.error_type_ != ErrorCode::ET_TRAP) {
    SANDBOX_DIE("ErrorCode is not suitable for returning from a BPF program");
  }
  return MakeInstruction(code, err.err_);
}

Instruction* CodeGen::MakeInstruction(uint16_t code,
                                      uint32_t k,
                                      Instruction* jt,
                                      Instruction* jf) {
  
  
  if (BPF_CLASS(code) != BPF_JMP || BPF_OP(code) == BPF_JA) {
    SANDBOX_DIE("Expected a BPF_JMP instruction");
  }
  if (!jt && !jf) {
    
    
    SANDBOX_DIE("Branches must jump to a valid instruction");
  }
  Instruction* insn = new Instruction(code, k, jt, jf);
  instructions_.push_back(insn);
  return insn;
}

void CodeGen::JoinInstructions(Instruction* head, Instruction* tail) {
  
  
  
  if (BPF_CLASS(head->code) == BPF_JMP) {
    if (BPF_OP(head->code) == BPF_JA) {
      if (head->jt_ptr) {
        SANDBOX_DIE("Cannot append instructions in the middle of a sequence");
      }
      head->jt_ptr = tail;
    } else {
      if (!head->jt_ptr && head->jf_ptr) {
        head->jt_ptr = tail;
      } else if (!head->jf_ptr && head->jt_ptr) {
        head->jf_ptr = tail;
      } else {
        SANDBOX_DIE("Cannot append instructions after a jump");
      }
    }
  } else if (BPF_CLASS(head->code) == BPF_RET) {
    SANDBOX_DIE("Cannot append instructions after a return statement");
  } else if (head->next) {
    SANDBOX_DIE("Cannot append instructions in the middle of a sequence");
  } else {
    head->next = tail;
  }
  return;
}

void CodeGen::Traverse(Instruction* instruction,
                       void (*fnc)(Instruction*, void*),
                       void* aux) {
  std::set<Instruction*> visited;
  TraverseRecursively(&visited, instruction);
  for (std::set<Instruction*>::const_iterator iter = visited.begin();
       iter != visited.end();
       ++iter) {
    fnc(*iter, aux);
  }
}

void CodeGen::FindBranchTargets(const Instruction& instructions,
                                BranchTargets* branch_targets) {
  
  
  
  
  
  std::set<const Instruction*> seen_instructions;
  Instructions stack;
  for (const Instruction* insn = &instructions; insn;) {
    seen_instructions.insert(insn);
    if (BPF_CLASS(insn->code) == BPF_JMP) {
      
      
      ++(*branch_targets)[insn->jt_ptr];
      if (BPF_OP(insn->code) != BPF_JA) {
        ++(*branch_targets)[insn->jf_ptr];
        stack.push_back(const_cast<Instruction*>(insn));
      }
      
      if (seen_instructions.find(insn->jt_ptr) == seen_instructions.end()) {
        
        
        
        insn = insn->jt_ptr;
        continue;
      } else {
        
        insn = NULL;
      }
    } else {
      
      
      
      if (!insn->next != (BPF_CLASS(insn->code) == BPF_RET)) {
        SANDBOX_DIE(
            "Internal compiler error; return instruction must be at "
            "the end of the BPF program");
      }
      if (seen_instructions.find(insn->next) == seen_instructions.end()) {
        insn = insn->next;
      } else {
        
        
        insn = NULL;
      }
    }
    while (!insn && !stack.empty()) {
      
      
      
      
      insn = stack.back();
      stack.pop_back();
      if (seen_instructions.find(insn->jf_ptr) == seen_instructions.end()) {
        
        
        insn = insn->jf_ptr;
      } else {
        
        
        if (seen_instructions.find(insn->jt_ptr) == seen_instructions.end()) {
          SANDBOX_DIE(
              "Internal compiler error; cannot find all "
              "branch targets");
        }
        insn = NULL;
      }
    }
  }
  return;
}

BasicBlock* CodeGen::MakeBasicBlock(Instruction* head, Instruction* tail) {
  
  
  BasicBlock* bb = new BasicBlock;
  for (;; head = head->next) {
    bb->instructions.push_back(head);
    if (head == tail) {
      break;
    }
    if (BPF_CLASS(head->code) == BPF_JMP) {
      SANDBOX_DIE("Found a jump inside of a basic block");
    }
  }
  basic_blocks_.push_back(bb);
  return bb;
}

void CodeGen::AddBasicBlock(Instruction* head,
                            Instruction* tail,
                            const BranchTargets& branch_targets,
                            TargetsToBlocks* basic_blocks,
                            BasicBlock** firstBlock) {
  
  
  BranchTargets::const_iterator iter = branch_targets.find(head);
  if ((iter == branch_targets.end()) != !*firstBlock ||
      !*firstBlock != basic_blocks->empty()) {
    SANDBOX_DIE(
        "Only the very first basic block should have no "
        "incoming jumps");
  }
  BasicBlock* bb = MakeBasicBlock(head, tail);
  if (!*firstBlock) {
    *firstBlock = bb;
  }
  (*basic_blocks)[head] = bb;
  return;
}

BasicBlock* CodeGen::CutGraphIntoBasicBlocks(
    Instruction* instructions,
    const BranchTargets& branch_targets,
    TargetsToBlocks* basic_blocks) {
  
  
  
  
  BasicBlock* first_block = NULL;
  std::set<const Instruction*> seen_instructions;
  Instructions stack;
  Instruction* tail = NULL;
  Instruction* head = instructions;
  for (Instruction* insn = head; insn;) {
    if (seen_instructions.find(insn) != seen_instructions.end()) {
      
      
      SANDBOX_DIE("Internal compiler error; cannot compute basic blocks");
    }
    seen_instructions.insert(insn);
    if (tail && branch_targets.find(insn) != branch_targets.end()) {
      
      
      AddBasicBlock(head, tail, branch_targets, basic_blocks, &first_block);
      head = insn;
    }
    if (BPF_CLASS(insn->code) == BPF_JMP) {
      
      
      
      AddBasicBlock(head, insn, branch_targets, basic_blocks, &first_block);
      if (BPF_OP(insn->code) != BPF_JA) {
        stack.push_back(insn->jf_ptr);
      }
      insn = insn->jt_ptr;

      
      
      
      while (seen_instructions.find(insn) != seen_instructions.end()) {
      backtracking:
        if (stack.empty()) {
          
          return first_block;
        } else {
          
          insn = stack.back();
          stack.pop_back();
        }
      }
      
      tail = NULL;
      head = insn;
    } else {
      
      
      tail = insn;
      insn = insn->next;
      if (!insn) {
        
        
        AddBasicBlock(head, tail, branch_targets, basic_blocks, &first_block);
        goto backtracking;
      }
    }
  }
  return first_block;
}






static int PointerCompare(const BasicBlock* block1,
                          const BasicBlock* block2,
                          const TargetsToBlocks& blocks) {
  
  
  
  if (block1 == block2) {
    return 0;
  }

  
  const Instructions& insns1 = block1->instructions;
  const Instructions& insns2 = block2->instructions;
  
  CHECK(!insns1.empty());
  CHECK(!insns2.empty());

  Instructions::const_iterator iter1 = insns1.begin();
  Instructions::const_iterator iter2 = insns2.begin();
  for (;; ++iter1, ++iter2) {
    
    
    
    if (iter1 == insns1.end()) {
      if (iter2 == insns2.end()) {
        
        
        
        
        Instruction* const insns1_last = insns1.back();
        Instruction* const insns2_last = insns2.back();
        if (BPF_CLASS(insns1_last->code) != BPF_JMP &&
            BPF_CLASS(insns1_last->code) != BPF_RET) {
          
          CHECK(insns1_last->next);
          CHECK(insns2_last->next);
          return PointerCompare(blocks.find(insns1_last->next)->second,
                                blocks.find(insns2_last->next)->second,
                                blocks);
        } else {
          return 0;
        }
      }
      return -1;
    } else if (iter2 == insns2.end()) {
      return 1;
    }

    
    const Instruction& insn1 = **iter1;
    const Instruction& insn2 = **iter2;
    if (insn1.code == insn2.code) {
      if (insn1.k == insn2.k) {
        
        
        if (BPF_CLASS(insn1.code) == BPF_JMP) {
          if (BPF_OP(insn1.code) != BPF_JA) {
            
            
            
            
            
            
            int c = PointerCompare(blocks.find(insn1.jt_ptr)->second,
                                   blocks.find(insn2.jt_ptr)->second,
                                   blocks);
            if (c == 0) {
              c = PointerCompare(blocks.find(insn1.jf_ptr)->second,
                                 blocks.find(insn2.jf_ptr)->second,
                                 blocks);
              if (c == 0) {
                continue;
              } else {
                return c;
              }
            } else {
              return c;
            }
          } else {
            int c = PointerCompare(blocks.find(insn1.jt_ptr)->second,
                                   blocks.find(insn2.jt_ptr)->second,
                                   blocks);
            if (c == 0) {
              continue;
            } else {
              return c;
            }
          }
        } else {
          continue;
        }
      } else {
        return insn1.k - insn2.k;
      }
    } else {
      return insn1.code - insn2.code;
    }
  }
}

void CodeGen::MergeTails(TargetsToBlocks* blocks) {
  
  
  
  
  
  
  
  
  
  
  
  
  BasicBlock::Less<TargetsToBlocks> less(*blocks, PointerCompare);
  typedef std::set<BasicBlock*, BasicBlock::Less<TargetsToBlocks> > Set;
  Set seen_basic_blocks(less);
  for (TargetsToBlocks::iterator iter = blocks->begin(); iter != blocks->end();
       ++iter) {
    BasicBlock* bb = iter->second;
    Set::const_iterator entry = seen_basic_blocks.find(bb);
    if (entry == seen_basic_blocks.end()) {
      
      
      
      seen_basic_blocks.insert(bb);
    } else {
      
      
      
      iter->second = *entry;
    }
  }
}

void CodeGen::ComputeIncomingBranches(BasicBlock* block,
                                      const TargetsToBlocks& targets_to_blocks,
                                      IncomingBranches* incoming_branches) {
  
  
  
  
  if (++(*incoming_branches)[block] == 1) {
    Instruction* last_insn = block->instructions.back();
    if (BPF_CLASS(last_insn->code) == BPF_JMP) {
      ComputeIncomingBranches(targets_to_blocks.find(last_insn->jt_ptr)->second,
                              targets_to_blocks,
                              incoming_branches);
      if (BPF_OP(last_insn->code) != BPF_JA) {
        ComputeIncomingBranches(
            targets_to_blocks.find(last_insn->jf_ptr)->second,
            targets_to_blocks,
            incoming_branches);
      }
    } else if (BPF_CLASS(last_insn->code) != BPF_RET) {
      ComputeIncomingBranches(targets_to_blocks.find(last_insn->next)->second,
                              targets_to_blocks,
                              incoming_branches);
    }
  }
}

void CodeGen::TopoSortBasicBlocks(BasicBlock* first_block,
                                  const TargetsToBlocks& blocks,
                                  BasicBlocks* basic_blocks) {
  
  
  
  
  
  
  
  
  
  
  IncomingBranches unordered_blocks;
  ComputeIncomingBranches(first_block, blocks, &unordered_blocks);

  std::set<BasicBlock*> heads;
  for (;;) {
    
    basic_blocks->push_back(first_block);

    
    
    
    Instruction* last_insn = first_block->instructions.back();
    if (BPF_CLASS(last_insn->code) == BPF_JMP) {
      
      
      TargetsToBlocks::const_iterator iter;
      if (BPF_OP(last_insn->code) != BPF_JA) {
        iter = blocks.find(last_insn->jf_ptr);
        if (!--unordered_blocks[iter->second]) {
          heads.insert(iter->second);
        }
      }
      iter = blocks.find(last_insn->jt_ptr);
      if (!--unordered_blocks[iter->second]) {
        first_block = iter->second;
        continue;
      }
    } else if (BPF_CLASS(last_insn->code) != BPF_RET) {
      
      
      TargetsToBlocks::const_iterator iter;
      iter = blocks.find(last_insn->next);
      if (!--unordered_blocks[iter->second]) {
        first_block = iter->second;
        continue;
      } else {
        
        
        
        Instruction* ja = MakeInstruction(BPF_JMP + BPF_JA, 0, last_insn->next);
        first_block->instructions.push_back(ja);
        last_insn->next = ja;
      }
    }
    if (heads.empty()) {
      if (unordered_blocks.size() != basic_blocks->size()) {
        SANDBOX_DIE("Internal compiler error; cyclic graph detected");
      }
      return;
    }
    
    
    first_block = *heads.begin();
    heads.erase(heads.begin());
  }
}

void CodeGen::ComputeRelativeJumps(BasicBlocks* basic_blocks,
                                   const TargetsToBlocks& targets_to_blocks) {
  
  
  
  int offset = 0;

  
  
  
  BasicBlock* bb = NULL;
  BasicBlock* last_bb = NULL;
  for (BasicBlocks::reverse_iterator iter = basic_blocks->rbegin();
       iter != basic_blocks->rend();
       ++iter) {
    last_bb = bb;
    bb = *iter;
    Instruction* insn = bb->instructions.back();
    if (BPF_CLASS(insn->code) == BPF_JMP) {
      
      
      if (BPF_OP(insn->code) == BPF_JA) {
        
        
        int jmp = offset - targets_to_blocks.find(insn->jt_ptr)->second->offset;
        insn->k = jmp;
        insn->jt = insn->jf = 0;
      } else {
        
        
        int jt = offset - targets_to_blocks.find(insn->jt_ptr)->second->offset;
        int jf = offset - targets_to_blocks.find(insn->jf_ptr)->second->offset;

        
        
        
        Instructions::size_type jmp = bb->instructions.size();
        if (jt > 255 || (jt == 255 && jf > 255)) {
          Instruction* ja = MakeInstruction(BPF_JMP + BPF_JA, 0, insn->jt_ptr);
          bb->instructions.push_back(ja);
          ja->k = jt;
          ja->jt = ja->jf = 0;

          
          
          jt = 0;
          ++jf;
        }
        if (jf > 255) {
          Instruction* ja = MakeInstruction(BPF_JMP + BPF_JA, 0, insn->jf_ptr);
          bb->instructions.insert(bb->instructions.begin() + jmp, ja);
          ja->k = jf;
          ja->jt = ja->jf = 0;

          
          
          ++jt;
          jf = 0;
        }

        
        
        
        insn->jt = jt;
        insn->jf = jf;
      }
    } else if (BPF_CLASS(insn->code) != BPF_RET &&
               targets_to_blocks.find(insn->next)->second != last_bb) {
      SANDBOX_DIE("Internal compiler error; invalid basic block encountered");
    }

    
    offset += bb->instructions.size();
    bb->offset = offset;
  }
  return;
}

void CodeGen::ConcatenateBasicBlocks(const BasicBlocks& basic_blocks,
                                     SandboxBPF::Program* program) {
  
  
  
  program->clear();
  for (BasicBlocks::const_iterator bb_iter = basic_blocks.begin();
       bb_iter != basic_blocks.end();
       ++bb_iter) {
    const BasicBlock& bb = **bb_iter;
    for (Instructions::const_iterator insn_iter = bb.instructions.begin();
         insn_iter != bb.instructions.end();
         ++insn_iter) {
      const Instruction& insn = **insn_iter;
      program->push_back(
          (struct sock_filter) {insn.code, insn.jt, insn.jf, insn.k});
    }
  }
  return;
}

void CodeGen::Compile(Instruction* instructions, SandboxBPF::Program* program) {
  if (compiled_) {
    SANDBOX_DIE(
        "Cannot call Compile() multiple times. Create a new code "
        "generator instead");
  }
  compiled_ = true;

  BranchTargets branch_targets;
  FindBranchTargets(*instructions, &branch_targets);
  TargetsToBlocks all_blocks;
  BasicBlock* first_block =
      CutGraphIntoBasicBlocks(instructions, branch_targets, &all_blocks);
  MergeTails(&all_blocks);
  BasicBlocks basic_blocks;
  TopoSortBasicBlocks(first_block, all_blocks, &basic_blocks);
  ComputeRelativeJumps(&basic_blocks, all_blocks);
  ConcatenateBasicBlocks(basic_blocks, program);
  return;
}

}  
