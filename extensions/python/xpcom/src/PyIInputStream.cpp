














































#include "PyXPCOM_std.h"
#include "nsIInputStream.h"




static nsIInputStream *GetI(PyObject *self) {
	nsIID iid = NS_GET_IID(nsIInputStream);

	if (!Py_nsISupports::Check(self, iid)) {
		PyErr_SetString(PyExc_TypeError, "This object is not the correct interface");
		return NULL;
	}
	return (nsIInputStream *)Py_nsISupports::GetI(self);
}

static PyObject *DoPyRead_Buffer(nsIInputStream *pI, PyObject *obBuffer, PRUint32 n)
{
	PRUint32 nread;
	void *buf;
	PRUint32 buf_len;
	if (PyObject_AsWriteBuffer(obBuffer, &buf, (int *)&buf_len) != 0) {
		PyErr_Clear();
		PyErr_SetString(PyExc_TypeError, "The buffer object does not have a write buffer!");
		return NULL;
	}
	if (n==(PRUint32)-1) {
		n = buf_len;
	} else {
		if (n > buf_len) {
			NS_WARNING("Warning: PyIInputStream::read() was passed an integer size greater than the size of the passed buffer!  Buffer size used.\n");
			n = buf_len;
		}
	}
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->Read((char *)buf, n, &nread);
	Py_END_ALLOW_THREADS;
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);
	return PyInt_FromLong(nread);
}

static PyObject *DoPyRead_Size(nsIInputStream *pI, PRUint32 n)
{
	if (n==(PRUint32)-1) {
		nsresult r;
		Py_BEGIN_ALLOW_THREADS;
		r = pI->Available(&n);
		Py_END_ALLOW_THREADS;
		if (NS_FAILED(r))
			return PyXPCOM_BuildPyException(r);
	}
	if (n==0) { 
		return PyBuffer_New(0);
	}
	char *buf = (char *)nsMemory::Alloc(n);
	if (buf==NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	nsresult r;
	PRUint32 nread;
	Py_BEGIN_ALLOW_THREADS;
	r = pI->Read(buf, n, &nread);
	Py_END_ALLOW_THREADS;
	PyObject *rc = NULL;
	if ( NS_SUCCEEDED(r) ) {
		rc = PyBuffer_New(nread);
		if (rc != NULL) {
			void *ob_buf;
			PRUint32 buf_len;
			if (PyObject_AsWriteBuffer(rc, &ob_buf, (int *)&buf_len) != 0) {
				
				return NULL;
			}
			if (buf_len != nread) {
				PyErr_SetString(PyExc_RuntimeError, "New buffer isn't the size we created it!");
				return NULL;
			}
			memcpy(ob_buf, buf, nread);
		}
	} else
		PyXPCOM_BuildPyException(r);
	nsMemory::Free(buf);
	return rc;
}

static PyObject *PyRead(PyObject *self, PyObject *args)
{
	PyObject *obBuffer = NULL;
	PRUint32 n = (PRUint32)-1;

	nsIInputStream *pI = GetI(self);
	if (pI==NULL)
		return NULL;
	if (PyArg_ParseTuple(args, "|i", (int *)&n))
		
		return DoPyRead_Size(pI, n);
	
	PyErr_Clear();
	if (!PyArg_ParseTuple(args, "O|i", &obBuffer, (int *)&n)) {
		PyErr_Clear();
		PyErr_SetString(PyExc_TypeError, "'read()' must be called as (buffer_ob, int_size=-1) or (int_size=-1)");
		return NULL;
	}
	return DoPyRead_Buffer(pI, obBuffer, n);
}


struct PyMethodDef 
PyMethods_IInputStream[] =
{
	{ "read", PyRead, 1},
	
	{NULL}
};
