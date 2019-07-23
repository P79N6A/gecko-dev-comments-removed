














































#include "PyXPCOM_std.h"

static nsIComponentManager *GetI(PyObject *self) {
	static const nsIID iid = NS_GET_IID(nsIComponentManager);

	if (!Py_nsISupports::Check(self, iid)) {
		PyErr_SetString(PyExc_TypeError, "This object is not the correct interface");
		return NULL;
	}
	return static_cast<nsIComponentManager*>(Py_nsISupports::GetI(self));
}

static PyObject *PyCreateInstanceByContractID(PyObject *self, PyObject *args)
{
	
	
	
	char *pid, *notyet = NULL;
	PyObject *obIID = NULL;
	if (!PyArg_ParseTuple(args, "s|zO", &pid, &notyet, &obIID))
		return NULL;
	if (notyet != NULL) {
		PyErr_SetString(PyExc_ValueError, "2nd arg must be none");
		return NULL;
	}
	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID	iid;
	if (obIID==NULL)
		iid = NS_GET_IID(nsISupports);
	else
		if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
			return NULL;

	nsCOMPtr<nsISupports> pis;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->CreateInstanceByContractID(pid, NULL, iid, getter_AddRefs(pis));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	
	return Py_nsISupports::PyObjectFromInterface(pis, iid, PR_FALSE);
}

static PyObject *PyCreateInstance(PyObject *self, PyObject *args)
{
	char *notyet = NULL;
	PyObject *obClassID = NULL, *obIID = NULL;
	if (!PyArg_ParseTuple(args, "O|zO", &obClassID, &notyet, &obIID))
		return NULL;
	if (notyet != NULL) {
		PyErr_SetString(PyExc_ValueError, "2nd arg must be none");
		return NULL;
	}
	nsIComponentManager *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsIID classID;
	if (!Py_nsIID::IIDFromPyObject(obClassID, &classID))
		return NULL;
	nsIID	iid;
	if (obIID==NULL)
		iid = NS_GET_IID(nsISupports);
	else
		if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
			return NULL;

	nsCOMPtr<nsISupports> pis;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->CreateInstance(classID, NULL, iid, getter_AddRefs(pis));
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	
	return Py_nsISupports::PyObjectFromInterface(pis, iid, PR_FALSE);
}

struct PyMethodDef 
PyMethods_IComponentManager[] =
{
	{ "createInstanceByContractID", PyCreateInstanceByContractID, 1},
	{ "createInstance",             PyCreateInstance,             1},
	{NULL}
};
