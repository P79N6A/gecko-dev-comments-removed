






































#include <unistd.h>

#include "breakpad_googletest_includes.h"
#include "processor/disassembler_x86.h"
#include "third_party/libdisasm/libdis.h"

namespace {

using google_breakpad::DisassemblerX86;

unsigned char just_return[] = "\xc3";  

unsigned char invalid_instruction[] = "\x00";  

unsigned char read_eax_jmp_eax[] =
    "\x8b\x18"                  
    "\x33\xc9"                  
    "\xff\x20"                  
    "\xc3";                     

unsigned char write_eax_arg_to_call[] =
    "\x89\xa8\x00\x02\x00\x00"  
    "\xc1\xeb\x02"              
    "\x50"                      
    "\xe8\xd1\x24\x77\x88"      
    "\xc3";                     

unsigned char read_edi_stosb[] =
    "\x8b\x07"                  
    "\x8b\xc8"                  
    "\xf3\xaa"                  
    "\xc3";                     

unsigned char read_clobber_write[] =
    "\x03\x18"                  
    "\x8b\xc1"                  
    "\x89\x10"                  
    "\xc3";                     

unsigned char read_xchg_write[] =
    "\x03\x18"                  
    "\x91"                      
    "\x89\x18"                  
    "\x89\x11"                  
    "\xc3";                     

unsigned char read_cmp[] =
    "\x03\x18"                  
    "\x83\xf8\x00"              
    "\x74\x04"                  
    "\xc3";                     

TEST(DisassemblerX86Test, SimpleReturnInstruction) {
  DisassemblerX86 dis(just_return, sizeof(just_return)-1, 0);
  EXPECT_EQ(1U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_TRUE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_controlflow, dis.currentInstructionGroup());
  const libdis::x86_insn_t* instruction = dis.currentInstruction();
  EXPECT_EQ(libdis::insn_controlflow, instruction->group);
  EXPECT_EQ(libdis::insn_return, instruction->type);
  EXPECT_EQ(0U, dis.NextInstruction());
  EXPECT_FALSE(dis.currentInstructionValid());
  EXPECT_EQ(NULL, dis.currentInstruction());
}

TEST(DisassemblerX86Test, SimpleInvalidInstruction) {
  DisassemblerX86 dis(invalid_instruction, sizeof(invalid_instruction)-1, 0);
  EXPECT_EQ(0U, dis.NextInstruction());
  EXPECT_FALSE(dis.currentInstructionValid());
}

TEST(DisassemblerX86Test, BadReadLeadsToBranch) {
  DisassemblerX86 dis(read_eax_jmp_eax, sizeof(read_eax_jmp_eax)-1, 0);
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_TRUE(dis.setBadRead());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_logic, dis.currentInstructionGroup());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(google_breakpad::DISX86_BAD_BRANCH_TARGET, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_controlflow, dis.currentInstructionGroup());
}

TEST(DisassemblerX86Test, BadWriteLeadsToPushedArg) {
  DisassemblerX86 dis(write_eax_arg_to_call,
                      sizeof(write_eax_arg_to_call)-1, 0);
  EXPECT_EQ(6U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_TRUE(dis.setBadWrite());
  EXPECT_EQ(3U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_arithmetic, dis.currentInstructionGroup());
  EXPECT_EQ(1U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(5U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(google_breakpad::DISX86_BAD_ARGUMENT_PASSED, dis.flags());
  EXPECT_EQ(libdis::insn_controlflow, dis.currentInstructionGroup());
  EXPECT_FALSE(dis.endOfBlock());
}


TEST(DisassemblerX86Test, BadReadLeadsToBlockWrite) {
  DisassemblerX86 dis(read_edi_stosb, sizeof(read_edi_stosb)-1, 0);
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_TRUE(dis.setBadRead());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(google_breakpad::DISX86_BAD_BLOCK_WRITE, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_string, dis.currentInstructionGroup());
}

TEST(DisassemblerX86Test, BadReadClobberThenWrite) {
  DisassemblerX86 dis(read_clobber_write, sizeof(read_clobber_write)-1, 0);
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_arithmetic, dis.currentInstructionGroup());
  EXPECT_TRUE(dis.setBadRead());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
}

TEST(DisassemblerX86Test, BadReadXCHGThenWrite) {
  DisassemblerX86 dis(read_xchg_write, sizeof(read_xchg_write)-1, 0);
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_arithmetic, dis.currentInstructionGroup());
  EXPECT_TRUE(dis.setBadRead());
  EXPECT_EQ(1U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(google_breakpad::DISX86_BAD_WRITE, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_move, dis.currentInstructionGroup());
}

TEST(DisassemblerX86Test, BadReadThenCMP) {
  DisassemblerX86 dis(read_cmp, sizeof(read_cmp)-1, 0);
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(0U, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_arithmetic, dis.currentInstructionGroup());
  EXPECT_TRUE(dis.setBadRead());
  EXPECT_EQ(3U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(google_breakpad::DISX86_BAD_COMPARISON, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_comparison, dis.currentInstructionGroup());
  EXPECT_EQ(2U, dis.NextInstruction());
  EXPECT_TRUE(dis.currentInstructionValid());
  EXPECT_EQ(google_breakpad::DISX86_BAD_COMPARISON, dis.flags());
  EXPECT_FALSE(dis.endOfBlock());
  EXPECT_EQ(libdis::insn_controlflow, dis.currentInstructionGroup());
}
}
