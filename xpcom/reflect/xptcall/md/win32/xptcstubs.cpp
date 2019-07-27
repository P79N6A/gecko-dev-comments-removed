






#include "xptcprivate.h"
#include "xptiprivate.h"

#ifndef WIN32
#error "This code is for Win32 only"
#endif

extern "C" {

#if !defined(__GNUC__) && !defined(__clang__)
static
#endif
nsresult __stdcall
PrepareAndDispatch(nsXPTCStubBase* self, uint32_t methodIndex,
                   uint32_t* args, uint32_t* stackBytesToPop)
{
#define PARAM_BUFFER_COUNT     16

    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = nullptr;
    const nsXPTMethodInfo* info = nullptr;
    uint8_t paramCount;
    uint8_t i;
    nsresult result = NS_ERROR_FAILURE;

    
    

    NS_ASSERTION(self,"no self");

    self->mEntry->GetMethodInfo(uint16_t(methodIndex), &info);
    NS_ASSERTION(info,"no method info");

    paramCount = info->GetParamCount();

    
    if(paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;
    NS_ASSERTION(dispatchParams,"no place for params");

    uint32_t* ap = args;
    for(i = 0; i < paramCount; i++, ap++)
    {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];

        if(param.IsOut() || !type.IsArithmetic())
        {
            dp->val.p = (void*) *ap;
            continue;
        }
        
        switch(type)
        {
        case nsXPTType::T_I8     : dp->val.i8  = *((int8_t*)  ap);       break;
        case nsXPTType::T_I16    : dp->val.i16 = *((int16_t*) ap);       break;
        case nsXPTType::T_I32    : dp->val.i32 = *((int32_t*) ap);       break;
        case nsXPTType::T_I64    : dp->val.i64 = *((int64_t*) ap); ap++; break;
        case nsXPTType::T_U8     : dp->val.u8  = *((uint8_t*) ap);       break;
        case nsXPTType::T_U16    : dp->val.u16 = *((uint16_t*)ap);       break;
        case nsXPTType::T_U32    : dp->val.u32 = *((uint32_t*)ap);       break;
        case nsXPTType::T_U64    : dp->val.u64 = *((uint64_t*)ap); ap++; break;
        case nsXPTType::T_FLOAT  : dp->val.f   = *((float*)   ap);       break;
        case nsXPTType::T_DOUBLE : dp->val.d   = *((double*)  ap); ap++; break;
        case nsXPTType::T_BOOL   : dp->val.b   = *((bool*)  ap);       break;
        case nsXPTType::T_CHAR   : dp->val.c   = *((char*)    ap);       break;
        case nsXPTType::T_WCHAR  : dp->val.wc  = *((wchar_t*) ap);       break;
        default:
            NS_ERROR("bad type");
            break;
        }
    }
    *stackBytesToPop = ((uint32_t)ap) - ((uint32_t)args);

    result = self->mOuter->CallMethod((uint16_t)methodIndex, info, dispatchParams);

    if(dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

} 


#if !defined(__GNUC__) && !defined(__clang__)
static 
__declspec(naked)
void SharedStub(void)
{
    __asm {
        push ebp            
        mov  ebp, esp       
        push ecx            
        lea  eax, [ebp-4]   
        push eax
        lea  eax, [ebp+12]  
        push eax
        push ecx            
        mov  eax, [ebp+8]   
        push eax
        call PrepareAndDispatch
        mov  edx, [ebp+4]   
        mov  ecx, [ebp-4]   
        add  ecx, 8         
        mov  esp, ebp
        pop  ebp
        add  esp, ecx       
        jmp  edx            
    }
}


#define STUB_ENTRY(n) \
__declspec(naked) nsresult __stdcall nsXPTCStubBase::Stub##n() \
{ __asm mov ecx, n __asm jmp SharedStub }

#else

asm(".text\n\t"
    ".align     4\n\t"
    "SharedStub:\n\t"
    "push       %ebp\n\t"
    "mov        %esp, %ebp\n\t"
    "push       %ecx\n\t"
    "lea        -4(%ebp), %eax\n\t"
    "push       %eax\n\t"
    "lea        12(%ebp), %eax\n\t"
    "push       %eax\n\t"
    "push       %ecx\n\t"
    "movl       8(%ebp), %eax\n\t"
    "push       %eax\n\t"
    "call       _PrepareAndDispatch@16\n\t"
    "mov        4(%ebp), %edx\n\t"
    "mov        -4(%ebp), %ecx\n\t"
    "add        $8, %ecx\n\t"
    "mov        %ebp, %esp\n\t"
    "pop        %ebp\n\t"
    "add        %ecx, %esp\n\t"
    "jmp        *%edx"
);




#ifdef __clang__

#define STUB_ENTRY(n) \
asm(".text\n\t" \
    ".align     4\n\t" \
    ".globl     \"?Stub" #n "@nsXPTCStubBase@@UAG?AW4nsresult@@XZ\"\n\t" \
    ".def       \"?Stub" #n "@nsXPTCStubBase@@UAG?AW4nsresult@@XZ\"; \n\t" \
    ".scl       2\n\t" \
    ".type      46\n\t" \
    ".endef\n\t" \
    "\"?Stub" #n "@nsXPTCStubBase@@UAG?AW4nsresult@@XZ\":\n\t" \
    "mov $" #n ", %ecx\n\t" \
    "jmp SharedStub");

#else

#define STUB_ENTRY(n) \
asm(".text\n\t" \
    ".align     4\n\t" \
    ".if	" #n " < 10\n\t" \
    ".globl     __ZN14nsXPTCStubBase5Stub" #n "Ev@4\n\t" \
    ".def       __ZN14nsXPTCStubBase5Stub" #n "Ev@4; \n\t" \
    ".scl       3\n\t" \
    ".type      46\n\t" \
    ".endef\n\t" \
    "__ZN14nsXPTCStubBase5Stub" #n "Ev@4:\n\t" \
    ".elseif	" #n " < 100\n\t" \
    ".globl     __ZN14nsXPTCStubBase6Stub" #n "Ev@4\n\t" \
    ".def       __ZN14nsXPTCStubBase6Stub" #n "Ev@4\n\t" \
    ".scl       3\n\t" \
    ".type      46\n\t" \
    ".endef\n\t" \
    "__ZN14nsXPTCStubBase6Stub" #n "Ev@4:\n\t" \
    ".elseif    " #n " < 1000\n\t" \
    ".globl     __ZN14nsXPTCStubBase7Stub" #n "Ev@4\n\t" \
    ".def       __ZN14nsXPTCStubBase7Stub" #n "Ev@4\n\t" \
    ".scl       3\n\t" \
    ".type      46\n\t" \
    ".endef\n\t" \
    "__ZN14nsXPTCStubBase7Stub" #n "Ev@4:\n\t" \
    ".else\n\t" \
    ".err	\"stub number " #n " >= 1000 not yet supported\"\n\t" \
    ".endif\n\t" \
    "mov $" #n ", %ecx\n\t" \
    "jmp SharedStub");

#endif

#endif

#define SENTINEL_ENTRY(n) \
nsresult __stdcall nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ERROR("nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#ifdef _MSC_VER
#pragma warning(disable : 4035) // OK to have no return value
#endif
#include "xptcstubsdef.inc"
#ifdef _MSC_VER
#pragma warning(default : 4035) // restore default
#endif

void
#ifdef __GNUC__
__cdecl
#endif
xptc_dummy()
{
}
