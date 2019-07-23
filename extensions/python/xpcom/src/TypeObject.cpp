














































#include "PyXPCOM_std.h"
#include <nsIInterfaceInfoManager.h>
#include <nsXPCOM.h>
#include <nsISupportsPrimitives.h>


static PyTypeObject PyInterfaceType_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			
	"interface-type",			
	sizeof(PyTypeObject),	
	0,			
	0,			
	0,			
	PyType_Type.tp_getattr, 
	0,			
	0,			
	PyType_Type.tp_repr,	
	0,			
	0,			
	0,			
	0,			
	0,			
	0,			
	0,			
	0,			
	0,			
	0,			
	"Define the behavior of a PythonCOM Interface type.",
};

 PRBool
PyXPCOM_TypeObject::IsType(PyTypeObject *t)
{
	return t->ob_type == &PyInterfaceType_Type;
}





PyObject *
PyXPCOM_TypeObject::Py_getattr(PyObject *self, char *name)
{
	return ((Py_nsISupports *)self)->getattr(name);
}

int
PyXPCOM_TypeObject::Py_setattr(PyObject *op, char *name, PyObject *v)
{
	return ((Py_nsISupports *)op)->setattr(name, v);
}


int
PyXPCOM_TypeObject::Py_cmp(PyObject *self, PyObject *other)
{
	
	
	
	nsISupports *pUnkOther;
	nsISupports *pUnkThis;
	if (!Py_nsISupports::InterfaceFromPyObject(self, NS_GET_IID(nsISupports), &pUnkThis, PR_FALSE))
		return -1;
	if (!Py_nsISupports::InterfaceFromPyObject(other, NS_GET_IID(nsISupports), &pUnkOther, PR_FALSE)) {
		pUnkThis->Release();
		return -1;
	}
	int rc = pUnkThis==pUnkOther ? 0 :
		(pUnkThis < pUnkOther ? -1 : 1);
	pUnkThis->Release();
	pUnkOther->Release();
	return rc;
}


long PyXPCOM_TypeObject::Py_hash(PyObject *self)
{
	
	nsISupports *pUnkThis;
	if (!Py_nsISupports::InterfaceFromPyObject(self, NS_GET_IID(nsISupports), &pUnkThis, PR_FALSE))
		return -1;
	long ret = _Py_HashPointer(pUnkThis);
	pUnkThis->Release();
	return ret;
}


PyObject *
PyXPCOM_TypeObject::Py_repr(PyObject *self)
{
	
	Py_nsISupports *pis = (Py_nsISupports *)self;
	
	char *iid_repr;
	nsCOMPtr<nsIInterfaceInfoManager> iim(do_GetService(
	                      NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
	if (iim!=nsnull)
		iim->GetNameForIID(&pis->m_iid, &iid_repr);
	if (iid_repr==nsnull)
		
		iid_repr = pis->m_iid.ToString();
	
	char buf[512];
	sprintf(buf, "<XPCOM object (%s) at 0x%p/0x%p>",
	        iid_repr, (void *)self, (void *)pis->m_obj.get());
	nsMemory::Free(iid_repr);
	return PyString_FromString(buf);
}

PyObject *
PyXPCOM_TypeObject::Py_str(PyObject *self)
{
	Py_nsISupports *pis = (Py_nsISupports *)self;
	nsresult rv;
	char *val = NULL;
	Py_BEGIN_ALLOW_THREADS;
	{ 
	nsCOMPtr<nsISupportsCString> ss( do_QueryInterface(pis->m_obj, &rv ));
	if (NS_SUCCEEDED(rv))
		rv = ss->ToString(&val);
	} 
	Py_END_ALLOW_THREADS;
	PyObject *ret;
	if (NS_FAILED(rv))
		ret = Py_repr(self);
	else
		ret = PyString_FromString(val);
	if (val) nsMemory::Free(val);
	return ret;
}

void
PyXPCOM_TypeObject::Py_dealloc(PyObject *self)
{
	delete (Py_nsISupports *)self;
}

PyXPCOM_TypeObject::PyXPCOM_TypeObject( const char *name, PyXPCOM_TypeObject *pBase, int typeSize, struct PyMethodDef* methodList, PyXPCOM_I_CTOR thector)
{
	static const PyTypeObject type_template = {
		PyObject_HEAD_INIT(&PyInterfaceType_Type)
		0,                                           
		"XPCOMTypeTemplate",                         
		sizeof(Py_nsISupports),                 
		0,                                           
		Py_dealloc,                                  
		0,                                           
		Py_getattr,                                  
		Py_setattr,                                  
		Py_cmp,                                      
		Py_repr,                                     
    		0,                                           
		0,                                           
		0,                                           
		Py_hash,                                     
		0,                                           
		Py_str,                                      
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
		0,                                           
	};

	*((PyTypeObject *)this) = type_template;

	chain.methods = methodList;
	chain.link = pBase ? &pBase->chain : NULL;

	baseType = pBase;
	ctor = thector;

	
	tp_name = (char *)name;
	tp_basicsize = typeSize;
}

PyXPCOM_TypeObject::~PyXPCOM_TypeObject()
{
}
