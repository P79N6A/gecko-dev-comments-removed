






#ifndef xptiinfo_h___
#define xptiinfo_h___

#include "nscore.h"
#include "xpt_struct.h"





class nsXPTType : public XPTTypeDescriptorPrefix
{

public:
    nsXPTType()
        {}    
    MOZ_IMPLICIT nsXPTType(const XPTTypeDescriptorPrefix& prefix)
        {*(XPTTypeDescriptorPrefix*)this = prefix;}

    MOZ_IMPLICIT nsXPTType(const uint8_t& prefix)
        {*(uint8_t*)this = prefix;}

    nsXPTType& operator=(uint8_t val)
        {flags = val; return *this;}

    nsXPTType& operator=(const nsXPTType& other)
        {flags = other.flags; return *this;}

    operator uint8_t() const
        {return flags;}

    
    
    
    
    
    
    
    bool IsArithmetic() const
        {return flags <= T_WCHAR;}

    
    
    
    
    
    bool deprecated_IsPointer() const
        {return !IsArithmetic() && TagPart() != T_JSVAL;}

    bool IsInterfacePointer() const
        {  switch (TagPart()) {
             default:
               return false;
             case T_INTERFACE:
             case T_INTERFACE_IS:
               return true;
           }
        }

    bool IsArray() const
        {return TagPart() == T_ARRAY;}

    
    
    
    bool IsDependent() const
        {  switch (TagPart()) {
             default:
               return false;
             case T_INTERFACE_IS:
             case TD_ARRAY:
             case T_PSTRING_SIZE_IS:
             case T_PWSTRING_SIZE_IS:
               return true;
           }
        }

    uint8_t TagPart() const
        {return (uint8_t) (flags & XPT_TDP_TAGMASK);}

    enum
    {
        T_I8                = TD_INT8             ,
        T_I16               = TD_INT16            ,
        T_I32               = TD_INT32            ,
        T_I64               = TD_INT64            ,
        T_U8                = TD_UINT8            ,
        T_U16               = TD_UINT16           ,
        T_U32               = TD_UINT32           ,
        T_U64               = TD_UINT64           ,
        T_FLOAT             = TD_FLOAT            ,
        T_DOUBLE            = TD_DOUBLE           ,
        T_BOOL              = TD_BOOL             ,
        T_CHAR              = TD_CHAR             ,
        T_WCHAR             = TD_WCHAR            ,
        T_VOID              = TD_VOID             ,
        T_IID               = TD_PNSIID           ,
        T_DOMSTRING         = TD_DOMSTRING        ,
        T_CHAR_STR          = TD_PSTRING          ,
        T_WCHAR_STR         = TD_PWSTRING         ,
        T_INTERFACE         = TD_INTERFACE_TYPE   ,
        T_INTERFACE_IS      = TD_INTERFACE_IS_TYPE,
        T_ARRAY             = TD_ARRAY            ,
        T_PSTRING_SIZE_IS   = TD_PSTRING_SIZE_IS  ,
        T_PWSTRING_SIZE_IS  = TD_PWSTRING_SIZE_IS ,
        T_UTF8STRING        = TD_UTF8STRING       ,
        T_CSTRING           = TD_CSTRING          ,
        T_ASTRING           = TD_ASTRING          ,
        T_JSVAL             = TD_JSVAL
    };

};

class nsXPTParamInfo : public XPTParamDescriptor
{

public:
    MOZ_IMPLICIT nsXPTParamInfo(const XPTParamDescriptor& desc)
        {*(XPTParamDescriptor*)this = desc;}


    bool IsIn()  const    {return 0 != (XPT_PD_IS_IN(flags));}
    bool IsOut() const    {return 0 != (XPT_PD_IS_OUT(flags));}
    bool IsRetval() const {return 0 != (XPT_PD_IS_RETVAL(flags));}
    bool IsShared() const {return 0 != (XPT_PD_IS_SHARED(flags));}

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool IsDipper() const {return 0 != (XPT_PD_IS_DIPPER(flags));}
    bool IsOptional() const {return 0 != (XPT_PD_IS_OPTIONAL(flags));}
    const nsXPTType GetType() const {return type.prefix;}

    bool IsStringClass() const {
      switch (GetType().TagPart()) {
        case nsXPTType::T_ASTRING:
        case nsXPTType::T_DOMSTRING:
        case nsXPTType::T_UTF8STRING:
        case nsXPTType::T_CSTRING:
          return true;
        default:
          return false;
      }
    }

    
    
    
    bool IsIndirect() const {return IsOut() ||
                               GetType().TagPart() == nsXPTType::T_JSVAL;}

    

private:
    nsXPTParamInfo();   

};

class nsXPTMethodInfo : public XPTMethodDescriptor
{

public:
    MOZ_IMPLICIT nsXPTMethodInfo(const XPTMethodDescriptor& desc)
        {*(XPTMethodDescriptor*)this = desc;}

    bool IsGetter()      const {return 0 != (XPT_MD_IS_GETTER(flags) );}
    bool IsSetter()      const {return 0 != (XPT_MD_IS_SETTER(flags) );}
    bool IsNotXPCOM()    const {return 0 != (XPT_MD_IS_NOTXPCOM(flags));}
    bool IsConstructor() const {return 0 != (XPT_MD_IS_CTOR(flags)   );}
    bool IsHidden()      const {return 0 != (XPT_MD_IS_HIDDEN(flags) );}
    bool WantsOptArgc()  const {return 0 != (XPT_MD_WANTS_OPT_ARGC(flags));}
    bool WantsContext()  const {return 0 != (XPT_MD_WANTS_CONTEXT(flags));}
    const char* GetName()  const {return name;}
    uint8_t GetParamCount()  const {return num_args;}
    
    const nsXPTParamInfo GetParam(uint8_t idx) const
        {
            NS_PRECONDITION(idx < GetParamCount(),"bad arg");
            return params[idx];
        }
    const nsXPTParamInfo GetResult() const
        {return result;}
private:
    nsXPTMethodInfo();  

};



struct nsXPTCMiniVariant;

class nsXPTConstant : public XPTConstDescriptor
{

public:
    MOZ_IMPLICIT nsXPTConstant(const XPTConstDescriptor& desc)
        {*(XPTConstDescriptor*)this = desc;}

    const char* GetName() const
        {return name;}

    const nsXPTType GetType() const
        {return type.prefix;}

    
    
    
    
    const nsXPTCMiniVariant* GetValue() const
        {return (nsXPTCMiniVariant*) &value;}
private:
    nsXPTConstant();    

};

#endif 
