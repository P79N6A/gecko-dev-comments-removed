














































#include "PyXPCOM_std.h"
#include "nsISupportsPrimitives.h"

static PRInt32 cInterfaces=0;
static PyObject *g_obFuncMakeInterfaceCount = NULL; 

PYXPCOM_EXPORT PyObject *
PyObject_FromNSInterface(nsISupports *aInterface,
                         const nsIID &iid, 
                         PRBool bMakeNicePyObject )
{
	return Py_nsISupports::PyObjectFromInterface(aInterface, iid,
	                                             bMakeNicePyObject);
}

PYXPCOM_EXPORT PRInt32 
_PyXPCOM_GetInterfaceCount(void)
{
	return cInterfaces;
}

Py_nsISupports::Py_nsISupports(nsISupports *punk, const nsIID &iid, PyTypeObject *this_type)
{
	ob_type = this_type;
	m_obj = punk;
	m_iid = iid;
	
	PR_AtomicIncrement(&cInterfaces);
	_Py_NewReference(this);
}

Py_nsISupports::~Py_nsISupports()
{
	SafeRelease(this);	
	PR_AtomicDecrement(&cInterfaces);
}

 nsISupports *
Py_nsISupports::GetI(PyObject *self, nsIID *ret_iid)
{
	if (self==NULL) {
		PyErr_SetString(PyExc_ValueError, "The Python object is invalid");
		return NULL;
	}
	Py_nsISupports *pis = (Py_nsISupports *)self;
	if (pis->m_obj==NULL) {
		
		PyErr_SetString(PyExc_ValueError, "Internal Error - The XPCOM object has been released.");
		return NULL;
	}
	if (ret_iid)
		*ret_iid = pis->m_iid;
	return pis->m_obj;
}

 void
Py_nsISupports::SafeRelease(Py_nsISupports *ob)
{
	if (!ob)
		return;
	if (ob->m_obj)
	{
		Py_BEGIN_ALLOW_THREADS;
		ob->m_obj = nsnull;
		Py_END_ALLOW_THREADS;
	}
}

 PyObject *
Py_nsISupports::getattr(const char *name)
{
	if (strcmp(name, "IID")==0)
		return Py_nsIID::PyObjectFromIID( m_iid );

	
	if (strcmp(name, "__unicode__")==0) {
		nsresult rv;
		PRUnichar *val = NULL;
		Py_BEGIN_ALLOW_THREADS;
		{ 
		nsCOMPtr<nsISupportsString> ss( do_QueryInterface(m_obj, &rv ));
		if (NS_SUCCEEDED(rv))
			rv = ss->ToString(&val);
		} 
		Py_END_ALLOW_THREADS;
		PyObject *ret = NS_FAILED(rv) ?
			PyXPCOM_BuildPyException(rv) :
			PyObject_FromNSString(val);
		if (val) nsMemory::Free(val);
		return ret;
	}
	PyXPCOM_TypeObject *this_type = (PyXPCOM_TypeObject *)ob_type;
	return Py_FindMethodInChain(&this_type->chain, this, (char *)name);
}

 int
Py_nsISupports::setattr(const char *name, PyObject *v)
{
	char buf[128];
	sprintf(buf, "%s has read-only attributes", ob_type->tp_name );
	PyErr_SetString(PyExc_TypeError, buf);
	return -1;
}

 Py_nsISupports *
Py_nsISupports::Constructor(nsISupports *pInitObj, const nsIID &iid)
{
	return new Py_nsISupports(pInitObj, 
				       iid, 
				       type);
}

PRBool
Py_nsISupports::InterfaceFromPyISupports(PyObject *ob, 
                                         const nsIID &iid, 
                                         nsISupports **ppv)
{
	nsISupports *pis;
	PRBool rc = PR_FALSE;
	if ( !Check(ob) )
	{
		PyErr_Format(PyExc_TypeError, "Objects of type '%s' can not be used as COM objects", ob->ob_type->tp_name);
		goto done;
	}
	nsIID already_iid;
	pis = GetI(ob, &already_iid);
	if ( !pis )
		goto done;	
	
	if (iid.Equals(Py_nsIID_NULL)) {
		
		
		
		Py_BEGIN_ALLOW_THREADS
		pis->AddRef();
		Py_END_ALLOW_THREADS
		*ppv = pis;
	} else {
		
		
		if (iid.Equals(already_iid)) {
			*ppv = pis;
			pis->AddRef();
		} else {
			nsresult r;
			Py_BEGIN_ALLOW_THREADS
			r = pis->QueryInterface(iid, (void **)ppv);
			Py_END_ALLOW_THREADS
			if ( NS_FAILED(r) )
			{
				PyXPCOM_BuildPyException(r);
				goto done;
			}
			
		}
	}
	rc = PR_TRUE;
done:
	return rc;
}

PRBool
Py_nsISupports::InterfaceFromPyObject(PyObject *ob, 
					   const nsIID &iid, 
					   nsISupports **ppv, 
					   PRBool bNoneOK,
					   PRBool bTryAutoWrap )
{
	if ( ob == NULL )
	{
		
		if ( !PyErr_Occurred() )
			PyErr_SetString(PyExc_TypeError, "The Python object is invalid");
		return PR_FALSE;
	}
	if ( ob == Py_None )
	{
		if ( bNoneOK )
		{
			*ppv = NULL;
			return PR_TRUE;
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "None is not a invalid interface object in this context");
			return PR_FALSE;
		}
	}

	
	if (iid.Equals(NS_GET_IID(nsIVariant)) || iid.Equals(NS_GET_IID(nsIWritableVariant))) {
		
		if (PyInstance_Check(ob)) {
			PyObject *sub_ob = PyObject_GetAttrString(ob, "_comobj_");
			if (sub_ob==NULL) {
				PyErr_Clear();
			} else {
				if (InterfaceFromPyISupports(sub_ob, iid, ppv)) {
					Py_DECREF(sub_ob);
					return PR_TRUE;
				}
				PyErr_Clear();
				Py_DECREF(sub_ob);
			}
		}
		nsresult nr = PyObject_AsVariant(ob, (nsIVariant **)ppv);
		if (NS_FAILED(nr)) {
			PyXPCOM_BuildPyException(nr);
			return PR_FALSE;
		}
		NS_ASSERTION(ppv != nsnull, "PyObject_AsVariant worked but gave null!");
		return PR_TRUE;
	}
	

	if (PyInstance_Check(ob)) {
		
		PyObject *use_ob = PyObject_GetAttrString(ob, "_comobj_");
		if (use_ob==NULL) {
			PyErr_Clear();
			if (bTryAutoWrap)
				
				return PyXPCOM_XPTStub::AutoWrapPythonInstance(ob, iid, ppv);
			PyErr_SetString(PyExc_TypeError, "The Python instance can not be converted to an XPCOM object");
			return PR_FALSE;
		} else
			ob = use_ob;

	} else {
		Py_INCREF(ob);
	}
	PRBool rc = InterfaceFromPyISupports(ob, iid, ppv);
	Py_DECREF(ob);
	return rc;
}



void
Py_nsISupports::RegisterInterface( const nsIID &iid, PyTypeObject *t)
{
	if (mapIIDToType==NULL)
		mapIIDToType = PyDict_New();

	if (mapIIDToType) {
		PyObject *key = Py_nsIID::PyObjectFromIID(iid);
		if (key)
			PyDict_SetItem(mapIIDToType, key, (PyObject *)t);
		Py_XDECREF(key);
	}
}

PyObject *
Py_nsISupports::PyObjectFromInterface(nsISupports *pis, 
				      const nsIID &riid, 
				      PRBool bMakeNicePyObject, 
				      PRBool bIsInternalCall )
{
	
	if (pis==NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (!bIsInternalCall) {
#ifdef NS_DEBUG
		nsISupports *queryResult = nsnull;
		pis->QueryInterface(riid, (void **)&queryResult);
		NS_ASSERTION(queryResult == pis, "QueryInterface needed");
		NS_IF_RELEASE(queryResult);
#endif
	}

	PyTypeObject *createType = NULL;
	
	
	if (!riid.Equals(NS_GET_IID(nsISupports))) {
		
		PyObject *obiid = Py_nsIID::PyObjectFromIID(riid);
		if (!obiid) return NULL;

		if (mapIIDToType != NULL)
			createType = (PyTypeObject *)PyDict_GetItem(mapIIDToType, obiid);
		Py_DECREF(obiid);
	}
	if (createType==NULL)
		createType = Py_nsISupports::type;
	
	if (!PyXPCOM_TypeObject::IsType(createType)) {
		PyErr_SetString(PyExc_RuntimeError, "The type map is invalid");
		return NULL;
	}
	
	PyXPCOM_TypeObject *myCreateType = (PyXPCOM_TypeObject *)createType;
	if (myCreateType->ctor==NULL) {
		PyErr_SetString(PyExc_TypeError, "The type does not declare a PyCom constructor");
		return NULL;
	}

	Py_nsISupports *ret = (*myCreateType->ctor)(pis, riid);
#ifdef _DEBUG_LIFETIMES
	PyXPCOM_LogF("XPCOM Object created at 0x%0xld, nsISupports at 0x%0xld",
		ret, ret->m_obj);
#endif
	if (ret && bMakeNicePyObject)
		return MakeDefaultWrapper(ret, riid);
	return ret;
}



PyObject *
Py_nsISupports::MakeDefaultWrapper(PyObject *pyis, 
			     const nsIID &iid)
{
	NS_PRECONDITION(pyis, "NULL pyobject!");
	PyObject *obIID = NULL;
	PyObject *args = NULL;
	PyObject *mod = NULL;
	PyObject *ret = NULL;

	obIID = Py_nsIID::PyObjectFromIID(iid);
	if (obIID==NULL)
		goto done;

	if (g_obFuncMakeInterfaceCount==NULL) {
		PyObject *mod = PyImport_ImportModule("xpcom.client");
		if (mod) 
			g_obFuncMakeInterfaceCount = PyObject_GetAttrString(mod, "MakeInterfaceResult");
		Py_XDECREF(mod);
	}
	if (g_obFuncMakeInterfaceCount==NULL) goto done;

	args = Py_BuildValue("OO", pyis, obIID);
	if (args==NULL) goto done;
	ret = PyEval_CallObject(g_obFuncMakeInterfaceCount, args);
done:
	if (PyErr_Occurred()) {
		NS_ABORT_IF_FALSE(ret==NULL, "Have an error, but also a return val!");
		PyXPCOM_LogError("Creating an interface object to be used as a result failed\n");
		PyErr_Clear();
	}
	Py_XDECREF(mod);
	Py_XDECREF(args);
	Py_XDECREF(obIID);
	if (ret==NULL) 
		ret = pyis; 
	else
		
		Py_DECREF(pyis);
	
	return ret;
}


PyObject *
Py_nsISupports::QueryInterface(PyObject *self, PyObject *args)
{
	PyObject *obiid;
	int bWrap = 1;
	
	
	
	if (!PyArg_ParseTuple(args, "O|i:QueryInterface", &obiid, &bWrap))
		return NULL;

	nsIID	iid;
	if (!Py_nsIID::IIDFromPyObject(obiid, &iid))
		return NULL;

	nsISupports *pMyIS = GetI(self);
	if (pMyIS==NULL) return NULL;

	
	
	if (!bWrap && iid.Equals(((Py_nsISupports *)self)->m_iid)) {
		Py_INCREF(self);
		return self;
	}

	nsCOMPtr<nsISupports> pis;
	nsresult r;
	Py_BEGIN_ALLOW_THREADS;
	r = pMyIS->QueryInterface(iid, getter_AddRefs(pis));
	Py_END_ALLOW_THREADS;

	
	if ( NS_FAILED(r) )
		return PyXPCOM_BuildPyException(r);

	
	return ((Py_nsISupports *)self)->MakeInterfaceResult(pis, iid, (PRBool)bWrap);
}



NS_EXPORT_STATIC_MEMBER_(struct PyMethodDef)
Py_nsISupports::methods[] =
{
	{ "queryInterface", Py_nsISupports::QueryInterface, 1, "Queries the object for an interface."},
	{ "QueryInterface", Py_nsISupports::QueryInterface, 1, "An alias for queryInterface."},
	{NULL}
};

void Py_nsISupports::InitType(void)
{
	type = new PyXPCOM_TypeObject(
		"nsISupports",
		NULL,
		sizeof(Py_nsISupports),
		methods,
		Constructor);
}

NS_EXPORT_STATIC_MEMBER_(PyXPCOM_TypeObject *) Py_nsISupports::type = NULL;
NS_EXPORT_STATIC_MEMBER_(PyObject *) Py_nsISupports::mapIIDToType = NULL;
