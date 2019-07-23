















































#include "PyXPCOM_std.h"
#include <nsIModule.h>
#include <nsIInputStream.h>

static PRInt32 cGateways = 0;

PYXPCOM_EXPORT PRInt32 _PyXPCOM_GetGatewayCount(void)
{
	return cGateways;
}

extern PyG_Base *MakePyG_nsIModule(PyObject *);
extern PyG_Base *MakePyG_nsIInputStream(PyObject *instance);

static char *PyXPCOM_szDefaultGatewayAttributeName = "_com_instance_default_gateway_";
static PyG_Base *GetDefaultGateway(PyObject *instance);
static PRBool CheckDefaultGateway(PyObject *real_inst, REFNSIID iid, nsISupports **ret_gateway);

 nsresult 
PyG_Base::CreateNew(PyObject *pPyInstance, const nsIID &iid, void **ppResult)
{
	NS_PRECONDITION(ppResult && *ppResult==NULL, "NULL or uninitialized pointer");
	if (ppResult==nsnull)
		return NS_ERROR_NULL_POINTER;

	PyG_Base *ret;
	
	if (iid.Equals(NS_GET_IID(nsIModule)))
		ret = MakePyG_nsIModule(pPyInstance);
	else if (iid.Equals(NS_GET_IID(nsIInputStream)))
		ret = MakePyG_nsIInputStream(pPyInstance);
	else
		ret = new PyXPCOM_XPTStub(pPyInstance, iid);
	if (ret==nsnull)
		return NS_ERROR_OUT_OF_MEMORY;
	ret->AddRef(); 
	*ppResult = ret->ThisAsIID(iid);
	NS_ABORT_IF_FALSE(*ppResult != NULL, "ThisAsIID() gave NULL, but we know it supports it!");
	return *ppResult ? NS_OK : NS_ERROR_FAILURE;
}

PyG_Base::PyG_Base(PyObject *instance, const nsIID &iid)
{
	
	PR_AtomicIncrement(&cGateways);
	m_pBaseObject = GetDefaultGateway(instance);
	

	NS_ABORT_IF_FALSE(!(iid.Equals(NS_GET_IID(nsISupportsWeakReference)) || iid.Equals(NS_GET_IID(nsIWeakReference))),"Should not be creating gateways with weak-ref interfaces");
	m_iid = iid;
	m_pPyObject = instance;
	NS_PRECONDITION(instance, "NULL PyObject for PyXPCOM_XPTStub!");

#ifdef NS_BUILD_REFCNT_LOGGING
	
	PyObject *realInstance = PyObject_GetAttrString(instance, "_obj_");
	PyObject *r = PyObject_Repr(realInstance);
	const char *szRepr;
	if (r==NULL) {
		PyXPCOM_LogError("Getting the __repr__ of the object failed");
		PyErr_Clear();
		szRepr = "(repr failed!)";
	}
	else
		szRepr = PyString_AsString(r);
	if (szRepr==NULL) szRepr = "";
	int reprOffset = *szRepr=='<' ? 1 : 0;
	static const char *reprPrefix = "component:";
	if (strncmp(reprPrefix, szRepr+reprOffset, strlen(reprPrefix)) == 0)
		reprOffset += strlen(reprPrefix);
	strncpy(refcntLogRepr, szRepr + reprOffset, sizeof(refcntLogRepr)-1);
	refcntLogRepr[sizeof(refcntLogRepr)-1] = '\0';
	
	char *lastPos = strstr(refcntLogRepr, " at ");
	if (lastPos) *lastPos = '\0';
	Py_XDECREF(realInstance);
	Py_XDECREF(r);
#endif 

#ifdef DEBUG_LIFETIMES
	{
		char *iid_repr;
		nsCOMPtr<nsIInterfaceInfoManager> iim = XPTI_GetInterfaceInfoManager();
		if (iim!=nsnull)
			iim->GetNameForIID(&iid, &iid_repr);
		PyObject *real_instance = PyObject_GetAttrString(instance, "_obj_");
		PyObject *real_repr = PyObject_Repr(real_instance);

		PYXPCOM_LOG_DEBUG("PyG_Base created at %p\n  instance_repr=%s\n  IID=%s\n", this, PyString_AsString(real_repr), iid_repr);
		nsMemory::Free(iid_repr);
		Py_XDECREF(real_instance);
		Py_XDECREF(real_repr);
	}
#endif 
	Py_XINCREF(instance); 

#ifdef DEBUG_FULL
	LogF("PyGatewayBase: created %s", m_pPyObject ? m_pPyObject->ob_type->tp_name : "<NULL>");
#endif
}

PyG_Base::~PyG_Base()
{
	PR_AtomicDecrement(&cGateways);
#ifdef DEBUG_LIFETIMES
	PYXPCOM_LOG_DEBUG("PyG_Base: deleted %p", this);
#endif
	if ( m_pPyObject ) {
		CEnterLeavePython celp;
		Py_DECREF(m_pPyObject);
	}
	if (m_pBaseObject)
		m_pBaseObject->Release();
	if (m_pWeakRef) {
		
		
		CEnterLeaveXPCOMFramework _celf;
		PyXPCOM_GatewayWeakReference *p = (PyXPCOM_GatewayWeakReference *)(nsISupports *)m_pWeakRef;
		p->m_pBase = nsnull;
		m_pWeakRef = nsnull;
	}
}


void *PyG_Base::ThisAsIID( const nsIID &iid )
{
	if (this==NULL) return NULL;
	if (iid.Equals(NS_GET_IID(nsISupports)))
		return (nsISupports *)(nsIInternalPython *)this;
	if (iid.Equals(NS_GET_IID(nsISupportsWeakReference)))
		return (nsISupportsWeakReference *)this;
	if (iid.Equals(NS_GET_IID(nsIInternalPython))) 
		return (nsISupports *)(nsIInternalPython *)this;
	return NULL;
}



 PRBool 
PyG_Base::AutoWrapPythonInstance(PyObject *ob, const nsIID &iid, nsISupports **ppret)
{
	NS_PRECONDITION(ppret!=NULL, "null pointer when wrapping a Python instance!");
	NS_PRECONDITION(ob && PyInstance_Check(ob), "AutoWrapPythonInstance is expecting an non-NULL instance!");
	PRBool ok = PR_FALSE;
    
	static PyObject *func = NULL; 
	PyObject *obIID = NULL;
	PyObject *wrap_ret = NULL;
	PyObject *args = NULL;
	if (func==NULL) { 
		PyObject *mod = PyImport_ImportModule("xpcom.server");
		if (mod)
			func = PyObject_GetAttrString(mod, "WrapObject");
		Py_XDECREF(mod);
		if (func==NULL) goto done;
	}
	
	if (CheckDefaultGateway(ob, iid, ppret)) {
		ok = PR_TRUE; 
	} else {
		PyErr_Clear();

		obIID = Py_nsIID::PyObjectFromIID(iid);
		if (obIID==NULL) goto done;
		args = Py_BuildValue("OOzi", ob, obIID, NULL, 0);
		if (args==NULL) goto done;
		wrap_ret = PyEval_CallObject(func, args);
		if (wrap_ret==NULL) goto done;
		ok = Py_nsISupports::InterfaceFromPyObject(wrap_ret, iid, ppret, PR_FALSE, PR_FALSE);
#ifdef DEBUG
		if (ok)
		
		{
			nsISupports *temp = NULL;
			NS_ABORT_IF_FALSE(CheckDefaultGateway(ob, iid, &temp), "Auto-wrapped object didn't get a default gateway!");
			if (temp) temp->Release();
		}
#endif
	}
done:

	Py_XDECREF(obIID);
	Py_XDECREF(wrap_ret);
	Py_XDECREF(args);
	return ok;
}














PyObject *
PyG_Base::MakeInterfaceParam(nsISupports *pis, 
			     const nsIID *piid, 
			     int methodIndex ,
			     const XPTParamDescriptor *d , 
			     int paramIndex )
{
	if (pis==NULL) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	
	
	
	
	NS_ASSERTION( ((piid != NULL) ^ (d != NULL)) == 1, "No information on the interface available - Python's gunna have a hard time doing much with it!");
	PyObject *obIID = NULL;
	PyObject *obISupports = NULL;
	PyObject *obParamDesc = NULL;
	PyObject *result = NULL;

	
	
	nsCOMPtr<nsISupports> piswrap;
	nsIID iid_check;
	if (piid) {
		iid_check = *piid;
		piswrap = pis;
	} else {
		iid_check = NS_GET_IID(nsISupports);
		pis->QueryInterface(iid_check, getter_AddRefs(piswrap));
	}

	obISupports = Py_nsISupports::PyObjectFromInterface(piswrap, iid_check, PR_FALSE);
	if (!obISupports)
		goto done;
	if (piid==NULL) {
		obIID = Py_None;
		Py_INCREF(Py_None);
	} else
		obIID = Py_nsIID::PyObjectFromIID(*piid);
	if (obIID==NULL)
		goto done;
	obParamDesc = PyObject_FromXPTParamDescriptor(d);
	if (obParamDesc==NULL)
		goto done;

	result = PyObject_CallMethod(m_pPyObject, 
	                               "_MakeInterfaceParam_",
				       "OOiOi",
				       obISupports,
				       obIID,
				       methodIndex,
				       obParamDesc,
				       paramIndex);
done:
	if (PyErr_Occurred()) {
		NS_ASSERTION(result==NULL, "Have an error, but also a result!");
		PyXPCOM_LogError("Wrapping an interface object for the gateway failed\n");
	}
	Py_XDECREF(obIID);
	Py_XDECREF(obParamDesc);
	if (result==NULL) { 
		PyErr_Clear(); 
		
		return obISupports;
	}
	
	Py_XDECREF(obISupports);
	return result;
}

NS_IMETHODIMP
PyG_Base::QueryInterface(REFNSIID iid, void** ppv)
{
#ifdef PYXPCOM_DEBUG_FULL
	{
		char *sziid = iid.ToString();
		LogF("PyGatewayBase::QueryInterface: %s", sziid);
		Allocator::Free(sziid);
	}
#endif
	NS_PRECONDITION(ppv, "NULL pointer");
	if (ppv==nsnull)
		return NS_ERROR_NULL_POINTER;
	*ppv = nsnull;
	
	
	
	
	
	if ( (m_pBaseObject==NULL || !iid.Equals(NS_GET_IID(nsISupports)))
	   && (*ppv=ThisAsIID(iid)) != NULL ) {
		AddRef();
		return NS_OK;
	}
	
	
	if (m_pBaseObject != NULL)
		return m_pBaseObject->QueryInterface(iid, ppv);

	
	PRBool supports = PR_FALSE;
	{ 
		CEnterLeavePython celp;

		PyObject * ob = Py_nsIID::PyObjectFromIID(iid);
		
		
		PyObject * this_interface_ob = Py_nsISupports::PyObjectFromInterface(
		                                       (nsIXPTCProxy *)this,
		                                       iid, PR_FALSE, PR_TRUE);
		if ( !ob || !this_interface_ob) {
			Py_XDECREF(ob);
			Py_XDECREF(this_interface_ob);
			return NS_ERROR_OUT_OF_MEMORY;
		}

		PyObject *result = PyObject_CallMethod(m_pPyObject, "_QueryInterface_",
		                                                    "OO", 
		                                                    this_interface_ob, ob);
		Py_DECREF(ob);
		Py_DECREF(this_interface_ob);

		if ( result ) {
			if (Py_nsISupports::InterfaceFromPyObject(result, iid, (nsISupports **)ppv, PR_TRUE)) {
				
				
				supports = (*ppv!=NULL);
				
			} else {
				
				
				
				PyXPCOM_LogError("The _QueryInterface_ method returned an object of type '%s', but an interface was expected\n", result->ob_type->tp_name);
				
			}
			Py_DECREF(result);
		} else {
			NS_ABORT_IF_FALSE(PyErr_Occurred(), "Got NULL result, but no Python error flagged!");
			NS_ASSERTION(!supports, "Have failure with success flag set!");
			PyXPCOM_LogError("The _QueryInterface_ processing failed.\n");
			
			
			
			PyErr_Clear();
		}
	} 
	if ( !supports )
		return NS_ERROR_NO_INTERFACE;
	return NS_OK;
}

nsrefcnt
PyG_Base::AddRef(void)
{
	nsrefcnt cnt = (nsrefcnt) PR_AtomicIncrement((PRInt32*)&mRefCnt);
#ifdef NS_BUILD_REFCNT_LOGGING
	
	if (m_pBaseObject == NULL)
		NS_LOG_ADDREF(this, cnt, refcntLogRepr, sizeof(*this));
#endif
	return cnt;
}

nsrefcnt
PyG_Base::Release(void)
{
	nsrefcnt cnt = (nsrefcnt) PR_AtomicDecrement((PRInt32*)&mRefCnt);
#ifdef NS_BUILD_REFCNT_LOGGING
	if (m_pBaseObject == NULL)
		NS_LOG_RELEASE(this, cnt, refcntLogRepr);
#endif
	if ( cnt == 0 )
		delete this;
	return cnt;
}

NS_IMETHODIMP
PyG_Base::GetWeakReference(nsIWeakReference **ret)
{
	
	
	if (m_pBaseObject) {
		NS_PRECONDITION(m_pWeakRef == nsnull, "Not a base object, but do have a weak-ref!");
		return m_pBaseObject->GetWeakReference(ret);
	}
	NS_PRECONDITION(ret, "null pointer");
	if (ret==nsnull) return NS_ERROR_INVALID_POINTER;
	if (!m_pWeakRef) {
		
		
		m_pWeakRef = new PyXPCOM_GatewayWeakReference(this);
		NS_ABORT_IF_FALSE(m_pWeakRef, "Shouldn't be able to fail creating a weak reference!");
		if (!m_pWeakRef)
			return NS_ERROR_UNEXPECTED;
	}
	*ret = m_pWeakRef;
	(*ret)->AddRef();
	return NS_OK;
}

nsresult PyG_Base::HandleNativeGatewayError(const char *szMethodName)
{
	nsresult rc = NS_OK;
	if (PyErr_Occurred()) {
		
		
		
		
		
		
		
		

		
		
		

		
		

		PRBool bProcessMainError = PR_TRUE; 
		PyObject *exc_typ, *exc_val, *exc_tb;
		PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);

		PyObject *err_result = PyObject_CallMethod(m_pPyObject, 
	                                       "_GatewayException_",
					       "z(OOO)",
					       szMethodName,
		                               exc_typ ? exc_typ : Py_None, 
		                               exc_val ? exc_val : Py_None, 
					       exc_tb ? exc_tb : Py_None); 
		if (err_result == NULL) {
			PyXPCOM_LogError("The exception handler _CallMethodException_ failed!\n");
		} else if (err_result == Py_None) {
			
			
			;
		} else if (PyInt_Check(err_result)) {
			
			rc = PyInt_AsLong(err_result);
			bProcessMainError = PR_FALSE;
		} else {
			
			
			PyXPCOM_LogError("The _CallMethodException_ handler returned object of type '%s' - None or an integer expected\n", err_result->ob_type->tp_name);
		}
		Py_XDECREF(err_result);
		PyErr_Restore(exc_typ, exc_val, exc_tb);
		if (bProcessMainError) {
			PyXPCOM_LogError("The function '%s' failed\n", szMethodName);
			rc = PyXPCOM_SetCOMErrorFromPyException();
		}
		PyErr_Clear();
	}
	return rc;
}

static nsresult do_dispatch(
	PyObject *pPyObject,
	PyObject **ppResult,
	const char *szMethodName,
	const char *szFormat,
	va_list va
	)
{
	NS_PRECONDITION(ppResult, "Must provide a result buffer");
	*ppResult = nsnull;
	
	PyObject *args = NULL;
	PyObject *method = NULL;
	PyObject *real_ob = NULL;
	nsresult ret = NS_ERROR_FAILURE;
	if ( szFormat )
		args = Py_VaBuildValue((char *)szFormat, va);
	else
		args = PyTuple_New(0);
	if ( !args )
		goto done;

	
	if ( !PyTuple_Check(args) ) {
		PyObject *a = PyTuple_New(1);
		if ( a == NULL )
		{
			Py_DECREF(args);
			goto done;
		}
		PyTuple_SET_ITEM(a, 0, args);
		args = a;
	}
	
	
	
	real_ob = PyObject_GetAttrString(pPyObject, "_obj_");
	if (real_ob == NULL) {
		PyErr_Format(PyExc_AttributeError, "The policy object does not have an '_obj_' attribute.");
		goto done;
	}
	method = PyObject_GetAttrString(real_ob, (char *)szMethodName);
	if ( !method ) {
		PyErr_Clear();
		ret = NS_PYXPCOM_NO_SUCH_METHOD;
		goto done;
	}
	
	*ppResult = PyEval_CallObject(method, args);
	ret = *ppResult ? NS_OK : NS_ERROR_FAILURE;
done:
	Py_XDECREF(method);
	Py_XDECREF(real_ob);
	Py_XDECREF(args);
	return ret;
}


nsresult PyG_Base::InvokeNativeViaPolicyInternal(
	const char *szMethodName,
	PyObject **ppResult,
	const char *szFormat,
	va_list va
	)
{
	if ( m_pPyObject == NULL || szMethodName == NULL )
		return NS_ERROR_NULL_POINTER;

	PyObject *temp = nsnull;
	if (ppResult == nsnull)
		ppResult = &temp;
	nsresult nr = do_dispatch(m_pPyObject, ppResult, szMethodName, szFormat, va);

	
	
	Py_XDECREF(temp);
	return nr;
}

nsresult PyG_Base::InvokeNativeViaPolicy(
	const char *szMethodName,
	PyObject **ppResult ,
	const char *szFormat ,
	...
	)
{
	va_list va;
	va_start(va, szFormat);
	nsresult nr = InvokeNativeViaPolicyInternal(szMethodName, ppResult, szFormat, va);
	va_end(va);

	if (nr == NS_PYXPCOM_NO_SUCH_METHOD) {
		
		PyErr_Format(PyExc_AttributeError, "The object does not have a '%s' function.", szMethodName);
	}
	return nr == NS_OK ? NS_OK : HandleNativeGatewayError(szMethodName);
}


PyObject *PyG_Base::UnwrapPythonObject(void)
{
    Py_INCREF(m_pPyObject);
    return m_pPyObject;
}





































PyG_Base *GetDefaultGateway(PyObject *policy)
{
	
	PyObject *instance = PyObject_GetAttrString(policy, "_obj_");
	if (instance == nsnull)
		return nsnull;
	PyObject *ob_existing_weak = PyObject_GetAttrString(instance, PyXPCOM_szDefaultGatewayAttributeName);
	Py_DECREF(instance);
	if (ob_existing_weak != NULL) {
		PRBool ok = PR_TRUE;
		nsCOMPtr<nsIWeakReference> pWeakRef;
		ok = NS_SUCCEEDED(Py_nsISupports::InterfaceFromPyObject(ob_existing_weak, 
		                                       NS_GET_IID(nsIWeakReference), 
		                                       getter_AddRefs(pWeakRef),
		                                       PR_FALSE));
		Py_DECREF(ob_existing_weak);
		nsISupports *pip;
		if (ok) {
			nsresult nr = pWeakRef->QueryReferent( NS_GET_IID(nsIInternalPython), (void **)&pip);
			if (NS_FAILED(nr))
				return nsnull;
			return (PyG_Base *)(nsIInternalPython *)pip;
		}
	} else
		PyErr_Clear();
	return nsnull;
}

PRBool CheckDefaultGateway(PyObject *real_inst, REFNSIID iid, nsISupports **ret_gateway)
{
	NS_ABORT_IF_FALSE(real_inst, "Did not have an _obj_ attribute");
	if (real_inst==NULL) {
		PyErr_Clear();
		return PR_FALSE;
	}
	PyObject *ob_existing_weak = PyObject_GetAttrString(real_inst, PyXPCOM_szDefaultGatewayAttributeName);
	if (ob_existing_weak != NULL) {
		
		
		PRBool ok = PR_TRUE;
		nsCOMPtr<nsIWeakReference> pWeakRef;
		ok = NS_SUCCEEDED(Py_nsISupports::InterfaceFromPyObject(ob_existing_weak, 
		                                       NS_GET_IID(nsIWeakReference), 
		                                       getter_AddRefs(pWeakRef), 
		                                       PR_FALSE));
		Py_DECREF(ob_existing_weak);
		if (ok) {
			Py_BEGIN_ALLOW_THREADS;
			ok = NS_SUCCEEDED(pWeakRef->QueryReferent( iid, (void **)(ret_gateway)));
			Py_END_ALLOW_THREADS;
		}
		if (!ok) {
			
			
			if (0 != PyObject_DelAttrString(real_inst, PyXPCOM_szDefaultGatewayAttributeName))
				PyErr_Clear();
		}
		return ok;
	}
	PyErr_Clear();
	return PR_FALSE;
}

PYXPCOM_EXPORT void AddDefaultGateway(PyObject *instance, nsISupports *gateway)
{
	
	PyObject *real_inst = PyObject_GetAttrString(instance, "_obj_");
	NS_ABORT_IF_FALSE(real_inst, "Could not get the '_obj_' element");
	if (!real_inst) return;
	if (!PyObject_HasAttrString(real_inst, PyXPCOM_szDefaultGatewayAttributeName)) {
		nsCOMPtr<nsISupportsWeakReference> swr( do_QueryInterface((nsISupportsWeakReference *)(gateway)) );
		NS_ABORT_IF_FALSE(swr, "Our gateway failed with a weak reference query");
		
		if (swr) {
			nsCOMPtr<nsIWeakReference> pWeakReference;
			swr->GetWeakReference( getter_AddRefs(pWeakReference) );
			if (pWeakReference) {
				PyObject *ob_new_weak = Py_nsISupports::PyObjectFromInterface(pWeakReference, 
										   NS_GET_IID(nsIWeakReference),
										   PR_FALSE ); 
				
				if (ob_new_weak) {
					PyObject_SetAttrString(real_inst, PyXPCOM_szDefaultGatewayAttributeName, ob_new_weak);
					Py_DECREF(ob_new_weak);
				}
			}
		}
	}
	Py_DECREF(real_inst);
}
