















































#include "PyXPCOM_std.h"
#include <nsIInputStream.h>

class PyG_nsIInputStream : public PyG_Base, public nsIInputStream
{
public:
	PyG_nsIInputStream(PyObject *instance) : PyG_Base(instance, NS_GET_IID(nsIInputStream)) {;}
	PYGATEWAY_BASE_SUPPORT(nsIInputStream, PyG_Base);

	NS_IMETHOD Close(void);
	NS_IMETHOD Available(PRUint32 *_retval);
	NS_IMETHOD Read(char * buf, PRUint32 count, PRUint32 *_retval);
	NS_IMETHOD ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 count, PRUint32 *_retval);
	NS_IMETHOD IsNonBlocking(PRBool *aNonBlocking);
};


PyG_Base *MakePyG_nsIInputStream(PyObject *instance)
{
	return new PyG_nsIInputStream(instance);
}

NS_IMETHODIMP
PyG_nsIInputStream::Close()
{
	CEnterLeavePython _celp;
	const char *methodName = "close";
	return InvokeNativeViaPolicy(methodName, NULL);
}

NS_IMETHODIMP
PyG_nsIInputStream::Available(PRUint32 *_retval)
{
	NS_PRECONDITION(_retval, "null pointer");
	CEnterLeavePython _celp;
	PyObject *ret;
	const char *methodName = "available";
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret);
	if (NS_SUCCEEDED(nr)) {
		*_retval = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
		Py_XDECREF(ret);
	}
	return nr;
}

NS_IMETHODIMP
PyG_nsIInputStream::Read(char * buf, PRUint32 count, PRUint32 *_retval)
{
	NS_PRECONDITION(_retval, "null pointer");
	NS_PRECONDITION(buf, "null pointer");
	CEnterLeavePython _celp;
	PyObject *ret;
	const char *methodName = "read";
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "i", count);
	if (NS_SUCCEEDED(nr)) {
		PRUint32 py_size;
		const void *py_buf;
		if (PyObject_AsReadBuffer(ret, &py_buf, (int *)&py_size)!=0) {
			PyErr_Format(PyExc_TypeError, "nsIInputStream::read() method must return a buffer object - not a '%s' object", ret->ob_type->tp_name);
			nr = HandleNativeGatewayError(methodName);
		} else {
			if (py_size > count) {
				PyXPCOM_LogWarning("nsIInputStream::read() was asked for %d bytes, but the string returned is %d bytes - truncating!\n", count, py_size);
				py_size = count;
			}
			memcpy(buf, py_buf, py_size);
			*_retval = py_size;
		}
	}
	return nr;
}


NS_IMETHODIMP
PyG_nsIInputStream::ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 count, PRUint32 *_retval)
{
	NS_WARNING("ReadSegments() not implemented!!!");
	return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
PyG_nsIInputStream::IsNonBlocking(PRBool *aNonBlocking)
{
	NS_PRECONDITION(aNonBlocking, "null pointer");
	CEnterLeavePython _celp;
	PyObject *ret;
	const char *methodName = "isNonBlocking";
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret);
	if (NS_SUCCEEDED(nr)) {
		*aNonBlocking = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
		Py_XDECREF(ret);
	}
	return nr;
}
