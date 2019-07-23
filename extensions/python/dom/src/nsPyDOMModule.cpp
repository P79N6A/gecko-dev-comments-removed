








































#include "nsIGenericFactory.h"
#include "nsPyRuntime.h"

#include "nsPyContext.h"
#include "nsIServiceManager.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIAtomService.h"
#include "nsIEventListenerManager.h"
#include "nsIScriptTimeoutHandler.h"
#include "nsIDOMEventReceiver.h"
#include "nsIArray.h"
#include "jscntxt.h"
#include "nsPIDOMWindow.h"
#include "nsDOMScriptObjectHolder.h"


#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPythonRuntime)

static const nsModuleComponentInfo components[] = {
     {"Python Script Language", NS_SCRIPT_LANGUAGE_PYTHON_CID,
      NS_SCRIPT_LANGUAGE_PYTHON_CONTRACTID_NAME,
      nsPythonRuntimeConstructor,},
     
     {"Python Script Language", NS_SCRIPT_LANGUAGE_PYTHON_CID,
      NS_SCRIPT_LANGUAGE_PYTHON_CONTRACTID_ID,
      nsPythonRuntimeConstructor,},
};


NS_IMPL_NSGETMODULE(nsPyDOMModule, components)





PyObject *PyJSExec(PyObject *self, PyObject *args)
{
    PyObject *obGlobal;
    char *code;
    char *url = "<python JSExec script>";
    PRUint32 lineNo = 0;
    PRUint32 version = 0;
    if (!PyArg_ParseTuple(args, "Os|sii", &obGlobal, &code, &url, &lineNo, &version))
        return NULL;
    nsCOMPtr<nsISupports> isup;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obGlobal,
                NS_GET_IID(nsISupports),
                getter_AddRefs(isup),
                PR_FALSE, PR_FALSE))
        return NULL;
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal = do_QueryInterface(isup);
    if (scriptGlobal == nsnull)
        return PyErr_Format(PyExc_TypeError, "Object is not an nsIScriptGlobal");
    nsIScriptContext *scriptContext =
           scriptGlobal->GetScriptContext(nsIProgrammingLanguage::JAVASCRIPT);
    if (!scriptContext)
        return PyErr_Format(PyExc_RuntimeError, "No javascript context available");

    
    nsIPrincipal *principal = nsnull;
    nsCOMPtr<nsIScriptObjectPrincipal> globalData = do_QueryInterface(scriptGlobal);
    if (globalData)
        principal = globalData->GetPrincipal();
    if (!principal)
        return PyErr_Format(PyExc_RuntimeError, "No nsIPrincipal available");

    nsresult rv;
    void *scope = scriptGlobal->GetScriptGlobal(nsIProgrammingLanguage::JAVASCRIPT);
    nsScriptObjectHolder scriptObject(scriptContext);
    JSContext *ctx = (JSContext *)scriptContext->GetNativeContext();

    nsAutoString str;
    PRBool bIsUndefined;
    Py_BEGIN_ALLOW_THREADS
    rv = scriptContext->CompileScript( NS_ConvertASCIItoUTF16(code).get(),
                                       strlen(code),
                                       scope,
                                       principal, 
                                       url,
                                       lineNo,
                                       version,
                                       scriptObject);
    if (NS_SUCCEEDED(rv) && scriptObject)
        rv = scriptContext->ExecuteScript(scriptObject, scope, &str,
                                          &bIsUndefined);
    Py_END_ALLOW_THREADS

    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    return Py_BuildValue("NN", PyObject_FromNSString(str), PyBool_FromLong(bIsUndefined));
    
}



static PyObject *PyIsOuterWindow(PyObject *self, PyObject *args)
{
    PyObject *obTarget;
    if (!PyArg_ParseTuple(args, "O", &obTarget))
        return NULL;

    
    nsCOMPtr<nsPIDOMWindow> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsPIDOMWindow),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    return PyBool_FromLong(target->IsOuterWindow());
}

static PyObject *PyGetCurrentInnerWindow(PyObject *self, PyObject *args)
{
    PyObject *obTarget;
    if (!PyArg_ParseTuple(args, "O", &obTarget))
        return NULL;

    
    nsCOMPtr<nsPIDOMWindow> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsPIDOMWindow),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    
    nsCOMPtr<nsIScriptGlobalObject> sgo(do_QueryInterface(target));
    if (!sgo)
        return PyErr_Format(PyExc_ValueError, "Object does not supports nsIScriptGlobalObject");
    nsIScriptContext *ctx = sgo->GetScriptContext(nsIProgrammingLanguage::PYTHON);
    if (!ctx)
        return PyErr_Format(PyExc_ValueError, "Object not initialized for Python");

    nsCOMPtr<nsIScriptGlobalObject> innerGlobal(
               do_QueryInterface(target->GetCurrentInnerWindow()));
    if (!innerGlobal)
        return PyErr_Format(PyExc_ValueError, "Result object does not supports nsIScriptGlobalObject");
    nsPythonContext *pyctx = (nsPythonContext *)ctx;
    return pyctx->PyObject_FromInterface(innerGlobal,
                                         NS_GET_IID(nsIScriptGlobalObject));
}

static PyObject *PySetTimeoutOrInterval(PyObject *self, PyObject *args)
{
    PyObject *obTarget;
    PyObject *obHandler;
    PyObject *obArgs;
    int interval;
    int isInterval;
    if (!PyArg_ParseTuple(args, "OiOO!i:SetTimeoutOrInterval", &obTarget,
                          &interval, &obHandler,
                          &PyTuple_Type, &obArgs,
                          &isInterval))
        return NULL;

    if (!PySequence_Check(obArgs))
        return PyErr_Format(PyExc_TypeError, "args param must be a sequence (got %s)",
                            obArgs->ob_type->tp_name);

    PyObject *funObject = NULL;
    nsAutoString strExpr;
    if (PyCallable_Check(obHandler)) {
        funObject = obHandler;
    } else {
        if (!PyObject_AsNSString(obHandler, strExpr))
            return NULL;
    }

    
    nsCOMPtr<nsPIDOMWindow> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsPIDOMWindow),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    nsCOMPtr<nsIScriptTimeoutHandler> handler;
    nsresult rv;
    rv = CreatePyTimeoutHandler(strExpr, funObject, obArgs,
                                getter_AddRefs(handler));
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    PRInt32 ret;
    Py_BEGIN_ALLOW_THREADS;
    rv = target->SetTimeoutOrInterval(handler, interval, isInterval, &ret);
    Py_END_ALLOW_THREADS;
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    return PyInt_FromLong(ret);
}

static PyObject *PyClearTimeoutOrInterval(PyObject *self, PyObject *args)
{
    int tid;
    PyObject *obTarget;
    if (!PyArg_ParseTuple(args, "Oi", &obTarget, &tid))
        return NULL;

    
    nsCOMPtr<nsPIDOMWindow> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsPIDOMWindow),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    nsresult rv;
    Py_BEGIN_ALLOW_THREADS;
    rv = target->ClearTimeoutOrInterval((PRInt32)tid);
    Py_END_ALLOW_THREADS;
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    Py_INCREF(Py_None);
    return Py_None;
}














static PyObject *PyAddScriptEventListener(PyObject *self, PyObject *args)
{
    PyObject *obTarget, *obBody;
    char *name;
    int lang = nsIProgrammingLanguage::PYTHON;
    int defer, untrusted;
    if (!PyArg_ParseTuple(args, "OsOii|i", &obTarget, &name, &obBody,
                          &defer, &untrusted, &lang))
        return NULL;

    
    nsCOMPtr<nsISupports> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsISupports),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    nsAutoString body;
    if (!PyObject_AsNSString(obBody, body))
        return NULL;

    
    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(target));
    if (!receiver) return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIEventListenerManager> manager;
    receiver->GetListenerManager(PR_TRUE, getter_AddRefs(manager));
    if (!manager) return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);

    
    nsresult rv;
    nsCOMPtr<nsIAtomService> atomService =
                  do_GetService(NS_ATOMSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    nsCOMPtr<nsIAtom> atom;
    rv = atomService->GetAtomUTF8(name, getter_AddRefs(atom));
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    Py_BEGIN_ALLOW_THREADS
    rv = manager->AddScriptEventListener(target, atom, body, lang, defer,
                                         untrusted);
    Py_END_ALLOW_THREADS
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    Py_INCREF(Py_None);
    return Py_None;
}





static PyObject *PyRegisterScriptEventListener(PyObject *self, PyObject *args)
{
    PyObject *obTarget, *obGlobal, *obScope;
    char *name;
    if (!PyArg_ParseTuple(args, "OOOs", &obGlobal, &obScope, &obTarget, &name))
        return NULL;

    nsCOMPtr<nsISupports> isup;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obGlobal,
                NS_GET_IID(nsISupports),
                getter_AddRefs(isup),
                PR_FALSE, PR_FALSE))
        return NULL;
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal = do_QueryInterface(isup);
    if (scriptGlobal == nsnull)
        return PyErr_Format(PyExc_TypeError, "Object is not an nsIScriptGlobal");
    nsIScriptContext *scriptContext =
           scriptGlobal->GetScriptContext(nsIProgrammingLanguage::PYTHON);
    if (!scriptContext)
        return PyErr_Format(PyExc_RuntimeError, "Can't find my context??");

    
    nsCOMPtr<nsISupports> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsISupports),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    
    
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(target));
    if (win != nsnull && win->IsOuterWindow()) {
        target = win->GetCurrentInnerWindow();
    }
    
    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(target));
    if (!receiver) return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIEventListenerManager> manager;
    receiver->GetListenerManager(PR_TRUE, getter_AddRefs(manager));
    if (!manager) return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);

    nsresult rv;
    nsCOMPtr<nsIAtomService> atomService =
                  do_GetService(NS_ATOMSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    nsCOMPtr<nsIAtom> atom;
    rv = atomService->GetAtomUTF8(name, getter_AddRefs(atom));
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    Py_BEGIN_ALLOW_THREADS
    rv = manager->RegisterScriptEventListener(scriptContext, obScope,
                                              target, atom);
    Py_END_ALLOW_THREADS
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    Py_INCREF(Py_None);
    return Py_None;
}






static PyObject *PyCompileScriptEventListener(PyObject *self, PyObject *args)
{
    PyObject *obTarget, *obGlobal, *obScope;
    char *name;
    if (!PyArg_ParseTuple(args, "OOOs", &obGlobal, &obScope, &obTarget, &name))
        return NULL;

    nsCOMPtr<nsISupports> isup;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obGlobal,
                NS_GET_IID(nsISupports),
                getter_AddRefs(isup),
                PR_FALSE, PR_FALSE))
        return NULL;
    nsCOMPtr<nsIScriptGlobalObject> scriptGlobal = do_QueryInterface(isup);
    if (scriptGlobal == nsnull)
        return PyErr_Format(PyExc_TypeError, "Object is not an nsIScriptGlobal");
    nsIScriptContext *scriptContext =
           scriptGlobal->GetScriptContext(nsIProgrammingLanguage::PYTHON);
    if (!scriptContext)
        return PyErr_Format(PyExc_RuntimeError, "Can't find my context??");

    
    nsCOMPtr<nsISupports> target;
    if (!Py_nsISupports::InterfaceFromPyObject(
                obTarget,
                NS_GET_IID(nsISupports),
                getter_AddRefs(target),
                PR_FALSE, PR_FALSE))
        return NULL;

    
    
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(target));
    if (win != nsnull && win->IsOuterWindow()) {
        target = win->GetCurrentInnerWindow();
    }
    
    nsCOMPtr<nsIDOMEventReceiver> receiver(do_QueryInterface(target));
    if (!receiver) return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIEventListenerManager> manager;
    receiver->GetListenerManager(PR_TRUE, getter_AddRefs(manager));
    if (!manager) return PyXPCOM_BuildPyException(NS_ERROR_UNEXPECTED);

    nsresult rv;
    nsCOMPtr<nsIAtomService> atomService =
                  do_GetService(NS_ATOMSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    nsCOMPtr<nsIAtom> atom;
    rv = atomService->GetAtomUTF8(name, getter_AddRefs(atom));
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);

    PRBool didCompile;
    Py_BEGIN_ALLOW_THREADS
    rv = manager->CompileScriptEventListener(scriptContext, obScope,
                                             target, atom, &didCompile);
    Py_END_ALLOW_THREADS
    if (NS_FAILED(rv))
        return PyXPCOM_BuildPyException(rv);
    return PyBool_FromLong(didCompile);
}

static PyObject *PyMakeArray(PyObject *self, PyObject *args)
{
    PyObject *ob;
    if (!PyArg_ParseTuple(args, "O!:MakeArray", &PyTuple_Type, &ob))
        return NULL;

     nsCOMPtr<nsIArray> array;
     nsresult rv = NS_CreatePyArgv(ob, getter_AddRefs(array));
     if (NS_FAILED(rv) || array == nsnull)
        return PyXPCOM_BuildPyException(rv);
     return PyObject_FromNSInterface(array, NS_GET_IID(nsIArray));
}

static PyObject *PyMakeDOMObject(PyObject *self, PyObject *args)
{
     PyObject *ob, *obContext;
     if (!PyArg_ParseTuple(args, "OO:MakeDOMObject", &obContext, &ob))
         return NULL;

     nsCOMPtr<nsISupports> sup;
     if (!Py_nsISupports::InterfaceFromPyObject(ob, NS_GET_IID(nsISupports),
                                                getter_AddRefs(sup),
                                                PR_FALSE, PR_FALSE))
          return NULL;

     return PyObject_FromNSDOMInterface(obContext, sup, NS_GET_IID(nsISupports));
}

static struct PyMethodDef methods[]=
{
    {"JSExec", PyJSExec, 1},
    {"AddScriptEventListener", PyAddScriptEventListener, 1},
    {"RegisterScriptEventListener", PyRegisterScriptEventListener, 1},
    {"CompileScriptEventListener", PyCompileScriptEventListener, 1},
    {"GetCurrentInnerWindow", PyGetCurrentInnerWindow, 1},
    {"IsOuterWindow", PyIsOuterWindow, 1},
    {"SetTimeoutOrInterval", PySetTimeoutOrInterval, 1},
    {"ClearTimeoutOrInterval", PyClearTimeoutOrInterval, 1},
    {"MakeArray", PyMakeArray, 1},
    {"MakeDOMObject", PyMakeDOMObject, 1},
    { NULL }
};




#define REGISTER_IID(t) { \
	PyObject *iid_ob = Py_nsIID::PyObjectFromIID(NS_GET_IID(t)); \
	PyDict_SetItemString(dict, "IID_"#t, iid_ob); \
	Py_DECREF(iid_ob); \
	}



void 
init_nsdom() {
    CEnterLeavePython _celp;
    
    PyObject *oModule = Py_InitModule("_nsdom", methods);
    PyObject *dict = PyModule_GetDict(oModule);
    REGISTER_IID(nsIScriptGlobalObject);
}
