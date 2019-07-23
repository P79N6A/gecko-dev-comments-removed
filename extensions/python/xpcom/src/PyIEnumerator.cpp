














































#include "PyXPCOM_std.h"
#include <nsIEnumerator.h>

static nsIEnumerator *GetI(PyObject *self) {
	nsIID iid = NS_GET_IID(nsIEnumerator);

	if (!Py_nsISupports::Check(self, iid)) {
		PyErr_SetString(PyExc_TypeError, "This object is not the correct interface");
		return NULL;
	}
	return (nsIEnumerator *)Py_nsISupports::GetI(self);
}

static PyObject *PyFirst(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":First"))
		return NULL;

	nsIEnumerator *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->First();
	Py_END_ALLOW_THREADS;
	return PyInt_FromLong(r);
}

static PyObject *PyNext(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":Next"))
		return NULL;

	nsIEnumerator *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->Next();
	Py_END_ALLOW_THREADS;
	return PyInt_FromLong(r);
}

static PyObject *PyCurrentItem(PyObject *self, PyObject *args)
{
	PyObject *obIID = NULL;
	if (!PyArg_ParseTuple(args, "|O:CurrentItem", &obIID))
		return NULL;

	nsIID iid(NS_GET_IID(nsISupports));
	if (obIID != NULL && !Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;
	nsIEnumerator *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	nsISupports *pRet = nsnull;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->CurrentItem(&pRet);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);
	if (obIID) {
		nsISupports *temp;
		Py_BEGIN_ALLOW_THREADS;
		r = pRet->QueryInterface(iid, (void **)&temp);
		pRet->Release();
		Py_END_ALLOW_THREADS;
		if ( NS_FAILED(r) ) {
			return PyXPCOM_BuildPyException(r);
		}
		pRet = temp;
	}
	PyObject *ret = Py_nsISupports::PyObjectFromInterface(pRet, iid);
	NS_IF_RELEASE(pRet);
	return ret;
}




static PyObject *PyFetchBlock(PyObject *self, PyObject *args)
{
	PyObject *obIID = NULL;
	int n_wanted;
	int n_fetched = 0;
	if (!PyArg_ParseTuple(args, "i|O:FetchBlock", &n_wanted, &obIID))
		return NULL;

	nsIID iid(NS_GET_IID(nsISupports));
	if (obIID != NULL && !Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;
	nsIEnumerator *pI = GetI(self);
	if (pI==NULL)
		return NULL;

	
	
	nsISupports **fetched = new nsISupports*[n_wanted];
	if (fetched==nsnull) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(fetched, 0, sizeof(nsISupports *) * n_wanted);
	nsresult r = NS_OK;
	Py_BEGIN_ALLOW_THREADS;
	for (;n_fetched<n_wanted;) {
		nsISupports *pNew;
		r = pI->CurrentItem(&pNew);
		if (NS_FAILED(r)) {
			r = 0; 
			break;
		}
		if (obIID) {
			nsISupports *temp;
			r = pNew->QueryInterface(iid, (void **)&temp);
			pNew->Release();
			if ( NS_FAILED(r) ) {
				break;
			}
			pNew = temp;
		}
		fetched[n_fetched] = pNew;
		n_fetched++; 
		if (NS_FAILED(pI->Next()))
			break; 
	}
	Py_END_ALLOW_THREADS;
	PyObject *ret;
	if (NS_SUCCEEDED(r)) {
		ret = PyList_New(n_fetched);
		if (ret)
			for (int i=0;i<n_fetched;i++) {
				PyObject *new_ob = Py_nsISupports::PyObjectFromInterface(fetched[i], iid);
				NS_IF_RELEASE(fetched[i]);
				PyList_SET_ITEM(ret, i, new_ob);
			}
	} else
		ret = PyXPCOM_BuildPyException(r);

	if ( ret == NULL ) {
		
		for (int i=0;i<n_fetched;i++)
			fetched[i]->Release();

	}
	delete [] fetched;
	return ret;
}

static PyObject *PyIsDone(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":IsDone"))
		return NULL;

	nsIEnumerator *pI = GetI(self);
	nsresult r;
	if (pI==NULL)
		return NULL;

	Py_BEGIN_ALLOW_THREADS;
	r = pI->IsDone();
	Py_END_ALLOW_THREADS;
	if (NS_FAILED(r))
		return PyXPCOM_BuildPyException(r);
	PyObject *ret = r==NS_OK ? Py_True : Py_False;
	Py_INCREF(ret);
	return ret;
}

struct PyMethodDef 
PyMethods_IEnumerator[] =
{
	{ "First", PyFirst, 1},
	{ "first", PyFirst, 1},
	{ "Next", PyNext, 1},
	{ "next", PyNext, 1},
	{ "CurrentItem", PyCurrentItem, 1},
	{ "currentItem", PyCurrentItem, 1},
	{ "IsDone", PyIsDone, 1},
	{ "isDone", PyIsDone, 1},
	{ "FetchBlock", PyFetchBlock, 1},
	{ "fetchBlock", PyFetchBlock, 1},
	{NULL}
};
