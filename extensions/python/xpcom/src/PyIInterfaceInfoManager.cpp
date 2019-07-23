















































#include "PyXPCOM_std.h"
#include <nsIInterfaceInfoManager.h>

static nsIInterfaceInfoManager *GetI(PyObject *self) {
	nsIID iid = NS_GET_IID(nsIInterfaceInfoManager);

	if (!Py_nsISupports::Check(self, iid)) {
		PyErr_SetString(PyExc_TypeError, "This object is not the correct interface");
		return NULL;
	}
	return (nsIInterfaceInfoManager *)Py_nsISupports::GetI(self);
}

static PyObject *PyGetInfoForIID(PyObject *self, PyObject *args)
{
	PyObject *obIID = NULL;
	if (!PyArg_ParseTuple(args, "O", &obIID))
		return NULL;

	nsIInterfaceInfoManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID iid;
	if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;

	nsCOMPtr<nsIInterfaceInfo> pi;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->GetInfoForIID(&iid, getter_AddRefs(pi));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	
	nsIID new_iid = NS_GET_IID(nsIInterfaceInfo);
	
	
	return Py_nsISupports::PyObjectFromInterface(pi, new_iid, PR_FALSE);
}

static PyObject *PyGetInfoForName(PyObject *self, PyObject *args)
{
	char *name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	nsIInterfaceInfoManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsCOMPtr<nsIInterfaceInfo> pi;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->GetInfoForName(name, getter_AddRefs(pi));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	
	
	
	return Py_nsISupports::PyObjectFromInterface(pi, NS_GET_IID(nsIInterfaceInfo), PR_FALSE);
}

static PyObject *PyGetNameForIID(PyObject *self, PyObject *args)
{
	PyObject *obIID = NULL;
	if (!PyArg_ParseTuple(args, "O", &obIID))
		return NULL;

	nsIInterfaceInfoManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID iid;
	if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;

	char *ret_name = NULL;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->GetNameForIID(&iid, &ret_name);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	PyObject *ret = PyString_FromString(ret_name);
	nsMemory::Free(ret_name);
	return ret;
}

static PyObject *PyGetIIDForName(PyObject *self, PyObject *args)
{
	char *name;
	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	nsIInterfaceInfoManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID *iid_ret;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->GetIIDForName(name, &iid_ret);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	PyObject *ret = Py_nsIID::PyObjectFromIID(*iid_ret);
	nsMemory::Free(iid_ret);
	return ret;
}

static PyObject *PyEnumerateInterfaces(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	nsIInterfaceInfoManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsCOMPtr<nsIEnumerator> pRet;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->EnumerateInterfaces(getter_AddRefs(pRet));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	return Py_nsISupports::PyObjectFromInterface(pRet, NS_GET_IID(nsIEnumerator));
}




PyMethodDef 
PyMethods_IInterfaceInfoManager[] =
{
	{ "GetInfoForIID", PyGetInfoForIID, 1},
	{ "getInfoForIID", PyGetInfoForIID, 1},
	{ "GetInfoForName", PyGetInfoForName, 1},
	{ "getInfoForName", PyGetInfoForName, 1},
	{ "GetIIDForName", PyGetIIDForName, 1},
	{ "getIIDForName", PyGetIIDForName, 1},
	{ "GetNameForIID", PyGetNameForIID, 1},
	{ "getNameForIID", PyGetNameForIID, 1},
	{ "EnumerateInterfaces", PyEnumerateInterfaces, 1},
	{ "enumerateInterfaces", PyEnumerateInterfaces, 1},
	{NULL}
};
