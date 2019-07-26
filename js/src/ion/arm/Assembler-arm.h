






#ifndef jsion_cpu_arm_assembler_h__
#define jsion_cpu_arm_assembler_h__

#include "mozilla/MathAlgorithms.h"
#include "mozilla/Util.h"

#include "ion/shared/Assembler-shared.h"
#include "assembler/assembler/AssemblerBufferWithConstantPool.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"
#include "ion/arm/Architecture-arm.h"
#include "ion/shared/IonAssemblerBufferWithConstantPools.h"

namespace js {
namespace ion {







static const Register r0  = { Registers::r0 };
static const Register r1  = { Registers::r1 };
static const Register r2  = { Registers::r2 };
static const Register r3  = { Registers::r3 };
static const Register r4  = { Registers::r4 };
static const Register r5  = { Registers::r5 };
static const Register r6  = { Registers::r6 };
static const Register r7  = { Registers::r7 };
static const Register r8  = { Registers::r8 };
static const Register r9  = { Registers::r9 };
static const Register r10 = { Registers::r10 };
static const Register r11 = { Registers::r11 };
static const Register r12 = { Registers::ip };
static const Register ip  = { Registers::ip };
static const Register sp  = { Registers::sp };
static const Register r14 = { Registers::lr };
static const Register lr  = { Registers::lr };
static const Register pc  = { Registers::pc };

static const Register ScratchRegister = {Registers::ip};

static const Register OsrFrameReg = r3;
static const Register ArgumentsRectifierReg = r8;
static const Register CallTempReg0 = r5;
static const Register CallTempReg1 = r6;
static const Register CallTempReg2 = r7;
static const Register CallTempReg3 = r8;
static const Register CallTempReg4 = r0;
static const Register CallTempReg5 = r1;

static const Register CallTempNonArgRegs[] = { r5, r6, r7, r8 };
static const uint32_t NumCallTempNonArgRegs =
    mozilla::ArrayLength(CallTempNonArgRegs);

static const Register PreBarrierReg = r1;

static const Register InvalidReg = { Registers::invalid_reg };
static const FloatRegister InvalidFloatReg = { FloatRegisters::invalid_freg };

static const Register JSReturnReg_Type = r3;
static const Register JSReturnReg_Data = r2;
static const Register StackPointer = sp;
static const Register FramePointer = InvalidReg;
static const Register ReturnReg = r0;
static const FloatRegister ReturnFloatReg = { FloatRegisters::d0 };
static const FloatRegister ScratchFloatReg = { FloatRegisters::d1 };

static const FloatRegister d0  = {FloatRegisters::d0};
static const FloatRegister d1  = {FloatRegisters::d1};
static const FloatRegister d2  = {FloatRegisters::d2};
static const FloatRegister d3  = {FloatRegisters::d3};
static const FloatRegister d4  = {FloatRegisters::d4};
static const FloatRegister d5  = {FloatRegisters::d5};
static const FloatRegister d6  = {FloatRegisters::d6};
static const FloatRegister d7  = {FloatRegisters::d7};
static const FloatRegister d8  = {FloatRegisters::d8};
static const FloatRegister d9  = {FloatRegisters::d9};
static const FloatRegister d10 = {FloatRegisters::d10};
static const FloatRegister d11 = {FloatRegisters::d11};
static const FloatRegister d12 = {FloatRegisters::d12};
static const FloatRegister d13 = {FloatRegisters::d13};
static const FloatRegister d14 = {FloatRegisters::d14};
static const FloatRegister d15 = {FloatRegisters::d15};






static const uint32_t StackAlignment = 8;
static const bool StackKeptAligned = true;
static const uint32_t NativeFrameSize = sizeof(void*);
static const uint32_t AlignmentAtPrologue = sizeof(void*);

static const Scale ScalePointer = TimesFour;

class Instruction;
class InstBranchImm;
uint32_t RM(Register r);
uint32_t RS(Register r);
uint32_t RD(Register r);
uint32_t RT(Register r);
uint32_t RN(Register r);

uint32_t maybeRD(Register r);
uint32_t maybeRT(Register r);
uint32_t maybeRN(Register r);

Register toRN (Instruction &i);
Register toRM (Instruction &i);
Register toRD (Instruction &i);
Register toR (Instruction &i);

class VFPRegister;
uint32_t VD(VFPRegister vr);
uint32_t VN(VFPRegister vr);
uint32_t VM(VFPRegister vr);

class VFPRegister
{
  public:
    
    
    
    enum RegType {
        Double = 0x0,
        Single = 0x1,
        UInt   = 0x2,
        Int    = 0x3
    };

  protected:
    RegType kind : 2;
    
    
    
    
    
    
    
    uint32_t _code : 5;
    bool _isInvalid : 1;
    bool _isMissing : 1;

    VFPRegister(int  r, RegType k)
      : kind(k), _code (r), _isInvalid(false), _isMissing(false)
    { }

  public:
    VFPRegister()
      : _isInvalid(true), _isMissing(false)
    { }

    VFPRegister(bool b)
      : _isInvalid(false), _isMissing(b)
    { }

    VFPRegister(FloatRegister fr)
      : kind(Double), _code(fr.code()), _isInvalid(false), _isMissing(false)
    {
        JS_ASSERT(_code == (unsigned)fr.code());
    }

    VFPRegister(FloatRegister fr, RegType k)
      : kind(k), _code (fr.code()), _isInvalid(false), _isMissing(false)
    {
        JS_ASSERT(_code == (unsigned)fr.code());
    }
    bool isDouble() { return kind == Double; }
    bool isSingle() { return kind == Single; }
    bool isFloat() { return (kind == Double) || (kind == Single); }
    bool isInt() { return (kind == UInt) || (kind == Int); }
    bool isSInt()   { return kind == Int; }
    bool isUInt()   { return kind == UInt; }
    bool equiv(VFPRegister other) { return other.kind == kind; }
    size_t size() { return (kind == Double) ? 8 : 4; }
    bool isInvalid();
    bool isMissing();

    VFPRegister doubleOverlay();
    VFPRegister singleOverlay();
    VFPRegister sintOverlay();
    VFPRegister uintOverlay();

    struct VFPRegIndexSplit;
    VFPRegIndexSplit encode();

    
    struct VFPRegIndexSplit {
        const uint32_t block : 4;
        const uint32_t bit : 1;

      private:
        friend VFPRegIndexSplit js::ion::VFPRegister::encode();

        VFPRegIndexSplit (uint32_t block_, uint32_t bit_)
          : block(block_), bit(bit_)
        {
            JS_ASSERT (block == block_);
            JS_ASSERT(bit == bit_);
        }
    };

    uint32_t code() const {
        return _code;
    }
};



extern VFPRegister NoVFPRegister;

struct ImmTag : public Imm32
{
    ImmTag(JSValueTag mask)
      : Imm32(int32_t(mask))
    { }
};

struct ImmType : public ImmTag
{
    ImmType(JSValueType type)
      : ImmTag(JSVAL_TYPE_TO_TAG(type))
    { }
};

enum Index {
    Offset = 0 << 21 | 1<<24,
    PreIndex = 1<<21 | 1 << 24,
    PostIndex = 0 << 21 | 0 << 24
    
    
};


enum IsImmOp2_ {
    IsImmOp2    = 1 << 25,
    IsNotImmOp2 = 0 << 25
};
enum IsImmDTR_ {
    IsImmDTR    = 0 << 25,
    IsNotImmDTR = 1 << 25
};

enum IsImmEDTR_ {
    IsImmEDTR    = 1 << 22,
    IsNotImmEDTR = 0 << 22
};


enum ShiftType {
    LSL = 0, 
    LSR = 1, 
    ASR = 2, 
    ROR = 3, 
    RRX = ROR 
};



struct ConditionCodes
{
    bool Zero : 1;
    bool Overflow : 1;
    bool Carry : 1;
    bool Minus : 1;
};




enum DTMMode {
    A = 0 << 24, 
    B = 1 << 24, 
    D = 0 << 23, 
    I = 1 << 23, 
    DA = D | A,
    DB = D | B,
    IA = I | A,
    IB = I | B
};

enum DTMWriteBack {
    WriteBack   = 1 << 21,
    NoWriteBack = 0 << 21
};

enum SetCond_ {
    SetCond   = 1 << 20,
    NoSetCond = 0 << 20
};
enum LoadStore {
    IsLoad  = 1 << 20,
    IsStore = 0 << 20
};




enum IsUp_ {
    IsUp   = 1 << 23,
    IsDown = 0 << 23
};
enum ALUOp {
    op_mov = 0xd << 21,
    op_mvn = 0xf << 21,
    op_and = 0x0 << 21,
    op_bic = 0xe << 21,
    op_eor = 0x1 << 21,
    op_orr = 0xc << 21,
    op_adc = 0x5 << 21,
    op_add = 0x4 << 21,
    op_sbc = 0x6 << 21,
    op_sub = 0x2 << 21,
    op_rsb = 0x3 << 21,
    op_rsc = 0x7 << 21,
    op_cmn = 0xb << 21,
    op_cmp = 0xa << 21,
    op_teq = 0x9 << 21,
    op_tst = 0x8 << 21,
    op_invalid = -1
};


enum MULOp {
    opm_mul   = 0 << 21,
    opm_mla   = 1 << 21,
    opm_umaal = 2 << 21,
    opm_mls   = 3 << 21,
    opm_umull = 4 << 21,
    opm_umlal = 5 << 21,
    opm_smull = 6 << 21,
    opm_smlal = 7 << 21
};
enum BranchTag {
    op_b = 0x0a000000,
    op_b_mask = 0x0f000000,
    op_b_dest_mask = 0x00ffffff,
    op_bl = 0x0b000000,
    op_blx = 0x012fff30,
    op_bx  = 0x012fff10
};


enum VFPOp {
    opv_mul  = 0x2 << 20,
    opv_add  = 0x3 << 20,
    opv_sub  = 0x3 << 20 | 0x1 << 6,
    opv_div  = 0x8 << 20,
    opv_mov  = 0xB << 20 | 0x1 << 6,
    opv_abs  = 0xB << 20 | 0x3 << 6,
    opv_neg  = 0xB << 20 | 0x1 << 6 | 0x1 << 16,
    opv_sqrt = 0xB << 20 | 0x3 << 6 | 0x1 << 16,
    opv_cmp  = 0xB << 20 | 0x1 << 6 | 0x4 << 16,
    opv_cmpz  = 0xB << 20 | 0x1 << 6 | 0x5 << 16
};

ALUOp ALUNeg(ALUOp op, Register dest, Imm32 *imm, Register *negDest);
bool can_dbl(ALUOp op);
bool condsAreSafe(ALUOp op);


ALUOp getDestVariant(ALUOp op);

static const ValueOperand JSReturnOperand = ValueOperand(JSReturnReg_Type, JSReturnReg_Data);



















namespace datastore {
struct Reg
{
    
    uint32_t RM : 4;
    
    uint32_t RRS : 1;
    ShiftType Type : 2;
    
    
    uint32_t ShiftAmount : 5;
    uint32_t pad : 20;

    Reg(uint32_t rm, ShiftType type, uint32_t rsr, uint32_t shiftamount)
      : RM(rm), RRS(rsr), Type(type), ShiftAmount(shiftamount), pad(0)
    { }

    uint32_t encode() {
        return RM | RRS << 4 | Type << 5 | ShiftAmount << 7;
    }
};





struct Imm8mData
{
  private:
    uint32_t data : 8;
    uint32_t rot : 4;
    
    
    
    uint32_t buff : 19;
  public:
    uint32_t invalid : 1;

    uint32_t encode() {
        JS_ASSERT(!invalid);
        return data | rot << 8;
    };

    
    Imm8mData()
      : data(0xff), rot(0xf), invalid(1)
    { }

    Imm8mData(uint32_t data_, uint32_t rot_)
      : data(data_), rot(rot_), invalid(0)
    {
        JS_ASSERT(data == data_);
        JS_ASSERT(rot == rot_);
    }
};

struct Imm8Data
{
  private:
    uint32_t imm4L : 4;
    uint32_t pad : 4;
    uint32_t imm4H : 4;

  public:
    uint32_t encode() {
        return imm4L | (imm4H << 8);
    };
    Imm8Data(uint32_t imm) : imm4L(imm&0xf), imm4H(imm>>4) {
        JS_ASSERT(imm <= 0xff);
    }
};



struct Imm8VFPOffData
{
  private:
    uint32_t data;

  public:
    uint32_t encode() {
        return data;
    };
    Imm8VFPOffData(uint32_t imm) : data (imm) {
        JS_ASSERT((imm & ~(0xff)) == 0);
    }
};



struct Imm8VFPImmData
{
  private:
    uint32_t imm4L : 4;
    uint32_t pad : 12;
    uint32_t imm4H : 4;
    int32_t isInvalid : 12;

  public:
    Imm8VFPImmData()
      : imm4L(-1U & 0xf), imm4H(-1U & 0xf), isInvalid(-1)
    { }

    Imm8VFPImmData(uint32_t imm)
      : imm4L(imm&0xf), imm4H(imm>>4), isInvalid(0)
    {
        JS_ASSERT(imm <= 0xff);
    }

    uint32_t encode() {
        if (isInvalid != 0)
            return -1;
        return imm4L | (imm4H << 16);
    };
};

struct Imm12Data
{
    uint32_t data : 12;
    uint32_t encode() {
        return data;
    }

    Imm12Data(uint32_t imm)
      : data(imm)
    {
        JS_ASSERT(data == imm);
    }

};

struct RIS
{
    uint32_t ShiftAmount : 5;
    uint32_t encode () {
        return ShiftAmount;
    }

    RIS(uint32_t imm)
      : ShiftAmount(imm)
    {
        JS_ASSERT(ShiftAmount == imm);
    }
};

struct RRS
{
    uint32_t MustZero : 1;
    
    uint32_t RS : 4;

    RRS(uint32_t rs)
      : RS(rs)
    {
        JS_ASSERT(rs == RS);
    }

    uint32_t encode () {
        return RS << 1;
    }
};

} 

class MacroAssemblerARM;
class Operand;

class Operand2
{
    friend class Operand;
    friend class MacroAssemblerARM;

  public:
    uint32_t oper : 31;
    uint32_t invalid : 1;

  protected:
    Operand2(datastore::Imm8mData base)
      : oper(base.invalid ? -1 : (base.encode() | (uint32_t)IsImmOp2)),
        invalid(base.invalid)
    { }

    Operand2(datastore::Reg base)
      : oper(base.encode() | (uint32_t)IsNotImmOp2)
    { }

  private:
    Operand2(int blob)
      : oper(blob)
    { }

  public:
    uint32_t encode() {
        return oper;
    }
};

class Imm8 : public Operand2
{
  public:
    static datastore::Imm8mData encodeImm(uint32_t imm) {
        
        if (imm == 0)
            return datastore::Imm8mData(0, 0);
        int left = js_bitscan_clz32(imm) & 30;
        
        
        
        if (left >= 24)
            return datastore::Imm8mData(imm, 0);

        
        
        int no_imm = imm & ~(0xff << (24 - left));
        if (no_imm == 0) {
            return  datastore::Imm8mData(imm >> (24 - left), ((8+left) >> 1));
        }
        
        int right = 32 - (js_bitscan_clz32(no_imm)  & 30);
        
        
        if (right >= 8)
            return datastore::Imm8mData();
        
        
        unsigned int mask = imm << (8 - right) | imm >> (24 + right);
        if (mask <= 0xff)
            return datastore::Imm8mData(mask, (8-right) >> 1);
        return datastore::Imm8mData();
    }
    
    struct TwoImm8mData
    {
        datastore::Imm8mData fst, snd;

        TwoImm8mData()
          : fst(), snd()
        { }

        TwoImm8mData(datastore::Imm8mData _fst, datastore::Imm8mData _snd)
          : fst(_fst), snd(_snd)
        { }
    };

    static TwoImm8mData encodeTwoImms(uint32_t);
    Imm8(uint32_t imm)
      : Operand2(encodeImm(imm))
    { }
};

class Op2Reg : public Operand2
{
  public:
    Op2Reg(Register rm, ShiftType type, datastore::RIS shiftImm)
      : Operand2(datastore::Reg(rm.code(), type, 0, shiftImm.encode()))
    { }

    Op2Reg(Register rm, ShiftType type, datastore::RRS shiftReg)
      : Operand2(datastore::Reg(rm.code(), type, 1, shiftReg.encode()))
    { }
};

class O2RegImmShift : public Op2Reg
{
  public:
    O2RegImmShift(Register rn, ShiftType type, uint32_t shift)
      : Op2Reg(rn, type, datastore::RIS(shift))
    { }
};

class O2RegRegShift : public Op2Reg
{
  public:
    O2RegRegShift(Register rn, ShiftType type, Register rs)
      : Op2Reg(rn, type, datastore::RRS(rs.code()))
    { }
};

O2RegImmShift O2Reg(Register r);
O2RegImmShift lsl (Register r, int amt);
O2RegImmShift lsr (Register r, int amt);
O2RegImmShift asr (Register r, int amt);
O2RegImmShift rol (Register r, int amt);
O2RegImmShift ror (Register r, int amt);

O2RegRegShift lsl (Register r, Register amt);
O2RegRegShift lsr (Register r, Register amt);
O2RegRegShift asr (Register r, Register amt);
O2RegRegShift ror (Register r, Register amt);






class DtrOff
{
    uint32_t data;

  protected:
    DtrOff(datastore::Imm12Data immdata, IsUp_ iu)
      : data(immdata.encode() | (uint32_t)IsImmDTR | ((uint32_t)iu))
    { }

    DtrOff(datastore::Reg reg, IsUp_ iu = IsUp)
      : data(reg.encode() | (uint32_t) IsNotImmDTR | iu)
    { }

  public:
    uint32_t encode() { return data; }
};

class DtrOffImm : public DtrOff
{
  public:
    DtrOffImm(int32_t imm)
      : DtrOff(datastore::Imm12Data(mozilla::Abs(imm)), imm >= 0 ? IsUp : IsDown)
    {
        JS_ASSERT(mozilla::Abs(imm) < 4096);
    }
};

class DtrOffReg : public DtrOff
{
    
    
  protected:
    DtrOffReg(Register rn, ShiftType type, datastore::RIS shiftImm, IsUp_ iu = IsUp)
      : DtrOff(datastore::Reg(rn.code(), type, 0, shiftImm.encode()), iu)
    { }

    DtrOffReg(Register rn, ShiftType type, datastore::RRS shiftReg, IsUp_ iu = IsUp)
      : DtrOff(datastore::Reg(rn.code(), type, 1, shiftReg.encode()), iu)
    { }
};

class DtrRegImmShift : public DtrOffReg
{
  public:
    DtrRegImmShift(Register rn, ShiftType type, uint32_t shift, IsUp_ iu = IsUp)
      : DtrOffReg(rn, type, datastore::RIS(shift), iu)
    { }
};

class DtrRegRegShift : public DtrOffReg
{
  public:
    DtrRegRegShift(Register rn, ShiftType type, Register rs, IsUp_ iu = IsUp)
      : DtrOffReg(rn, type, datastore::RRS(rs.code()), iu)
    { }
};



class DTRAddr
{
    uint32_t data;

  public:
    DTRAddr(Register reg, DtrOff dtr)
      : data(dtr.encode() | (reg.code() << 16))
    { }

    uint32_t encode() {
        return data;
    }
    Register getBase() {
        return Register::FromCode((data >> 16) &0xf);
    }
  private:
    friend class Operand;
    DTRAddr(uint32_t blob)
      : data(blob)
    { }
};



class EDtrOff
{
    uint32_t data;

  protected:
    EDtrOff(datastore::Imm8Data imm8, IsUp_ iu = IsUp)
      : data(imm8.encode() | IsImmEDTR | (uint32_t)iu)
    { }

    EDtrOff(Register rm, IsUp_ iu = IsUp)
      : data(rm.code() | IsNotImmEDTR | iu)
    { }

  public:
    uint32_t encode() {
        return data;
    }
};

class EDtrOffImm : public EDtrOff
{
  public:
    EDtrOffImm(int32_t imm)
      : EDtrOff(datastore::Imm8Data(mozilla::Abs(imm)), (imm >= 0) ? IsUp : IsDown)
    {
        JS_ASSERT(mozilla::Abs(imm) < 256);
    }
};




class EDtrOffReg : public EDtrOff
{
  public:
    EDtrOffReg(Register rm)
      : EDtrOff(rm)
    { }
};

class EDtrAddr
{
    uint32_t data;

  public:
    EDtrAddr(Register r, EDtrOff off)
      : data(RN(r) | off.encode())
    { }

    uint32_t encode() {
        return data;
    }
};

class VFPOff
{
    uint32_t data;

  protected:
    VFPOff(datastore::Imm8VFPOffData imm, IsUp_ isup)
      : data(imm.encode() | (uint32_t)isup)
    { }

  public:
    uint32_t encode() {
        return data;
    }
};

class VFPOffImm : public VFPOff
{
  public:
    VFPOffImm(int32_t imm)
      : VFPOff(datastore::Imm8VFPOffData(mozilla::Abs(imm) / 4), imm < 0 ? IsDown : IsUp)
    {
        JS_ASSERT(mozilla::Abs(imm) <= 255 * 4);
    }
};
class VFPAddr
{
    friend class Operand;

    uint32_t data;

  protected:
    VFPAddr(uint32_t blob)
      : data(blob)
    { }

  public:
    VFPAddr(Register base, VFPOff off)
      : data(RN(base) | off.encode())
    { }

    uint32_t encode() {
        return data;
    }
};

class VFPImm {
    uint32_t data;

  public:
    VFPImm(uint32_t top);

    uint32_t encode() {
        return data;
    }
    bool isValid() {
        return data != -1U;
    }
};



class BOffImm
{
    uint32_t data;

  public:
    uint32_t encode() {
        return data;
    }
    int32_t decode() {
        return ((((int32_t)data) << 8) >> 6) + 8;
    }

    explicit BOffImm(int offset)
      : data ((offset - 8) >> 2 & 0x00ffffff)
    {
        JS_ASSERT((offset & 0x3) == 0);
        JS_ASSERT(isInRange(offset));
    }
    static bool isInRange(int offset)
    {
        if ((offset - 8) < -33554432)
            return false;
        if ((offset - 8) > 33554428)
            return false;
        return true;
    }
    static const int INVALID = 0x00800000;
    BOffImm()
      : data(INVALID)
    { }

    bool isInvalid() {
        return data == uint32_t(INVALID);
    }
    Instruction *getDest(Instruction *src);

  private:
    friend class InstBranchImm;
    BOffImm(Instruction &inst);
};

class Imm16
{
    uint32_t lower : 12;
    uint32_t pad : 4;
    uint32_t upper : 4;
    uint32_t invalid : 12;

  public:
    Imm16();
    Imm16(uint32_t imm);
    Imm16(Instruction &inst);

    uint32_t encode() {
        return lower | upper << 16;
    }
    uint32_t decode() {
        return lower | upper << 12;
    }

    bool isInvalid () {
        return invalid;
    }
};





class FloatOp
{
    uint32_t data;
};





class Operand
{
    
    
    
  public:
    enum Tag_ {
        OP2,
        MEM,
        FOP
    };

  private:
    Tag_ Tag : 3;
    uint32_t reg : 5;
    int32_t offset;
    uint32_t data;

  public:
    Operand (Register reg_)
      : Tag(OP2), reg(reg_.code())
    { }

    Operand (FloatRegister freg)
      : Tag(FOP), reg(freg.code())
    { }

    Operand (Register base, Imm32 off)
      : Tag(MEM), reg(base.code()), offset(off.value)
    { }

    Operand (Register base, int32_t off)
      : Tag(MEM), reg(base.code()), offset(off)
    { }

    Operand (const Address &addr)
      : Tag(MEM), reg(addr.base.code()), offset(addr.offset)
    { }

    Tag_ getTag() const {
        return Tag;
    }

    Operand2 toOp2() const {
        JS_ASSERT(Tag == OP2);
        return O2Reg(Register::FromCode(reg));
    }

    Register toReg() const {
        JS_ASSERT(Tag == OP2);
        return Register::FromCode(reg);
    }

    void toAddr(Register *r, Imm32 *dest) const {
        JS_ASSERT(Tag == MEM);
        *r = Register::FromCode(reg);
        *dest = Imm32(offset);
    }
    Address toAddress() const {
        return Address(Register::FromCode(reg), offset);
    }
    int32_t disp() const {
        JS_ASSERT(Tag == MEM);
        return offset;
    }

    int32_t base() const {
        JS_ASSERT(Tag == MEM);
        return reg;
    }
    Register baseReg() const {
        return Register::FromCode(reg);
    }
    DTRAddr toDTRAddr() const {
        return DTRAddr(baseReg(), DtrOffImm(offset));
    }
    VFPAddr toVFPAddr() const {
        return VFPAddr(baseReg(), VFPOffImm(offset));
    }
};

void
PatchJump(CodeLocationJump &jump_, CodeLocationLabel label);
class InstructionIterator;
class Assembler;
typedef js::ion::AssemblerBufferWithConstantPool<1024, 4, Instruction, Assembler, 1> ARMBuffer;

class Assembler
{
  public:
    
    typedef enum {
        EQ = 0x00000000, 
        NE = 0x10000000, 
        CS = 0x20000000,
        CC = 0x30000000,
        MI = 0x40000000,
        PL = 0x50000000,
        VS = 0x60000000,
        VC = 0x70000000,
        HI = 0x80000000,
        LS = 0x90000000,
        GE = 0xa0000000,
        LT = 0xb0000000,
        GT = 0xc0000000,
        LE = 0xd0000000,
        AL = 0xe0000000
    } ARMCondition;

    enum Condition {
        Equal = EQ,
        NotEqual = NE,
        Above = HI,
        AboveOrEqual = CS,
        Below = CC,
        BelowOrEqual = LS,
        GreaterThan = GT,
        GreaterThanOrEqual = GE,
        LessThan = LT,
        LessThanOrEqual = LE,
        Overflow = VS,
        Signed = MI,
        Unsigned = PL,
        Zero = EQ,
        NonZero = NE,
        Always  = AL,

        VFP_NotEqualOrUnordered = NE,
        VFP_Equal = EQ,
        VFP_Unordered = VS,
        VFP_NotUnordered = VC,
        VFP_GreaterThanOrEqualOrUnordered = CS,
        VFP_GreaterThanOrEqual = GE,
        VFP_GreaterThanOrUnordered = HI,
        VFP_GreaterThan = GT,
        VFP_LessThanOrEqualOrUnordered = LE,
        VFP_LessThanOrEqual = LS,
        VFP_LessThanOrUnordered = LT,
        VFP_LessThan = CC 
    };

    
    
    
    static const int DoubleConditionBitSpecial = 0x1;

    enum DoubleCondition {
        
        DoubleOrdered = VFP_NotUnordered,
        DoubleEqual = VFP_Equal,
        DoubleNotEqual = VFP_NotEqualOrUnordered | DoubleConditionBitSpecial,
        DoubleGreaterThan = VFP_GreaterThan,
        DoubleGreaterThanOrEqual = VFP_GreaterThanOrEqual,
        DoubleLessThan = VFP_LessThan,
        DoubleLessThanOrEqual = VFP_LessThanOrEqual,
        
        DoubleUnordered = VFP_Unordered,
        DoubleEqualOrUnordered = VFP_Equal | DoubleConditionBitSpecial,
        DoubleNotEqualOrUnordered = VFP_NotEqualOrUnordered,
        DoubleGreaterThanOrUnordered = VFP_GreaterThanOrUnordered,
        DoubleGreaterThanOrEqualOrUnordered = VFP_GreaterThanOrEqualOrUnordered,
        DoubleLessThanOrUnordered = VFP_LessThanOrUnordered,
        DoubleLessThanOrEqualOrUnordered = VFP_LessThanOrEqualOrUnordered
    };

    Condition getCondition(uint32_t inst) {
        return (Condition) (0xf0000000 & inst);
    }
    static inline Condition ConditionFromDoubleCondition(DoubleCondition cond) {
        JS_ASSERT(!(cond & DoubleConditionBitSpecial));
        return static_cast<Condition>(cond);
    }

    
    

    BufferOffset nextOffset() {
        return m_buffer.nextOffset();
    }

  protected:
    BufferOffset labelOffset (Label *l) {
        return BufferOffset(l->bound());
    }

    Instruction * editSrc (BufferOffset bo) {
        return m_buffer.getInst(bo);
    }
  public:
    void resetCounter();
    uint32_t actualOffset(uint32_t) const;
    uint32_t actualIndex(uint32_t) const;
    static uint8_t *PatchableJumpAddress(IonCode *code, uint32_t index);
    BufferOffset actualOffset(BufferOffset) const;
  protected:

    
    
    struct RelativePatch
    {
        
        
        BufferOffset offset;
        void *target;
        Relocation::Kind kind;
        void fixOffset(ARMBuffer &m_buffer) {
            offset = BufferOffset(offset.getOffset() + m_buffer.poolSizeBefore(offset.getOffset()));
        }
        RelativePatch(BufferOffset offset, void *target, Relocation::Kind kind)
          : offset(offset),
            target(target),
            kind(kind)
        { }
    };

    
    
    class JumpPool;
    js::Vector<CodeLabel, 0, SystemAllocPolicy> codeLabels_;
    js::Vector<RelativePatch, 8, SystemAllocPolicy> jumps_;
    js::Vector<JumpPool *, 0, SystemAllocPolicy> jumpPools_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpJumpRelocations_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpDataRelocations_;
    js::Vector<BufferOffset, 0, SystemAllocPolicy> tmpPreBarriers_;
    class JumpPool : TempObject
    {
        BufferOffset start;
        uint32_t size;
        bool fixup(IonCode *code, uint8_t *data);
    };

    CompactBufferWriter jumpRelocations_;
    CompactBufferWriter dataRelocations_;
    CompactBufferWriter relocations_;
    CompactBufferWriter preBarriers_;

    bool enoughMemory_;

    
    ARMBuffer m_buffer;

    
    
    
    
    
    
    
    
    
    static Assembler *dummy;
    Pool pools_[4];
    Pool *int32Pool;
    Pool *doublePool;

  public:
    Assembler()
      : enoughMemory_(true),
        m_buffer(4, 4, 0, &pools_[0], 8),
        int32Pool(m_buffer.getPool(1)),
        doublePool(m_buffer.getPool(0)),
        isFinished(false),
        dtmActive(false),
        dtmCond(Always)
    {
    }

    
    
    void initWithAllocator() {
        m_buffer.initWithAllocator();

        
        new (&pools_[2]) Pool (1024, 8, 4, 8, 8, true);
        
        new (&pools_[3]) Pool (4096, 4, 4, 8, 4, true, true);
        
        new (doublePool) Pool (1024, 8, 4, 8, 8, false, false, &pools_[2]);
        
        new (int32Pool) Pool (4096, 4, 4, 8, 4, false, true, &pools_[3]);
        for (int i = 0; i < 4; i++) {
            if (pools_[i].poolData == NULL) {
                m_buffer.fail_oom();
                return;
            }
        }
    }

    static Condition InvertCondition(Condition cond);

    
    void trace(JSTracer *trc);
    void writeRelocation(BufferOffset src) {
        tmpJumpRelocations_.append(src);
    }

    
    
    void writeDataRelocation(const ImmGCPtr &ptr) {
        if (ptr.value)
            tmpDataRelocations_.append(nextOffset());
    }
    void writePrebarrierOffset(CodeOffsetLabel label) {
        tmpPreBarriers_.append(BufferOffset(label.offset()));
    }

    enum RelocBranchStyle {
        B_MOVWT,
        B_LDR_BX,
        B_LDR,
        B_MOVW_ADD
    };

    enum RelocStyle {
        L_MOVWT,
        L_LDR
    };

  public:
    
    
    
    template <class Iter>
    static const uint32_t * getCF32Target(Iter *iter);

    static uintptr_t getPointer(uint8_t *);
    template <class Iter>
    static const uint32_t * getPtr32Target(Iter *iter, Register *dest = NULL, RelocStyle *rs = NULL);

    bool oom() const;

    void setPrinter(Sprinter *sp) {
    }

  private:
    bool isFinished;
  public:
    void finish();
    void executableCopy(void *buffer);
    void processCodeLabels(uint8_t *rawCode);
    void copyJumpRelocationTable(uint8_t *dest);
    void copyDataRelocationTable(uint8_t *dest);
    void copyPreBarrierTable(uint8_t *dest);

    bool addCodeLabel(CodeLabel label);

    
    size_t size() const;
    
    size_t jumpRelocationTableBytes() const;
    size_t dataRelocationTableBytes() const;
    size_t preBarrierTableBytes() const;

    
    size_t bytesNeeded() const;

    
    
    
    
    
    BufferOffset writeInst(uint32_t x, uint32_t *dest = NULL);
    
    
    static void writeInstStatic(uint32_t x, uint32_t *dest);

  public:
    void writeCodePointer(AbsoluteLabel *label);

    BufferOffset align(int alignment);
    BufferOffset as_nop();
    BufferOffset as_alu(Register dest, Register src1, Operand2 op2,
                ALUOp op, SetCond_ sc = NoSetCond, Condition c = Always);

    BufferOffset as_mov(Register dest,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_mvn(Register dest, Operand2 op2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    
    BufferOffset as_and(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_bic(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_eor(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_orr(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    
    BufferOffset as_adc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_add(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_sbc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_sub(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_rsb(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_rsc(Register dest, Register src1,
                Operand2 op2, SetCond_ sc = NoSetCond, Condition c = Always);
    
    BufferOffset as_cmn(Register src1, Operand2 op2,
                Condition c = Always);
    BufferOffset as_cmp(Register src1, Operand2 op2,
                Condition c = Always);
    BufferOffset as_teq(Register src1, Operand2 op2,
                Condition c = Always);
    BufferOffset as_tst(Register src1, Operand2 op2,
                Condition c = Always);

    
    
    
    BufferOffset as_movw(Register dest, Imm16 imm, Condition c = Always, Instruction *pos = NULL);
    BufferOffset as_movt(Register dest, Imm16 imm, Condition c = Always, Instruction *pos = NULL);

    BufferOffset as_genmul(Register d1, Register d2, Register rm, Register rn,
                   MULOp op, SetCond_ sc, Condition c = Always);
    BufferOffset as_mul(Register dest, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_mla(Register dest, Register acc, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_umaal(Register dest1, Register dest2, Register src1, Register src2,
                  Condition c = Always);
    BufferOffset as_mls(Register dest, Register acc, Register src1, Register src2,
                Condition c = Always);
    BufferOffset as_umull(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_umlal(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_smull(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    BufferOffset as_smlal(Register dest1, Register dest2, Register src1, Register src2,
                SetCond_ sc = NoSetCond, Condition c = Always);
    
    
    
    BufferOffset as_dtr(LoadStore ls, int size, Index mode,
                Register rt, DTRAddr addr, Condition c = Always, uint32_t *dest = NULL);
    
    
    
    BufferOffset as_extdtr(LoadStore ls, int size, bool IsSigned, Index mode,
                   Register rt, EDtrAddr addr, Condition c = Always, uint32_t *dest = NULL);

    BufferOffset as_dtm(LoadStore ls, Register rn, uint32_t mask,
                DTMMode mode, DTMWriteBack wb, Condition c = Always);
    
    BufferOffset as_Imm32Pool(Register dest, uint32_t value, ARMBuffer::PoolEntry *pe = NULL, Condition c = Always);
    
    BufferOffset as_BranchPool(uint32_t value, RepatchLabel *label, ARMBuffer::PoolEntry *pe = NULL, Condition c = Always);

    
    BufferOffset as_FImm64Pool(VFPRegister dest, double value, ARMBuffer::PoolEntry *pe = NULL, Condition c = Always);
    

    
    
    BufferOffset as_bx(Register r, Condition c = Always, bool isPatchable = false);

    
    
    
    BufferOffset as_b(BOffImm off, Condition c, bool isPatchable = false);

    BufferOffset as_b(Label *l, Condition c = Always, bool isPatchable = false);
    BufferOffset as_b(BOffImm off, Condition c, BufferOffset inst);

    
    
    
    
    BufferOffset as_blx(Label *l);

    BufferOffset as_blx(Register r, Condition c = Always);
    BufferOffset as_bl(BOffImm off, Condition c);
    
    
    BufferOffset as_bl();
    
    
    BufferOffset as_bl(Label *l, Condition c);
    BufferOffset as_bl(BOffImm off, Condition c, BufferOffset inst);

    
  private:

    enum vfp_size {
        isDouble = 1 << 8,
        isSingle = 0 << 8
    };

    BufferOffset writeVFPInst(vfp_size sz, uint32_t blob, uint32_t *dest=NULL);
    
    
    BufferOffset as_vfp_float(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                      VFPOp op, Condition c = Always);

  public:
    BufferOffset as_vadd(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    BufferOffset as_vdiv(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    BufferOffset as_vmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    BufferOffset as_vnmul(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always);

    BufferOffset as_vnmla(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always);

    BufferOffset as_vnmls(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                  Condition c = Always);

    BufferOffset as_vneg(VFPRegister vd, VFPRegister vm, Condition c = Always);

    BufferOffset as_vsqrt(VFPRegister vd, VFPRegister vm, Condition c = Always);

    BufferOffset as_vabs(VFPRegister vd, VFPRegister vm, Condition c = Always);

    BufferOffset as_vsub(VFPRegister vd, VFPRegister vn, VFPRegister vm,
                 Condition c = Always);

    BufferOffset as_vcmp(VFPRegister vd, VFPRegister vm,
                 Condition c = Always);
    BufferOffset as_vcmpz(VFPRegister vd,  Condition c = Always);

    
    BufferOffset as_vmov(VFPRegister vd, VFPRegister vsrc, Condition c = Always);
    
    enum FloatToCore_ {
        FloatToCore = 1 << 20,
        CoreToFloat = 0 << 20
    };

  private:
    enum VFPXferSize {
        WordTransfer   = 0x02000010,
        DoubleTransfer = 0x00400010
    };

  public:
    
    
    
    
    

    BufferOffset as_vxfer(Register vt1, Register vt2, VFPRegister vm, FloatToCore_ f2c,
                  Condition c = Always, int idx = 0);

    
    
    BufferOffset as_vcvt(VFPRegister vd, VFPRegister vm, bool useFPSCR = false,
                         Condition c = Always);
    
    BufferOffset as_vcvtFixed(VFPRegister vd, bool isSigned, uint32_t fixedPoint, bool toFixed, Condition c = Always);

    
    BufferOffset as_vdtr(LoadStore ls, VFPRegister vd, VFPAddr addr,
                 Condition c = Always ,
                 uint32_t *dest = NULL);

    
    

    BufferOffset as_vdtm(LoadStore st, Register rn, VFPRegister vd, int length,
                 Condition c = Always);

    BufferOffset as_vimm(VFPRegister vd, VFPImm imm, Condition c = Always);

    BufferOffset as_vmrs(Register r, Condition c = Always);
    
    bool nextLink(BufferOffset b, BufferOffset *next);
    void bind(Label *label, BufferOffset boff = BufferOffset());
    void bind(RepatchLabel *label);
    uint32_t currentOffset() {
        return nextOffset().getOffset();
    }
    void retarget(Label *label, Label *target);
    
    void retarget(Label *label, void *target, Relocation::Kind reloc);
    void Bind(uint8_t *rawCode, AbsoluteLabel *label, const void *address);

    void call(Label *label);
    void call(void *target);

    void as_bkpt();

  public:
    static void TraceJumpRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);
    static void TraceDataRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

  protected:
    void addPendingJump(BufferOffset src, void *target, Relocation::Kind kind) {
        enoughMemory_ &= jumps_.append(RelativePatch(src, target, kind));
        if (kind == Relocation::IONCODE)
            writeRelocation(src);
    }

  public:
    
    
    void flush() {
        JS_ASSERT(!isFinished);
        m_buffer.flushPool();
        return;
    }

    
    
    void executableCopy(uint8_t *buffer);

    

    
    
    void startDataTransferM(LoadStore ls, Register rm,
                            DTMMode mode, DTMWriteBack update = NoWriteBack,
                            Condition c = Always)
    {
        JS_ASSERT(!dtmActive);
        dtmUpdate = update;
        dtmBase = rm;
        dtmLoadStore = ls;
        dtmLastReg = -1;
        dtmRegBitField = 0;
        dtmActive = 1;
        dtmCond = c;
        dtmMode = mode;
    }

    void transferReg(Register rn) {
        JS_ASSERT(dtmActive);
        JS_ASSERT(rn.code() > dtmLastReg);
        dtmRegBitField |= 1 << rn.code();
        if (dtmLoadStore == IsLoad && rn.code() == 13 && dtmBase.code() == 13) {
            JS_ASSERT("ARM Spec says this is invalid");
        }
    }
    void finishDataTransfer() {
        dtmActive = false;
        as_dtm(dtmLoadStore, dtmBase, dtmRegBitField, dtmMode, dtmUpdate, dtmCond);
    }

    void startFloatTransferM(LoadStore ls, Register rm,
                             DTMMode mode, DTMWriteBack update = NoWriteBack,
                             Condition c = Always)
    {
        JS_ASSERT(!dtmActive);
        dtmActive = true;
        dtmUpdate = update;
        dtmLoadStore = ls;
        dtmBase = rm;
        dtmCond = c;
        dtmLastReg = -1;
        dtmMode = mode;
    }
    void transferFloatReg(VFPRegister rn)
    {
        if (dtmLastReg == -1) {
            vdtmFirstReg = rn;
        } else {
            JS_ASSERT(dtmLastReg >= 0);
            JS_ASSERT(rn.code() == unsigned(dtmLastReg) + 1);
        }
        dtmLastReg = rn.code();
    }
    void finishFloatTransfer() {
        JS_ASSERT(dtmActive);
        dtmActive = false;
        JS_ASSERT(dtmLastReg != -1);
        
        int len = dtmLastReg - vdtmFirstReg.code() + 1;
        as_vdtm(dtmLoadStore, dtmBase, vdtmFirstReg, len, dtmCond);
    }

  private:
    int dtmRegBitField;
    int dtmLastReg;
    Register dtmBase;
    VFPRegister vdtmFirstReg;
    DTMWriteBack dtmUpdate;
    DTMMode dtmMode;
    LoadStore dtmLoadStore;
    bool dtmActive;
    Condition dtmCond;

  public:
    enum {
        padForAlign8  = (int)0x00,
        padForAlign16 = (int)0x0000,
        padForAlign32 = (int)0xe12fff7f  
    };

    
    
    static void insertTokenIntoTag(uint32_t size, uint8_t *load, int32_t token);
    
    
    static bool patchConstantPoolLoad(void* loadAddr, void* constPoolAddr);
    
    
    
    static uint32_t placeConstantPoolBarrier(int offset);
    

    
    
    
    void dumpPool();
    void flushBuffer();
    void enterNoPool();
    void leaveNoPool();
    
    
    static ptrdiff_t getBranchOffset(const Instruction *i);
    static void retargetNearBranch(Instruction *i, int offset, Condition cond, bool final = true);
    static void retargetNearBranch(Instruction *i, int offset, bool final = true);
    static void retargetFarBranch(Instruction *i, uint8_t **slot, uint8_t *dest, Condition cond);

    static void writePoolHeader(uint8_t *start, Pool *p, bool isNatural);
    static void writePoolFooter(uint8_t *start, Pool *p, bool isNatural);
    static void writePoolGuard(BufferOffset branch, Instruction *inst, BufferOffset dest);


    static uint32_t patchWrite_NearCallSize();
    static uint32_t nopSize() { return 4; }
    static void patchWrite_NearCall(CodeLocationLabel start, CodeLocationLabel toCall);
    static void patchDataWithValueCheck(CodeLocationLabel label, ImmWord newValue,
                                        ImmWord expectedValue);
    static void patchWrite_Imm32(CodeLocationLabel label, Imm32 imm);
    static uint32_t alignDoubleArg(uint32_t offset) {
        return (offset+1)&~1;
    }
    static uint8_t *nextInstruction(uint8_t *instruction, uint32_t *count = NULL);
    

    static void ToggleToJmp(CodeLocationLabel inst_);
    static void ToggleToCmp(CodeLocationLabel inst_);

    static void ToggleCall(CodeLocationLabel inst_, bool enabled);
}; 



class Instruction
{
    uint32_t data;

  protected:
    
    
    Instruction (uint32_t data_, bool fake = false) : data(data_ | 0xf0000000) {
        JS_ASSERT (fake || ((data_ & 0xf0000000) == 0));
    }
    
    Instruction (uint32_t data_, Assembler::Condition c) : data(data_ | (uint32_t) c) {
        JS_ASSERT ((data_ & 0xf0000000) == 0);
    }
    
    
    
  public:
    uint32_t encode() const {
        return data;
    }
    
    template <class C>
    bool is() const { return C::isTHIS(*this); }

    
    template <class C>
    C *as() const { return C::asTHIS(*this); }

    const Instruction & operator=(const Instruction &src) {
        data = src.data;
        return *this;
    }
    
    
    void extractCond(Assembler::Condition *c) {
        if (data >> 28 != 0xf )
            *c = (Assembler::Condition)(data & 0xf0000000);
    }
    
    
    Instruction *next();

    
    
    const uint32_t *raw() const { return &data; }
    uint32_t size() const { return 4; }
}; 


JS_STATIC_ASSERT(sizeof(Instruction) == 4);


class InstDTR : public Instruction
{
  public:
    enum IsByte_ {
        IsByte = 0x00400000,
        IsWord = 0x00000000
    };
    static const int IsDTR     = 0x04000000;
    static const int IsDTRMask = 0x0c000000;

    
    InstDTR(LoadStore ls, IsByte_ ib, Index mode, Register rt, DTRAddr addr, Assembler::Condition c)
      : Instruction(ls | ib | mode | RT(rt) | addr.encode() | IsDTR, c)
    { }

    static bool isTHIS(const Instruction &i);
    static InstDTR *asTHIS(Instruction &i);

};
JS_STATIC_ASSERT(sizeof(InstDTR) == sizeof(Instruction));

class InstLDR : public InstDTR
{
  public:
    InstLDR(Index mode, Register rt, DTRAddr addr, Assembler::Condition c)
        : InstDTR(IsLoad, IsWord, mode, rt, addr, c)
    { }
    static bool isTHIS(const Instruction &i);
    static InstLDR *asTHIS(Instruction &i);

};
JS_STATIC_ASSERT(sizeof(InstDTR) == sizeof(InstLDR));

class InstNOP : public Instruction
{
    static const uint32_t NopInst = 0x0320f000;

  public:
    InstNOP()
      : Instruction(NopInst, Assembler::Always)
    { }

    static bool isTHIS(const Instruction &i);
    static InstNOP *asTHIS(Instruction &i);
};


class InstBranchReg : public Instruction
{
  protected:
    
    enum BranchTag {
        IsBX  = 0x012fff10,
        IsBLX = 0x012fff30
    };
    static const uint32_t IsBRegMask = 0x0ffffff0;
    InstBranchReg(BranchTag tag, Register rm, Assembler::Condition c)
      : Instruction(tag | rm.code(), c)
    { }
  public:
    static bool isTHIS (const Instruction &i);
    static InstBranchReg *asTHIS (const Instruction &i);
    
    void extractDest(Register *dest);
    
    bool checkDest(Register dest);
};
JS_STATIC_ASSERT(sizeof(InstBranchReg) == sizeof(Instruction));


class InstBranchImm : public Instruction
{
  protected:
    enum BranchTag {
        IsB   = 0x0a000000,
        IsBL  = 0x0b000000
    };
    static const uint32_t IsBImmMask = 0x0f000000;

    InstBranchImm(BranchTag tag, BOffImm off, Assembler::Condition c)
      : Instruction(tag | off.encode(), c)
    { }

  public:
    static bool isTHIS (const Instruction &i);
    static InstBranchImm *asTHIS (const Instruction &i);
    void extractImm(BOffImm *dest);
};
JS_STATIC_ASSERT(sizeof(InstBranchImm) == sizeof(Instruction));


class InstBXReg : public InstBranchReg
{
  public:
    static bool isTHIS (const Instruction &i);
    static InstBXReg *asTHIS (const Instruction &i);
};
class InstBLXReg : public InstBranchReg
{
  public:
    InstBLXReg(Register reg, Assembler::Condition c)
      : InstBranchReg(IsBLX, reg, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstBLXReg *asTHIS (const Instruction &i);
};
class InstBImm : public InstBranchImm
{
  public:
    InstBImm(BOffImm off, Assembler::Condition c)
      : InstBranchImm(IsB, off, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstBImm *asTHIS (const Instruction &i);
};
class InstBLImm : public InstBranchImm
{
  public:
    InstBLImm(BOffImm off, Assembler::Condition c)
      : InstBranchImm(IsBL, off, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstBLImm *asTHIS (Instruction &i);
};



class InstMovWT : public Instruction
{
  protected:
    enum WT {
        IsW = 0x03000000,
        IsT = 0x03400000
    };
    static const uint32_t IsWTMask = 0x0ff00000;

    InstMovWT(Register rd, Imm16 imm, WT wt, Assembler::Condition c)
      : Instruction (RD(rd) | imm.encode() | wt, c)
    { }

  public:
    void extractImm(Imm16 *dest);
    void extractDest(Register *dest);
    bool checkImm(Imm16 dest);
    bool checkDest(Register dest);

    static bool isTHIS (Instruction &i);
    static InstMovWT *asTHIS (Instruction &i);

};
JS_STATIC_ASSERT(sizeof(InstMovWT) == sizeof(Instruction));

class InstMovW : public InstMovWT
{
  public:
    InstMovW (Register rd, Imm16 imm, Assembler::Condition c)
      : InstMovWT(rd, imm, IsW, c)
    { }

    static bool isTHIS (const Instruction &i);
    static InstMovW *asTHIS (const Instruction &i);
};

class InstMovT : public InstMovWT
{
  public:
    InstMovT (Register rd, Imm16 imm, Assembler::Condition c)
      : InstMovWT(rd, imm, IsT, c)
    { }
    static bool isTHIS (const Instruction &i);
    static InstMovT *asTHIS (const Instruction &i);
};

class InstALU : public Instruction
{
    static const int32_t ALUMask = 0xc << 24;
  public:
    InstALU (Register rd, Register rn, Operand2 op2, ALUOp op, SetCond_ sc, Assembler::Condition c)
        : Instruction(RD(rd) | RN(rn) | op2.encode() | op | sc | c)
    { }
    static bool isTHIS (const Instruction &i);
    static InstALU *asTHIS (const Instruction &i);
    void extractOp(ALUOp *ret);
    bool checkOp(ALUOp op);
    void extractDest(Register *ret);
    bool checkDest(Register rd);
    void extractOp1(Register *ret);
    bool checkOp1(Register rn);
    void extractOp2(Operand2 *ret);
};
class InstCMP : public InstALU
{
  public:
    static bool isTHIS (const Instruction &i);
    static InstCMP *asTHIS (const Instruction &i);
};


class InstructionIterator {
  private:
    Instruction *i;
  public:
    InstructionIterator(Instruction *i_) : i(i_) {}
    Instruction *next() {
        i = i->next();
        return cur();
    }
    Instruction *cur() const {
        return i;
    }
};

static const uint32_t NumIntArgRegs = 4;
static const uint32_t NumFloatArgRegs = 8;

#ifdef JS_CPU_ARM_HARDFP
static inline bool
GetIntArgReg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
   if (usedIntArgs >= NumIntArgRegs)
        return false;
    *out = Register::FromCode(usedIntArgs);
    return true;
}






static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
    if (GetIntArgReg(usedIntArgs, usedFloatArgs, out))
        return true;
    
    
    
    usedIntArgs -= NumIntArgRegs;
    if (usedIntArgs >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[usedIntArgs];
    return true;
}

static inline bool
GetFloatArgReg(uint32_t usedIntArgs, uint32_t usedFloatArgs, FloatRegister *out)
{
    if (usedFloatArgs >= NumFloatArgRegs)
        return false;
    *out = FloatRegister::FromCode(usedFloatArgs);
    return true;
}

static inline uint32_t
GetIntArgStackDisp(uint32_t usedIntArgs, uint32_t usedFloatArgs, uint32_t *padding)
{
    JS_ASSERT(usedIntArgs >= NumIntArgRegs);
    uint32_t doubleSlots = Max(0, (int32_t)usedFloatArgs - (int32_t)NumFloatArgRegs);
    doubleSlots *= 2;
    int intSlots = usedIntArgs - NumIntArgRegs;
    return (intSlots + doubleSlots + *padding) * STACK_SLOT_SIZE;
}

static inline uint32_t
GetFloatArgStackDisp(uint32_t usedIntArgs, uint32_t usedFloatArgs, uint32_t *padding)
{

    JS_ASSERT(usedFloatArgs >= NumFloatArgRegs);
    uint32_t intSlots = 0;
    if (usedIntArgs > NumIntArgRegs) {
        intSlots = usedIntArgs - NumIntArgRegs;
        
        *padding += (*padding + usedIntArgs) % 2;
    }
    uint32_t doubleSlots = usedFloatArgs - NumFloatArgRegs;
    doubleSlots *= 2;
    return (intSlots + doubleSlots + *padding) * STACK_SLOT_SIZE;
}
#else
static inline bool
GetIntArgReg(uint32_t arg, uint32_t floatArg, Register *out)
{
    if (arg < NumIntArgRegs) {
        *out = Register::FromCode(arg);
        return true;
    }
    return false;
}






static inline bool
GetTempRegForIntArg(uint32_t usedIntArgs, uint32_t usedFloatArgs, Register *out)
{
    if (GetIntArgReg(usedIntArgs, usedFloatArgs, out))
        return true;
    
    
    
    usedIntArgs -= NumIntArgRegs;
    if (usedIntArgs >= NumCallTempNonArgRegs)
        return false;
    *out = CallTempNonArgRegs[usedIntArgs];
    return true;
}

static inline uint32_t
GetArgStackDisp(uint32_t arg)
{
    JS_ASSERT(arg >= NumIntArgRegs);
    return (arg - NumIntArgRegs) * STACK_SLOT_SIZE;
}

#endif
class DoubleEncoder {
    uint32_t rep(bool b, uint32_t count) {
        uint32_t ret = 0;
        for (uint32_t i = 0; i < count; i++)
            ret = (ret << 1) | b;
        return ret;
    }
    uint32_t encode(uint8_t value) {
        
        
        
        
        bool a = value >> 7;
        bool b = value >> 6 & 1;
        bool B = !b;
        uint32_t cdefgh = value & 0x3f;
        return a << 31 |
            B << 30 |
            rep(b, 8) << 22 |
            cdefgh << 16;
    }

    struct DoubleEntry
    {
        uint32_t dblTop;
        datastore::Imm8VFPImmData data;

        DoubleEntry()
          : dblTop(-1)
        { }
        DoubleEntry(uint32_t dblTop_, datastore::Imm8VFPImmData data_)
          : dblTop(dblTop_), data(data_)
        { }
    };
    DoubleEntry table [256];

    
    static DoubleEncoder _this;
    DoubleEncoder()
    {
        for (int i = 0; i < 256; i++) {
            table[i] = DoubleEntry(encode(i), datastore::Imm8VFPImmData(i));
        }
    }

  public:
    static bool lookup(uint32_t top, datastore::Imm8VFPImmData *ret) {
        for (int i = 0; i < 256; i++) {
            if (_this.table[i].dblTop == top) {
                *ret = _this.table[i].data;
                return true;
            }
        }
        return false;
    }
};

class AutoForbidPools {
    Assembler *masm_;
  public:
    AutoForbidPools(Assembler *masm) : masm_(masm) {
        masm_->enterNoPool();
    }
    ~AutoForbidPools() {
        masm_->leaveNoPool();
    }
};

} 
} 

#endif
