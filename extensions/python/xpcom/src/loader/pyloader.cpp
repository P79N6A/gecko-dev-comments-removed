












































#include <PyXPCOM.h>
#include "nsITimelineService.h"
#include "nsILocalFile.h"

typedef nsresult (*pfnPyXPCOM_NSGetModule)(nsIComponentManager *servMgr,
                                          nsIFile* location,
                                          nsIModule** result);




nsresult PyXPCOM_NSGetModule(nsIComponentManager *servMgr,
                             nsIFile* location,
                             nsIModule** result)
{
	NS_PRECONDITION(result!=NULL, "null result pointer in PyXPCOM_NSGetModule!");
	NS_PRECONDITION(location!=NULL, "null nsIFile pointer in PyXPCOM_NSGetModule!");
	NS_PRECONDITION(servMgr!=NULL, "null servMgr pointer in PyXPCOM_NSGetModule!");
	CEnterLeavePython _celp;
	PyObject *func = NULL;
	PyObject *obServMgr = NULL;
	PyObject *obLocation = NULL;
	PyObject *wrap_ret = NULL;
	PyObject *args = NULL;
	PyObject *mod = PyImport_ImportModule("xpcom.server");
	if (!mod) goto done;
	func = PyObject_GetAttrString(mod, "NS_GetModule");
	if (func==NULL) goto done;
	obServMgr = Py_nsISupports::PyObjectFromInterface(servMgr, NS_GET_IID(nsIComponentManager));
	if (obServMgr==NULL) goto done;
	obLocation = Py_nsISupports::PyObjectFromInterface(location, NS_GET_IID(nsIFile));
	if (obLocation==NULL) goto done;
	args = Py_BuildValue("OO", obServMgr, obLocation);
	if (args==NULL) goto done;
	wrap_ret = PyEval_CallObject(func, args);
	if (wrap_ret==NULL) goto done;
	Py_nsISupports::InterfaceFromPyObject(wrap_ret, NS_GET_IID(nsIModule), (nsISupports **)result, PR_FALSE, PR_FALSE);
done:
	nsresult nr = NS_OK;
	if (PyErr_Occurred()) {
		PyXPCOM_LogError("Obtaining the module object from Python failed.\n");
		nr = PyXPCOM_SetCOMErrorFromPyException();
	}
	Py_XDECREF(func);
	Py_XDECREF(obServMgr);
	Py_XDECREF(obLocation);
	Py_XDECREF(wrap_ret);
	Py_XDECREF(mod);
	Py_XDECREF(args);
	return nr;
}

extern "C" NS_EXPORT nsresult NSGetModule(nsIComponentManager *servMgr,
                                          nsIFile* location,
                                          nsIModule** result)
{
	PyXPCOM_EnsurePythonEnvironment();
	NS_TIMELINE_START_TIMER("PyXPCOM: PyXPCOM NSGetModule entry point");
	nsresult rc = PyXPCOM_NSGetModule(servMgr, location, result);
	NS_TIMELINE_STOP_TIMER("PyXPCOM: PyXPCOM NSGetModule entry point");
	NS_TIMELINE_MARK_TIMER("PyXPCOM: PyXPCOM NSGetModule entry point");
	return rc;
}
