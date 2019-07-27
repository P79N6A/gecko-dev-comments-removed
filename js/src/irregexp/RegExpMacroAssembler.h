





























#ifndef V8_REGEXP_MACRO_ASSEMBLER_H_
#define V8_REGEXP_MACRO_ASSEMBLER_H_

#include "irregexp/RegExpAST.h"
#include "irregexp/RegExpEngine.h"
#include "jit/IonMacroAssembler.h"

namespace js {
namespace irregexp {

class MOZ_STACK_CLASS RegExpMacroAssembler
{
  public:
    RegExpMacroAssembler(LifoAlloc &alloc, RegExpShared *shared, size_t numSavedRegisters)
      : slow_safe_compiler_(false),
        global_mode_(NOT_GLOBAL),
        alloc_(alloc),
        num_registers_(numSavedRegisters),
        num_saved_registers_(numSavedRegisters),
        shared(shared)
    {}

    enum StackCheckFlag {
        kNoStackLimitCheck = false,
        kCheckStackLimit = true
    };

    
    static const int kMaxRegister = (1 << 16) - 1;
    static const int kMaxCPOffset = (1 << 15) - 1;
    static const int kMinCPOffset = -(1 << 15);

    static const int kTableSizeBits = 7;
    static const int kTableSize = 1 << kTableSizeBits;
    static const int kTableMask = kTableSize - 1;

    
    void set_slow_safe(bool ssc) { slow_safe_compiler_ = ssc; }
    bool slow_safe() { return slow_safe_compiler_; }

    enum GlobalMode { NOT_GLOBAL, GLOBAL, GLOBAL_NO_ZERO_LENGTH_CHECK };

    
    
    inline void set_global_mode(GlobalMode mode) { global_mode_ = mode; }
    inline bool global() { return global_mode_ != NOT_GLOBAL; }
    inline bool global_with_zero_length_check() {
        return global_mode_ == GLOBAL;
    }

    LifoAlloc &alloc() { return alloc_; }

    virtual RegExpCode GenerateCode(JSContext *cx, bool match_only) = 0;

    
    
    
    virtual int stack_limit_slack() = 0;

    virtual bool CanReadUnaligned() { return false; }

    virtual void AdvanceCurrentPosition(int by) = 0;  
    virtual void AdvanceRegister(int reg, int by) = 0;  

    
    
    virtual void Backtrack() = 0;

    virtual void Bind(jit::Label* label) = 0;
    virtual void CheckAtStart(jit::Label* on_at_start) = 0;

    
    
    virtual void CheckCharacter(unsigned c, jit::Label* on_equal) = 0;

    
    
    virtual void CheckCharacterAfterAnd(unsigned c, unsigned and_with, jit::Label* on_equal) = 0;

    virtual void CheckCharacterGT(char16_t limit, jit::Label* on_greater) = 0;
    virtual void CheckCharacterLT(char16_t limit, jit::Label* on_less) = 0;
    virtual void CheckGreedyLoop(jit::Label* on_tos_equals_current_position) = 0;
    virtual void CheckNotAtStart(jit::Label* on_not_at_start) = 0;
    virtual void CheckNotBackReference(int start_reg, jit::Label* on_no_match) = 0;
    virtual void CheckNotBackReferenceIgnoreCase(int start_reg, jit::Label* on_no_match) = 0;

    
    
    
    
    virtual void CheckNotCharacter(unsigned c, jit::Label* on_not_equal) = 0;
    virtual void CheckNotCharacterAfterAnd(unsigned c, unsigned and_with, jit::Label* on_not_equal) = 0;

    
    
    virtual void CheckNotCharacterAfterMinusAnd(char16_t c,
                                        char16_t minus,
                                        char16_t and_with,
                                        jit::Label* on_not_equal) = 0;

    virtual void CheckCharacterInRange(char16_t from, char16_t to,  
                               jit::Label* on_in_range) = 0;

    virtual void CheckCharacterNotInRange(char16_t from, char16_t to,  
                                  jit::Label* on_not_in_range) = 0;

    
    
    virtual void CheckBitInTable(uint8_t *table, jit::Label* on_bit_set) = 0;

    
    
    virtual void CheckPosition(int cp_offset, jit::Label* on_outside_input) {
        LoadCurrentCharacter(cp_offset, on_outside_input, true);
    }

    
    virtual void JumpOrBacktrack(jit::Label *to) = 0;

    
    
    
    
    virtual bool CheckSpecialCharacterClass(char16_t type, jit::Label* on_no_match) {
        return false;
    }

    virtual void Fail() = 0;

    
    
    virtual void IfRegisterGE(int reg, int comparand, jit::Label *if_ge) = 0;

    
    
    virtual void IfRegisterLT(int reg, int comparand, jit::Label *if_lt) = 0;

    
    
    virtual void IfRegisterEqPos(int reg, jit::Label *if_eq) = 0;

    virtual void LoadCurrentCharacter(int cp_offset,
                                      jit::Label *on_end_of_input,
                                      bool check_bounds = true,
                                      int characters = 1) = 0;
    virtual void PopCurrentPosition() = 0;
    virtual void PopRegister(int register_index) = 0;

    virtual void PushCurrentPosition() = 0;
    virtual void PushRegister(int register_index, StackCheckFlag check_stack_limit) = 0;
    virtual void ReadCurrentPositionFromRegister(int reg) = 0;
    virtual void ReadBacktrackStackPointerFromRegister(int reg) = 0;
    virtual void SetCurrentPositionFromEnd(int by) = 0;
    virtual void SetRegister(int register_index, int to) = 0;

    
    virtual bool Succeed() = 0;

    virtual void WriteCurrentPositionToRegister(int reg, int cp_offset) = 0;
    virtual void ClearRegisters(int reg_from, int reg_to) = 0;
    virtual void WriteBacktrackStackPointerToRegister(int reg) = 0;

    
    
    virtual void PushBacktrack(jit::Label *label) = 0;

    
    virtual void BindBacktrack(jit::Label *label) = 0;

  private:
    bool slow_safe_compiler_;
    GlobalMode global_mode_;
    LifoAlloc &alloc_;

  protected:
    int num_registers_;
    int num_saved_registers_;

    void checkRegister(int reg) {
        JS_ASSERT(reg >= 0);
        JS_ASSERT(reg <= kMaxRegister);
        if (num_registers_ <= reg)
            num_registers_ = reg + 1;
    }

  public:
    RegExpShared *shared;
};

template <typename CharT>
int
CaseInsensitiveCompareStrings(const CharT *substring1, const CharT *substring2, size_t byteLength);

class MOZ_STACK_CLASS InterpretedRegExpMacroAssembler : public RegExpMacroAssembler
{
  public:
    InterpretedRegExpMacroAssembler(LifoAlloc *alloc, RegExpShared *shared, size_t numSavedRegisters);
    ~InterpretedRegExpMacroAssembler();

    
    RegExpCode GenerateCode(JSContext *cx, bool match_only);
    void AdvanceCurrentPosition(int by);
    void AdvanceRegister(int reg, int by);
    void Backtrack();
    void Bind(jit::Label* label);
    void CheckAtStart(jit::Label* on_at_start);
    void CheckCharacter(unsigned c, jit::Label* on_equal);
    void CheckCharacterAfterAnd(unsigned c, unsigned and_with, jit::Label* on_equal);
    void CheckCharacterGT(char16_t limit, jit::Label* on_greater);
    void CheckCharacterLT(char16_t limit, jit::Label* on_less);
    void CheckGreedyLoop(jit::Label* on_tos_equals_current_position);
    void CheckNotAtStart(jit::Label* on_not_at_start);
    void CheckNotBackReference(int start_reg, jit::Label* on_no_match);
    void CheckNotBackReferenceIgnoreCase(int start_reg, jit::Label* on_no_match);
    void CheckNotCharacter(unsigned c, jit::Label* on_not_equal);
    void CheckNotCharacterAfterAnd(unsigned c, unsigned and_with, jit::Label* on_not_equal);
    void CheckNotCharacterAfterMinusAnd(char16_t c, char16_t minus, char16_t and_with,
                                        jit::Label* on_not_equal);
    void CheckCharacterInRange(char16_t from, char16_t to,
                               jit::Label* on_in_range);
    void CheckCharacterNotInRange(char16_t from, char16_t to,
                                  jit::Label* on_not_in_range);
    void CheckBitInTable(uint8_t *table, jit::Label* on_bit_set);
    void JumpOrBacktrack(jit::Label *to);
    void Fail();
    void IfRegisterGE(int reg, int comparand, jit::Label* if_ge);
    void IfRegisterLT(int reg, int comparand, jit::Label* if_lt);
    void IfRegisterEqPos(int reg, jit::Label* if_eq);
    void LoadCurrentCharacter(int cp_offset, jit::Label* on_end_of_input,
                              bool check_bounds = true, int characters = 1);
    void PopCurrentPosition();
    void PopRegister(int register_index);
    void PushCurrentPosition();
    void PushRegister(int register_index, StackCheckFlag check_stack_limit);
    void ReadCurrentPositionFromRegister(int reg);
    void ReadBacktrackStackPointerFromRegister(int reg);
    void SetCurrentPositionFromEnd(int by);
    void SetRegister(int register_index, int to);
    bool Succeed();
    void WriteCurrentPositionToRegister(int reg, int cp_offset);
    void ClearRegisters(int reg_from, int reg_to);
    void WriteBacktrackStackPointerToRegister(int reg);
    void PushBacktrack(jit::Label *label);
    void BindBacktrack(jit::Label *label);

    
    int stack_limit_slack() { return 1; }

  private:
    void Expand();

    
    void EmitOrLink(jit::Label* label);
    void Emit32(uint32_t x);
    void Emit16(uint32_t x);
    void Emit8(uint32_t x);
    void Emit(uint32_t bc, uint32_t arg);

    jit::Label backtrack_;

    
    int pc_;

    int advance_current_start_;
    int advance_current_offset_;
    int advance_current_end_;

    static const int kInvalidPC = -1;

    uint8_t *buffer_;
    int length_;
};

} }  

#endif  
