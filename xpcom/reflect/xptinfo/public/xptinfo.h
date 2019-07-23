






































#ifndef xptiinfo_h___
#define xptiinfo_h___

#include "prtypes.h"
#include "nscore.h"
#include "xpt_struct.h"

class nsIInterfaceInfoManager;





class nsXPTType : public XPTTypeDescriptorPrefix
{

public:
    nsXPTType()
        {}    
    nsXPTType(const XPTTypeDescriptorPrefix& prefix)
        {*(XPTTypeDescriptorPrefix*)this = prefix;}

    nsXPTType(const PRUint8& prefix)
        {*(PRUint8*)this = prefix;}

    nsXPTType& operator=(PRUint8 val)
        {flags = val; return *this;}

    nsXPTType& operator=(const nsXPTType& other)
        {flags = other.flags; return *this;}

    operator PRUint8() const
        {return flags;}

    PRBool IsPointer() const
        {return 0 != (XPT_TDP_IS_POINTER(flags));}

    PRBool IsUniquePointer() const
        {return 0 != (XPT_TDP_IS_UNIQUE_POINTER(flags));}

    PRBool IsReference() const
        {return 0 != (XPT_TDP_IS_REFERENCE(flags));}

    PRBool IsArithmetic() const     
        {return flags <= T_WCHAR;}

    PRBool IsInterfacePointer() const
        {  switch (TagPart()) {
             default:
               return PR_FALSE;
             case T_INTERFACE:
             case T_INTERFACE_IS:
               return PR_TRUE;
           }
        }

    PRBool IsArray() const
        {return TagPart() == T_ARRAY;}

    
    
    
    PRBool IsDependent() const
        {  switch (TagPart()) {
             default:
               return PR_FALSE;
             case T_INTERFACE_IS:
             case TD_ARRAY:
             case T_PSTRING_SIZE_IS:
             case T_PWSTRING_SIZE_IS:
               return PR_TRUE;
           }
        }

    PRUint8 TagPart() const
        {return (PRUint8) (flags & XPT_TDP_TAGMASK);}

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
        T_ASTRING           = TD_ASTRING
    };

};

class nsXPTParamInfo : public XPTParamDescriptor
{

public:
    nsXPTParamInfo(const XPTParamDescriptor& desc)
        {*(XPTParamDescriptor*)this = desc;}


    PRBool IsIn()  const    {return 0 != (XPT_PD_IS_IN(flags));}
    PRBool IsOut() const    {return 0 != (XPT_PD_IS_OUT(flags));}
    PRBool IsRetval() const {return 0 != (XPT_PD_IS_RETVAL(flags));}
    PRBool IsShared() const {return 0 != (XPT_PD_IS_SHARED(flags));}
    PRBool IsDipper() const {return 0 != (XPT_PD_IS_DIPPER(flags));}
    PRBool IsOptional() const {return 0 != (XPT_PD_IS_OPTIONAL(flags));}
    const nsXPTType GetType() const {return type.prefix;}

    

private:
    nsXPTParamInfo();   

};

class nsXPTMethodInfo : public XPTMethodDescriptor
{

public:
    nsXPTMethodInfo(const XPTMethodDescriptor& desc)
        {*(XPTMethodDescriptor*)this = desc;}

    PRBool IsGetter()      const {return 0 != (XPT_MD_IS_GETTER(flags) );}
    PRBool IsSetter()      const {return 0 != (XPT_MD_IS_SETTER(flags) );}
    PRBool IsNotXPCOM()    const {return 0 != (XPT_MD_IS_NOTXPCOM(flags));}
    PRBool IsConstructor() const {return 0 != (XPT_MD_IS_CTOR(flags)   );}
    PRBool IsHidden()      const {return 0 != (XPT_MD_IS_HIDDEN(flags) );}
    PRBool WantsOptArgc()  const {return 0 != (XPT_MD_WANTS_OPT_ARGC(flags));}
    const char* GetName()  const {return name;}
    PRUint8 GetParamCount()  const {return num_args;}
    
    const nsXPTParamInfo GetParam(PRUint8 idx) const
        {
            NS_PRECONDITION(idx < GetParamCount(),"bad arg");
            return params[idx];
        }
    const nsXPTParamInfo GetResult() const
        {return *result;}
private:
    nsXPTMethodInfo();  

};



struct nsXPTCMiniVariant;

class nsXPTConstant : public XPTConstDescriptor
{

public:
    nsXPTConstant(const XPTConstDescriptor& desc)
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
