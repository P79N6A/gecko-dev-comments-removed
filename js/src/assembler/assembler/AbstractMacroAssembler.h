




























#ifndef AbstractMacroAssembler_h
#define AbstractMacroAssembler_h

#include "assembler/wtf/Platform.h"
#include "assembler/assembler/MacroAssemblerCodeRef.h"
#include "assembler/assembler/CodeLocation.h"
#include "jsstdint.h"

#if ENABLE_ASSEMBLER

namespace JSC {

class LinkBuffer;
class RepatchBuffer;

template <class AssemblerType>
class AbstractMacroAssembler {
public:
    typedef AssemblerType AssemblerType_T;

    typedef MacroAssemblerCodePtr CodePtr;
    typedef MacroAssemblerCodeRef CodeRef;

    class Jump;

    typedef typename AssemblerType::RegisterID RegisterID;
    typedef typename AssemblerType::FPRegisterID FPRegisterID;
    typedef typename AssemblerType::JmpSrc JmpSrc;
    typedef typename AssemblerType::JmpDst JmpDst;

#ifdef DEBUG
    void setSpewPath(bool isOOLPath)
    {
        m_assembler.isOOLPath = isOOLPath;
    }
#endif

    
    
    
    


    enum Scale {
        TimesOne,
        TimesTwo,
        TimesFour,
        TimesEight
    };

    
    
    
    struct Address {
        explicit Address() {}

        explicit Address(RegisterID base, int32_t offset = 0)
            : base(base)
            , offset(offset)
        {
        }

        RegisterID base;
        int32_t offset;
    };

    struct ExtendedAddress {
        explicit ExtendedAddress(RegisterID base, intptr_t offset = 0)
            : base(base)
            , offset(offset)
        {
        }
        
        RegisterID base;
        intptr_t offset;
    };

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    struct ImplicitAddress {
        ImplicitAddress(RegisterID base)
            : base(base)
            , offset(0)
        {
        }

        ImplicitAddress(Address address)
            : base(address.base)
            , offset(address.offset)
        {
        }

        RegisterID base;
        int32_t offset;
    };

    
    
    
    struct BaseIndex {
        BaseIndex(RegisterID base, RegisterID index, Scale scale, int32_t offset = 0)
            : base(base)
            , index(index)
            , scale(scale)
            , offset(offset)
        {
        }

        RegisterID base;
        RegisterID index;
        Scale scale;
        int32_t offset;
    };

    
    
    
    
    struct AbsoluteAddress {
        explicit AbsoluteAddress(void* ptr)
            : m_ptr(ptr)
        {
        }

        void* m_ptr;
    };

    
    
    
    
    
    struct TrustedImmPtr {
        explicit TrustedImmPtr(const void* value)
            : m_value(value)
        {
        }

        intptr_t asIntptr()
        {
            return reinterpret_cast<intptr_t>(m_value);
        }

        const void* m_value;
    };

    struct ImmPtr : public TrustedImmPtr {
        explicit ImmPtr(const void* value)
            : TrustedImmPtr(value)
        {
        }
    };
 
    
    
    
    
    
    
    struct TrustedImm32 {
        explicit TrustedImm32(int32_t value)
            : m_value(value)
#if WTF_CPU_ARM || WTF_CPU_MIPS
            , m_isPointer(false)
#endif
        {
        }

#if !WTF_CPU_X86_64
        explicit TrustedImm32(TrustedImmPtr ptr)
            : m_value(ptr.asIntptr())
#if WTF_CPU_ARM || WTF_CPU_MIPS
            , m_isPointer(true)
#endif
        {
        }
#endif

        int32_t m_value;
#if WTF_CPU_ARM || WTF_CPU_MIPS
        
        
        
        
        
        
        
        bool m_isPointer;
#endif
    };


    struct Imm32 : public TrustedImm32 {
        explicit Imm32(int32_t value)
            : TrustedImm32(value)
        {
        }
#if !WTF_CPU_X86_64
        explicit Imm32(TrustedImmPtr ptr)
            : TrustedImm32(ptr)
        {
        }
#endif
    };

    struct ImmDouble {
        union {
            struct {
#if WTF_CPU_BIG_ENDIAN || WTF_CPU_MIDDLE_ENDIAN
                uint32 msb, lsb;
#else
                uint32 lsb, msb;
#endif
            } s;
            uint64_t u64;
            double d;
        } u;

        explicit ImmDouble(double d) {
            u.d = d;
        }
    };

    
    
    
    
    
    


    
    
    
    
    class Label {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class Jump;
        friend class MacroAssemblerCodeRef;
        friend class LinkBuffer;

    public:
        Label()
        {
        }

        Label(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }
        
        bool isUsed() const { return m_label.isUsed(); }
        void used() { m_label.used(); }
        bool isSet() const { return m_label.isValid(); }
    private:
        JmpDst m_label;
    };

    
    
    
    
    class DataLabelPtr {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabelPtr()
        {
        }

        DataLabelPtr(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }
        
        bool isSet() const { return m_label.isValid(); }

    private:
        JmpDst m_label;
    };

    
    
    
    
    class DataLabel32 {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabel32()
        {
        }

        DataLabel32(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }

    private:
        JmpDst m_label;
    };

    
    
    
    
    
    
    class Call {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;

    public:
        enum Flags {
            None = 0x0,
            Linkable = 0x1,
            Near = 0x2,
            LinkableNear = 0x3
        };

        Call()
            : m_flags(None)
        {
        }
        
        Call(JmpSrc jmp, Flags flags)
            : m_jmp(jmp)
            , m_flags(flags)
        {
        }

        bool isFlagSet(Flags flag)
        {
            return !!(m_flags & flag);
        }

        static Call fromTailJump(Jump jump)
        {
            return Call(jump.m_jmp, Linkable);
        }

        JmpSrc m_jmp;
    private:
        Flags m_flags;
    };

    
    
    
    
    
    
    class Jump {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class Call;
        friend class LinkBuffer;
    public:
        Jump()
        {
        }
        
        Jump(JmpSrc jmp)    
            : m_jmp(jmp)
        {
        }
        
        void link(AbstractMacroAssembler<AssemblerType>* masm)
        {
            masm->m_assembler.linkJump(m_jmp, masm->m_assembler.label());
        }
        
        void linkTo(Label label, AbstractMacroAssembler<AssemblerType>* masm)
        {
            masm->m_assembler.linkJump(m_jmp, label.m_label);
        }

    private:
        JmpSrc m_jmp;
    };

    
    
    
    
    class JumpList {
        friend class LinkBuffer;

    public:
        typedef js::Vector<Jump, 16 ,js::SystemAllocPolicy > JumpVector;

        JumpList() {}

        JumpList(const JumpList &other)
        {
            m_jumps.append(other.m_jumps);
        }

        JumpList &operator=(const JumpList &other)
        {
            m_jumps.clear();
            m_jumps.append(other.m_jumps);
            return *this;
        }

        void link(AbstractMacroAssembler<AssemblerType>* masm)
        {
            size_t size = m_jumps.length();
            for (size_t i = 0; i < size; ++i)
                m_jumps[i].link(masm);
            m_jumps.clear();
        }
        
        void linkTo(Label label, AbstractMacroAssembler<AssemblerType>* masm)
        {
            size_t size = m_jumps.length();
            for (size_t i = 0; i < size; ++i)
                m_jumps[i].linkTo(label, masm);
            m_jumps.clear();
        }
        
        void append(Jump jump)
        {
            m_jumps.append(jump);
        }
        
        void append(const JumpList& other)
        {
            m_jumps.append(other.m_jumps.begin(), other.m_jumps.length());
        }

        void clear()
        {
            m_jumps.clear();
        }

        bool empty()
        {
            return !m_jumps.length();
        }
        
        const JumpVector& jumps() const { return m_jumps; }

    private:
        JumpVector m_jumps;
    };


    

    static CodePtr trampolineAt(CodeRef ref, Label label)
    {
        return CodePtr(AssemblerType::getRelocatedAddress(ref.m_code.dataLocation(), label.m_label));
    }

    size_t size()
    {
        return m_assembler.size();
    }

    unsigned char *buffer()
    {
        return m_assembler.buffer();
    }

    bool oom()
    {
        return m_assembler.oom();
    }

    void executableCopy(void* buffer)
    {
        ASSERT(!oom());
        m_assembler.executableCopy(buffer);
    }

    Label label()
    {
        return Label(this);
    }

    DataLabel32 dataLabel32()
    {
        return DataLabel32(this);
    }
    
    Label align()
    {
        m_assembler.align(16);
        return Label(this);
    }

    ptrdiff_t differenceBetween(Label from, Jump to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    ptrdiff_t differenceBetween(Label from, Call to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    ptrdiff_t differenceBetween(Label from, Label to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(Label from, DataLabelPtr to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(Label from, DataLabel32 to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(DataLabel32 from, Label to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(DataLabelPtr from, Jump to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    ptrdiff_t differenceBetween(DataLabelPtr from, DataLabelPtr to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(DataLabelPtr from, Call to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

protected:
    AssemblerType m_assembler;

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkJump(void* code, Jump jump, CodeLocationLabel target)
    {
        AssemblerType::linkJump(code, jump.m_jmp, target.dataLocation());
    }

    static void linkPointer(void* code, typename AssemblerType::JmpDst label, void* value)
    {
        AssemblerType::linkPointer(code, label, value);
    }

    static void* getLinkerAddress(void* code, typename AssemblerType::JmpSrc label)
    {
        return AssemblerType::getRelocatedAddress(code, label);
    }

    static void* getLinkerAddress(void* code, typename AssemblerType::JmpDst label)
    {
        return AssemblerType::getRelocatedAddress(code, label);
    }

    static unsigned getLinkerCallReturnOffset(Call call)
    {
        return AssemblerType::getCallReturnOffset(call.m_jmp);
    }

    static void repatchJump(CodeLocationJump jump, CodeLocationLabel destination)
    {
        AssemblerType::relinkJump(jump.dataLocation(), destination.dataLocation());
    }

    static bool canRepatchJump(CodeLocationJump jump, CodeLocationLabel destination)
    {
        return AssemblerType::canRelinkJump(jump.dataLocation(), destination.dataLocation());
    }

    static void repatchNearCall(CodeLocationNearCall nearCall, CodeLocationLabel destination)
    {
        AssemblerType::relinkCall(nearCall.dataLocation(), destination.executableAddress());
    }

    static void repatchInt32(CodeLocationDataLabel32 dataLabel32, int32_t value)
    {
        AssemblerType::repatchInt32(dataLabel32.dataLocation(), value);
    }

    static void repatchPointer(CodeLocationDataLabelPtr dataLabelPtr, void* value)
    {
        AssemblerType::repatchPointer(dataLabelPtr.dataLocation(), value);
    }

    static void repatchLoadPtrToLEA(CodeLocationInstruction instruction)
    {
        AssemblerType::repatchLoadPtrToLEA(instruction.dataLocation());
    }

    static void repatchLEAToLoadPtr(CodeLocationInstruction instruction)
    {
        AssemblerType::repatchLEAToLoadPtr(instruction.dataLocation());
    }
};

} 

#endif 

#endif 
