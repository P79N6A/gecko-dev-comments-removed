

























#ifndef VIXL_A64_DECODER_A64_H_
#define VIXL_A64_DECODER_A64_H_

#include <list>

#include "jit/arm64/vixl/Globals-vixl.h"
#include "jit/arm64/vixl/Instructions-vixl.h"




#define VISITOR_LIST(V)             \
  V(PCRelAddressing)                \
  V(AddSubImmediate)                \
  V(LogicalImmediate)               \
  V(MoveWideImmediate)              \
  V(Bitfield)                       \
  V(Extract)                        \
  V(UnconditionalBranch)            \
  V(UnconditionalBranchToRegister)  \
  V(CompareBranch)                  \
  V(TestBranch)                     \
  V(ConditionalBranch)              \
  V(System)                         \
  V(Exception)                      \
  V(LoadStorePairPostIndex)         \
  V(LoadStorePairOffset)            \
  V(LoadStorePairPreIndex)          \
  V(LoadStorePairNonTemporal)       \
  V(LoadLiteral)                    \
  V(LoadStoreUnscaledOffset)        \
  V(LoadStorePostIndex)             \
  V(LoadStorePreIndex)              \
  V(LoadStoreRegisterOffset)        \
  V(LoadStoreUnsignedOffset)        \
  V(LoadStoreExclusive)             \
  V(LogicalShifted)                 \
  V(AddSubShifted)                  \
  V(AddSubExtended)                 \
  V(AddSubWithCarry)                \
  V(ConditionalCompareRegister)     \
  V(ConditionalCompareImmediate)    \
  V(ConditionalSelect)              \
  V(DataProcessing1Source)          \
  V(DataProcessing2Source)          \
  V(DataProcessing3Source)          \
  V(FPCompare)                      \
  V(FPConditionalCompare)           \
  V(FPConditionalSelect)            \
  V(FPImmediate)                    \
  V(FPDataProcessing1Source)        \
  V(FPDataProcessing2Source)        \
  V(FPDataProcessing3Source)        \
  V(FPIntegerConvert)               \
  V(FPFixedPointConvert)            \
  V(Unallocated)                    \
  V(Unimplemented)

namespace vixl {



class DecoderVisitor {
 public:
  enum VisitorConstness {
    kConstVisitor,
    kNonConstVisitor
  };
  explicit DecoderVisitor(VisitorConstness constness = kConstVisitor)
      : constness_(constness) {}

  virtual ~DecoderVisitor() {}

  #define DECLARE(A) virtual void Visit##A(const Instruction* instr) = 0;
  VISITOR_LIST(DECLARE)
  #undef DECLARE

  bool IsConstVisitor() const { return constness_ == kConstVisitor; }
  Instruction* MutableInstruction(const Instruction* instr) {
    VIXL_ASSERT(!IsConstVisitor());
    return const_cast<Instruction*>(instr);
  }

 private:
  const VisitorConstness constness_;
};


class Decoder {
 public:
  Decoder() {}

  
  void Decode(const Instruction* instr) {
    std::list<DecoderVisitor*>::iterator it;
    for (it = visitors_.begin(); it != visitors_.end(); it++) {
      VIXL_ASSERT((*it)->IsConstVisitor());
    }
    DecodeInstruction(instr);
  }
  void Decode(Instruction* instr) {
    DecodeInstruction(const_cast<const Instruction*>(instr));
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  void AppendVisitor(DecoderVisitor* visitor);
  void PrependVisitor(DecoderVisitor* visitor);
  
  
  
  
  
  
  
  
  
  
  
  
  
  void InsertVisitorBefore(DecoderVisitor* new_visitor,
                           DecoderVisitor* registered_visitor);
  void InsertVisitorAfter(DecoderVisitor* new_visitor,
                          DecoderVisitor* registered_visitor);

  
  
  void RemoveVisitor(DecoderVisitor* visitor);

  #define DECLARE(A) void Visit##A(const Instruction* instr);
  VISITOR_LIST(DECLARE)
  #undef DECLARE


  std::list<DecoderVisitor*>* visitors() { return &visitors_; }

 private:
  
  
  void DecodeInstruction(const Instruction* instr);

  
  
  
  void DecodePCRelAddressing(const Instruction* instr);

  
  
  
  void DecodeAddSubImmediate(const Instruction* instr);

  
  
  
  void DecodeBranchSystemException(const Instruction* instr);

  
  
  
  void DecodeLoadStore(const Instruction* instr);

  
  
  
  void DecodeLogical(const Instruction* instr);

  
  
  
  void DecodeBitfieldExtract(const Instruction* instr);

  
  
  
  void DecodeDataProcessing(const Instruction* instr);

  
  
  
  void DecodeFP(const Instruction* instr);

  
  
  
  void DecodeAdvSIMDLoadStore(const Instruction* instr);

  
  
  
  void DecodeAdvSIMDDataProcessing(const Instruction* instr);

 private:
  
  std::list<DecoderVisitor*> visitors_;
};

}  

#endif
