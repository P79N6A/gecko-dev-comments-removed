




















































#include "PyXPCOM_std.h"
#include <nsIModule.h>
#include <nsIFile.h>

class PyG_nsIModule : public PyG_Base, public nsIModule
{
public:
	PyG_nsIModule(PyObject *instance) : PyG_Base(instance, NS_GET_IID(nsIModule)) {;}
	PYGATEWAY_BASE_SUPPORT(nsIModule, PyG_Base);

	NS_DECL_NSIMODULE
};

PyG_Base *MakePyG_nsIModule(PyObject *instance)
{
	return new PyG_nsIModule(instance);
}



NS_IMETHODIMP
PyG_nsIModule::GetClassObject(nsIComponentManager *aCompMgr,
                                const nsCID& aClass,
                                const nsIID& aIID,
                                void** r_classObj)
{
	NS_PRECONDITION(r_classObj, "null pointer");
	*r_classObj = nsnull;
	CEnterLeavePython _celp;
	PyObject *cm = PyObject_FromNSInterface(aCompMgr, NS_GET_IID(nsIComponentManager));
	PyObject *iid = Py_nsIID::PyObjectFromIID(aIID);
	PyObject *clsid = Py_nsIID::PyObjectFromIID(aClass);
	const char *methodName = "getClassObject";
	PyObject *ret = NULL;
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "OOO", cm, clsid, iid);
	Py_XDECREF(cm);
	Py_XDECREF(iid);
	Py_XDECREF(clsid);
	if (NS_SUCCEEDED(nr)) {
		nr = Py_nsISupports::InterfaceFromPyObject(ret, aIID, (nsISupports **)r_classObj, PR_FALSE);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	if (NS_FAILED(nr)) {
		NS_ABORT_IF_FALSE(*r_classObj==NULL, "returning error result with an interface - probable leak!");
	}
	Py_XDECREF(ret);
	return nr;
}

NS_IMETHODIMP
PyG_nsIModule::RegisterSelf(nsIComponentManager *aCompMgr,
                              nsIFile* aPath,
                              const char* registryLocation,
                              const char* componentType)
{
	NS_PRECONDITION(aCompMgr, "null pointer");
	NS_PRECONDITION(aPath, "null pointer");
	CEnterLeavePython _celp;
	PyObject *cm = PyObject_FromNSInterface(aCompMgr, NS_GET_IID(nsIComponentManager));
	PyObject *path = PyObject_FromNSInterface(aPath, NS_GET_IID(nsIFile));
	const char *methodName = "registerSelf";
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "OOzz", cm, path, registryLocation, componentType);
	Py_XDECREF(cm);
	Py_XDECREF(path);
	return nr;
}

NS_IMETHODIMP
PyG_nsIModule::UnregisterSelf(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* registryLocation)
{
	NS_PRECONDITION(aCompMgr, "null pointer");
	NS_PRECONDITION(aPath, "null pointer");
	CEnterLeavePython _celp;
	PyObject *cm = PyObject_FromNSInterface(aCompMgr, NS_GET_IID(nsIComponentManager));
	PyObject *path = PyObject_FromNSInterface(aPath, NS_GET_IID(nsIFile));
	const char *methodName = "unregisterSelf";
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "OOz", cm, path, registryLocation);
	Py_XDECREF(cm);
	Py_XDECREF(path);
	return nr;
}

NS_IMETHODIMP
PyG_nsIModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
	NS_PRECONDITION(aCompMgr, "null pointer");
	NS_PRECONDITION(okToUnload, "null pointer");
	CEnterLeavePython _celp;
	
	PyObject *cm = PyObject_FromNSInterface(aCompMgr, NS_GET_IID(nsIComponentManager), PR_FALSE);
	const char *methodName = "canUnload";
	PyObject *ret = NULL;
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "O", cm);
	Py_XDECREF(cm);
	if (NS_SUCCEEDED(nr)) {
		*okToUnload = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	Py_XDECREF(ret);
	return nr;
}
