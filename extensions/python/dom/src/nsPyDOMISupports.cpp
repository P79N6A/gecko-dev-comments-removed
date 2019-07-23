



































#include "nsPyDOM.h"

PRInt32 cPyDOMISupportsObjects=0;

struct PyMethodDef 
Py_DOMnsISupports_Methods[] =
{
	{NULL}
};

class Py_DOMnsISupports : public Py_nsISupports
{
public:
	static PyXPCOM_TypeObject *type;
	static void InitType() {
		type = new PyXPCOM_TypeObject(
				"DOMISupports",
				Py_nsISupports::type,
				sizeof(Py_DOMnsISupports),
				Py_DOMnsISupports_Methods,
				nsnull);
		const nsIID &iid = NS_GET_IID(nsISupports);
		
		
	}
	
	
	virtual PyObject *MakeInterfaceResult(nsISupports *ps, const nsIID &iid,
	                                      PRBool bMakeNicePyObject = PR_TRUE)
	{
		if (!iid.Equals(NS_GET_IID(nsIClassInfo)))
			return PyObject_FromNSDOMInterface(m_pycontext, ps,
			                                   iid, bMakeNicePyObject);
		return Py_nsISupports::MakeInterfaceResult(ps, iid, bMakeNicePyObject);
	}

	Py_DOMnsISupports(PyObject *pycontext, nsISupports *p, const nsIID &iid) :
		Py_nsISupports(p, iid, type),
		m_pycontext(pycontext) {
		
		
		Py_INCREF(pycontext);
		PR_AtomicIncrement(&cPyDOMISupportsObjects);

	}
	virtual ~Py_DOMnsISupports() {
		Py_DECREF(m_pycontext);
		PR_AtomicDecrement(&cPyDOMISupportsObjects);
	}
	static PyObject *MakeDefaultWrapper(PyObject *pycontext,
	                                    PyObject *pyis, const nsIID &iid);
protected:
	PyObject *m_pycontext;
};


PyObject *PyObject_FromNSDOMInterface(PyObject *pycontext, nsISupports *pisOrig,
                                      const nsIID &iid, 
                                      PRBool bMakeNicePyObject )
{
	if (pisOrig==NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	nsCOMPtr<nsISupports> pis;
	
	
	if (iid.Equals(NS_GET_IID(nsISupports))) {
		pis = do_QueryInterface(pisOrig);
		if (!pis) {
			static const char *err = "Object failed QI for nsISupports!";
			NS_ERROR(err);
			PyErr_SetString(PyExc_RuntimeError, err);
			return NULL;
		}
	} else
		pis = pisOrig;
#ifdef NS_DEBUG
	nsISupports *queryResult = nsnull;
	pis->QueryInterface(iid, (void **)&queryResult);
	NS_ASSERTION(queryResult == pis, "QueryInterface needed");
	NS_IF_RELEASE(queryResult);
#endif

	Py_DOMnsISupports *ret = new Py_DOMnsISupports(pycontext, pis, iid);
	if (ret && bMakeNicePyObject)
		return Py_DOMnsISupports::MakeDefaultWrapper(pycontext, ret, iid);
	return ret;
}


PyObject *
Py_DOMnsISupports::MakeDefaultWrapper(PyObject *pycontext,
                                      PyObject *pyis, 
                                      const nsIID &iid)
{
	NS_PRECONDITION(pyis, "NULL pyobject!");
	PyObject *obIID = NULL;
	PyObject *ret = NULL;

	obIID = Py_nsIID::PyObjectFromIID(iid);
	if (obIID==NULL)
		goto done;

	ret = PyObject_CallMethod(pycontext, "MakeInterfaceResult",
	                          "OO", pyis, obIID);
	if (ret==NULL) goto done;
done:
	if (PyErr_Occurred()) {
		NS_ABORT_IF_FALSE(ret==NULL, "Have an error, but also a return val!");
		PyXPCOM_LogError("Creating an interface object to be used as a result failed\n");
		PyErr_Clear();
	}
	Py_XDECREF(obIID);
	if (ret==NULL) 
		ret = pyis; 
	else
		
		Py_DECREF(pyis);
	
	return ret;
}

void PyInit_DOMnsISupports()
{
	Py_DOMnsISupports::InitType();
}

PyXPCOM_TypeObject *Py_DOMnsISupports::type = NULL;
