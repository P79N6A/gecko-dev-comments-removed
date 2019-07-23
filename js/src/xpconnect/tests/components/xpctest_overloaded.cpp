










































#include "xpctest_private.h"
#include "nsIXPCScriptable.h"



















































class xpcoverloaded : public nsIXPCTestOverloaded, public nsIXPCScriptable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCTESTOVERLOADED
    NS_DECL_NSIXPCSCRIPTABLE

    xpcoverloaded();
    virtual ~xpcoverloaded();
};

xpcoverloaded::xpcoverloaded()
{
    NS_ADDREF_THIS();
}

xpcoverloaded::~xpcoverloaded()
{
    
}

NS_IMPL_ISUPPORTS2(xpcoverloaded, nsIXPCTestOverloaded, nsIXPCScriptable)


NS_IMETHODIMP
xpcoverloaded::Foo1(PRInt32 p1)
{
    printf("xpcoverloaded::Foo1 called with p1 = %d\n", p1);
    return NS_OK;
}


NS_IMETHODIMP
xpcoverloaded::Foo2(PRInt32 p1, PRInt32 p2)
{
    printf("xpcoverloaded::Foo2 called with p1 = %d and p2 = %d\n", p1, p2);
    return NS_OK;
}


#define XPC_MAP_CLASSNAME           xpcoverloaded
#define XPC_MAP_QUOTED_CLASSNAME   "xpcoverloaded"
#define                             XPC_MAP_WANT_CREATE
#define XPC_MAP_FLAGS               0
#include "xpc_map_end.h" 





NS_IMETHODIMP 
xpcoverloaded::Create(nsIXPConnectWrappedNative *wrapper, 
                      JSContext * cx, JSObject * obj)
{








#if 1










    static const char name[] = "__xpcoverloadedProto__";
    static const char source[] =
        "__xpcoverloadedProto__ = {"
        "   Foo : function() {"
        "     switch(arguments.length) {"
        "     case 1: return this.Foo1(arguments[0]);"
        "     case 2: return this.Foo2(arguments[0], arguments[1]);"
        "     default: throw '1 or 2 arguments required';"
        "     }"
        "   }"
        "};";

    jsval proto;

    if(!JS_GetProperty(cx, JS_GetGlobalObject(cx), name, &proto) ||
       JSVAL_IS_PRIMITIVE(proto))
    {
       if(!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), source, strlen(source),
                          "builtin", 1, &proto) ||
          !JS_GetProperty(cx, JS_GetGlobalObject(cx), name, &proto)||
          JSVAL_IS_PRIMITIVE(proto))
            return NS_ERROR_UNEXPECTED;
    }
    if(!JS_SetPrototype(cx, obj, JSVAL_TO_OBJECT(proto)))
        return NS_ERROR_UNEXPECTED;
    return NS_OK;

#else
    
    

    
    
    static const char source[] =
        "this.Foo = function() {"
        "  switch(arguments.length) {"
        "  case 1: return this.Foo1(arguments[0]);"
        "  case 2: return this.Foo2(arguments[0], arguments[1]);"
        "  default: throw '1 or 2 arguments required';"
        "  }"
        "};";

    jsval ignored;
    JS_EvaluateScript(cx, obj, source, strlen(source), "builtin", 1, &ignored);
    return NS_OK;
#endif
}








NS_IMETHODIMP
xpctest::ConstructOverloaded(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    xpcoverloaded* obj = new xpcoverloaded();

    if(obj)
    {
        rv = obj->QueryInterface(aIID, aResult);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
        NS_RELEASE(obj);
    }
    else
    {
        *aResult = nsnull;
        rv = NS_ERROR_OUT_OF_MEMORY;
    }

    return rv;
}





