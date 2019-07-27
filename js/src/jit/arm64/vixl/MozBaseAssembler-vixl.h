

























#ifndef jit_arm64_vixl_MozBaseAssembler_vixl_h
#define jit_arm64_vixl_MozBaseAssembler_vixl_h

#include "jit/arm64/vixl/Constants-vixl.h"
#include "jit/arm64/vixl/Instructions-vixl.h"

#include "jit/shared/Assembler-shared.h"
#include "jit/shared/IonAssemblerBufferWithConstantPools.h"

namespace vixl {


using js::jit::BufferOffset;


class MozBaseAssembler;
typedef js::jit::AssemblerBufferWithConstantPools<1024, 4, Instruction, MozBaseAssembler> ARMBuffer;



class MozBaseAssembler : public js::jit::AssemblerShared {
  
  static const unsigned BufferGuardSize = 1;
  static const unsigned BufferHeaderSize = 1;
  static const size_t   BufferCodeAlignment = 8;
  static const size_t   BufferMaxPoolOffset = 1024;
  static const unsigned BufferPCBias = 0;
  static const uint32_t BufferAlignmentFillInstruction = BRK | (0xdead << ImmException_offset);
  static const uint32_t BufferNopFillInstruction = HINT | (31 << Rt_offset);
  static const unsigned BufferNumDebugNopsToInsert = 0;

 public:
  MozBaseAssembler()
    : armbuffer_(BufferGuardSize,
                 BufferHeaderSize,
                 BufferCodeAlignment,
                 BufferMaxPoolOffset,
                 BufferPCBias,
                 BufferAlignmentFillInstruction,
                 BufferNopFillInstruction,
                 BufferNumDebugNopsToInsert)
  { }

 public:
  
  
  void initWithAllocator() {
    armbuffer_.initWithAllocator();
  }

  
  Instruction* getInstructionAt(BufferOffset offset) {
    return armbuffer_.getInst(offset);
  }

  
  template <typename T>
  inline T GetLabelByteOffset(const js::jit::Label* label) {
    VIXL_ASSERT(label->bound());
    JS_STATIC_ASSERT(sizeof(T) >= sizeof(uint32_t));
    return reinterpret_cast<T>(label->offset());
  }

 protected:
  
  BufferOffset Emit(Instr instruction, bool isBranch = false) {
    JS_STATIC_ASSERT(sizeof(instruction) == kInstructionSize);
    return armbuffer_.putInt(*(uint32_t*)(&instruction), isBranch);
  }

  BufferOffset EmitBranch(Instr instruction) {
    return Emit(instruction, true);
  }

 public:
  
  static void Emit(Instruction* at, Instr instruction) {
    JS_STATIC_ASSERT(sizeof(instruction) == kInstructionSize);
    memcpy(at, &instruction, sizeof(instruction));
  }

  static void EmitBranch(Instruction* at, Instr instruction) {
    
    Emit(at, instruction);
  }

  
  BufferOffset EmitData(void const * data, unsigned size) {
    VIXL_ASSERT(size % 4 == 0);
    return armbuffer_.allocEntry(size / sizeof(uint32_t), 0, (uint8_t*)(data), nullptr);
  }

 public:
  
  size_t SizeOfCodeGenerated() const {
    return armbuffer_.size();
  }

  
  void flushBuffer() {
    armbuffer_.flushPool();
  }

  
  
  
  
  void enterNoPool(size_t maxInst) {
    armbuffer_.enterNoPool(maxInst);
  }

  
  void leaveNoPool() {
    armbuffer_.leaveNoPool();
  }

 public:
  
  static void InsertIndexIntoTag(uint8_t* load, uint32_t index);
  static bool PatchConstantPoolLoad(void* loadAddr, void* constPoolAddr);
  static uint32_t PlaceConstantPoolBarrier(int offset);

  static void WritePoolHeader(uint8_t* start, js::jit::Pool* p, bool isNatural);
  static void WritePoolFooter(uint8_t* start, js::jit::Pool* p, bool isNatural);
  static void WritePoolGuard(BufferOffset branch, Instruction* inst, BufferOffset dest);

  static ptrdiff_t GetBranchOffset(const Instruction* i);
  static void RetargetNearBranch(Instruction* i, int offset, Condition cond, bool final = true);
  static void RetargetNearBranch(Instruction* i, int offset, bool final = true);
  static void RetargetFarBranch(Instruction* i, uint8_t** slot, uint8_t* dest, Condition cond);

 protected:
  
  ARMBuffer armbuffer_;

  js::jit::CompactBufferWriter jumpRelocations_;
  js::jit::CompactBufferWriter dataRelocations_;
  js::jit::CompactBufferWriter relocations_;
  js::jit::CompactBufferWriter preBarriers_;

  
  mozilla::Array<js::jit::Pool, 4> pools_;
};


}  


#endif  

