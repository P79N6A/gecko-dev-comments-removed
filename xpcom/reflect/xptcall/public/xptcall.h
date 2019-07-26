






#ifndef xptcall_h___
#define xptcall_h___

#include "nscore.h"
#include "nsISupports.h"
#include "xpt_struct.h"
#include "xptinfo.h"
#include "jsapi.h"

struct nsXPTCMiniVariant
{


    union
    {
        int8_t    i8;
        int16_t   i16;
        int32_t   i32;
        int64_t   i64;
        uint8_t   u8;
        uint16_t  u16;
        uint32_t  u32;
        uint64_t  u64;
        float     f;
        double    d;
        bool      b;
        char      c;
        PRUnichar wc;
        void*     p;

        
        
        
        jsval j;
    } val;
};

struct nsXPTCVariant : public nsXPTCMiniVariant
{



    
    void*     ptr;
    nsXPTType type;
    uint8_t   flags;

    enum
    {
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        PTR_IS_DATA    = 0x1,

        
        
        
        
        
        
        VAL_NEEDS_CLEANUP = 0x2
    };

    void ClearFlags()         {flags = 0;}
    void SetIndirect()        {ptr = &val; flags |= PTR_IS_DATA;}
    void SetValNeedsCleanup() {flags |= VAL_NEEDS_CLEANUP;}

    bool IsIndirect()         const  {return 0 != (flags & PTR_IS_DATA);}
    bool DoesValNeedCleanup() const  {return 0 != (flags & VAL_NEEDS_CLEANUP);}

    
    bool IsPtrData()       const  {return 0 != (flags & PTR_IS_DATA);}

    void Init(const nsXPTCMiniVariant& mv, const nsXPTType& t, uint8_t f)
    {
        type = t;
        flags = f;

        if(f & PTR_IS_DATA)
        {
            ptr = mv.val.p;
            val.p = nullptr;
        }
        else
        {
            ptr = nullptr;
            val.p = nullptr; 
            switch(t.TagPart()) {
              case nsXPTType::T_I8:                val.i8  = mv.val.i8;  break;
              case nsXPTType::T_I16:               val.i16 = mv.val.i16; break;
              case nsXPTType::T_I32:               val.i32 = mv.val.i32; break;
              case nsXPTType::T_I64:               val.i64 = mv.val.i64; break;
              case nsXPTType::T_U8:                val.u8  = mv.val.u8;  break;
              case nsXPTType::T_U16:               val.u16 = mv.val.u16; break;
              case nsXPTType::T_U32:               val.u32 = mv.val.u32; break;
              case nsXPTType::T_U64:               val.u64 = mv.val.u64; break;
              case nsXPTType::T_FLOAT:             val.f   = mv.val.f;   break;
              case nsXPTType::T_DOUBLE:            val.d   = mv.val.d;   break;
              case nsXPTType::T_BOOL:              val.b   = mv.val.b;   break;
              case nsXPTType::T_CHAR:              val.c   = mv.val.c;   break;
              case nsXPTType::T_WCHAR:             val.wc  = mv.val.wc;  break;
              case nsXPTType::T_VOID:              
              case nsXPTType::T_IID:               
              case nsXPTType::T_DOMSTRING:         
              case nsXPTType::T_CHAR_STR:          
              case nsXPTType::T_WCHAR_STR:         
              case nsXPTType::T_INTERFACE:         
              case nsXPTType::T_INTERFACE_IS:      
              case nsXPTType::T_ARRAY:             
              case nsXPTType::T_PSTRING_SIZE_IS:   
              case nsXPTType::T_PWSTRING_SIZE_IS:  
              case nsXPTType::T_UTF8STRING:        
              case nsXPTType::T_CSTRING:                         
              default:                             val.p   = mv.val.p;   break;
            }
        }
    }
};

class nsIXPTCProxy : public nsISupports
{
public:
    NS_IMETHOD CallMethod(uint16_t aMethodIndex,
                          const XPTMethodDescriptor *aInfo,
                          nsXPTCMiniVariant *aParams) = 0;
};






typedef nsISupports nsISomeInterface;
















XPCOM_API(nsresult)
NS_GetXPTCallStub(REFNSIID aIID, nsIXPTCProxy* aOuter,
                  nsISomeInterface* *aStub);




XPCOM_API(void)
NS_DestroyXPTCallStub(nsISomeInterface* aStub);

XPCOM_API(nsresult)
NS_InvokeByIndex(nsISupports* that, uint32_t methodIndex,
                 uint32_t paramCount, nsXPTCVariant* params);

#endif 
