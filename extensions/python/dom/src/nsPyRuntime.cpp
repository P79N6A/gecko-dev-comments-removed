



































#include "nsPyRuntime.h"
#include "nsPyContext.h"
#include "nsICategoryManager.h"
#include "nsIScriptContext.h"
#include "nsIDOMEventTarget.h"

extern void init_nsdom();

static PRBool initialized = PR_FALSE;


NS_INTERFACE_MAP_BEGIN(nsPythonRuntime)
  NS_INTERFACE_MAP_ENTRY(nsIScriptRuntime)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsPythonRuntime)
NS_IMPL_RELEASE(nsPythonRuntime)

nsresult
nsPythonRuntime::CreateContext(nsIScriptContext **ret)
{
    PyXPCOM_EnsurePythonEnvironment();
    if (!initialized) {
        PyInit_DOMnsISupports();
        init_nsdom();
        initialized = PR_TRUE;
    }
    *ret = new nsPythonContext();
    if (!ret)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_IF_ADDREF(*ret);
    return NS_OK;
}

nsresult
nsPythonRuntime::ParseVersion(const nsString &aVersionStr, PRUint32 *aFlags) {
    
    
    
    
    if (aVersionStr.IsEmpty()) {
        *aFlags = 0;
        return NS_OK;
    }
    NS_ERROR("Don't specify a version for Python");
    return NS_ERROR_UNEXPECTED;
  }

nsresult
nsPythonRuntime::DropScriptObject(void *object)
{
    if (object) {
        PYLEAK_STAT_DECREMENT(ScriptObject);
        CEnterLeavePython _celp;
        Py_DECREF((PyObject *)object);
    }
    return NS_OK;
}

nsresult
nsPythonRuntime::HoldScriptObject(void *object)
{
    if (object) {
        PYLEAK_STAT_INCREMENT(ScriptObject);
        CEnterLeavePython _celp;
        Py_INCREF((PyObject *)object);
    }
    return NS_OK;
}
