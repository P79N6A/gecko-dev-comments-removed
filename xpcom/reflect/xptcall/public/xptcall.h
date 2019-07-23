







































#ifndef xptcall_h___
#define xptcall_h___

#include "prtypes.h"
#include "nscore.h"
#include "nsISupports.h"
#include "xpt_struct.h"
#include "xptinfo.h"

struct nsXPTCMiniVariant
{


    union
    {
        PRInt8    i8;
        PRInt16   i16;
        PRInt32   i32;
        PRInt64   i64;
        PRUint8   u8;
        PRUint16  u16;
        PRUint32  u32;
        PRUint64  u64;
        float     f;
        double    d;
        PRBool    b;
        char      c;
        PRUnichar wc;
        void*     p;
    } val;
};

struct nsXPTCVariant : public nsXPTCMiniVariant
{



    
    void*     ptr;
    nsXPTType type;
    PRUint8   flags;

    enum
    {
        
        PTR_IS_DATA    = 0x1,  
        VAL_IS_ALLOCD  = 0x2,  
        VAL_IS_IFACE   = 0x4,  
        VAL_IS_ARRAY   = 0x8,  
        VAL_IS_DOMSTR  = 0x10, 
        VAL_IS_UTF8STR = 0x20, 
        VAL_IS_CSTR    = 0x40  
    };

    void ClearFlags()         {flags = 0;}
    void SetPtrIsData()       {flags |= PTR_IS_DATA;}
    void SetValIsAllocated()  {flags |= VAL_IS_ALLOCD;}
    void SetValIsInterface()  {flags |= VAL_IS_IFACE;}
    void SetValIsArray()      {flags |= VAL_IS_ARRAY;}
    void SetValIsDOMString()  {flags |= VAL_IS_DOMSTR;}
    void SetValIsUTF8String() {flags |= VAL_IS_UTF8STR;}
    void SetValIsCString()    {flags |= VAL_IS_CSTR;}    

    PRBool IsPtrData()       const  {return 0 != (flags & PTR_IS_DATA);}
    PRBool IsValAllocated()  const  {return 0 != (flags & VAL_IS_ALLOCD);}
    PRBool IsValInterface()  const  {return 0 != (flags & VAL_IS_IFACE);}
    PRBool IsValArray()      const  {return 0 != (flags & VAL_IS_ARRAY);}
    PRBool IsValDOMString()  const  {return 0 != (flags & VAL_IS_DOMSTR);}
    PRBool IsValUTF8String() const  {return 0 != (flags & VAL_IS_UTF8STR);}
    PRBool IsValCString()    const  {return 0 != (flags & VAL_IS_CSTR);}    

    void Init(const nsXPTCMiniVariant& mv, const nsXPTType& t, PRUint8 f)
    {
        type = t;
        flags = f;

        if(f & PTR_IS_DATA)
        {
            ptr = mv.val.p;
            val.p = nsnull;
        }
        else
        {
            ptr = nsnull;
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
    NS_IMETHOD CallMethod(PRUint16 aMethodIndex,
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
NS_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                 PRUint32 paramCount, nsXPTCVariant* params);

#endif 
