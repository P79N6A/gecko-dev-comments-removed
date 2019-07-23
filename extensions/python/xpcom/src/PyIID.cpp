

















































#include "PyXPCOM_std.h"
#include <nsIInterfaceInfoManager.h>

PYXPCOM_EXPORT nsIID Py_nsIID_NULL = {0,0,0,{0,0,0,0,0,0,0,0}};


PYXPCOM_EXPORT PyObject *PyXPCOMMethod_IID(PyObject *self, PyObject *args)
{
	PyObject *obIID;
	PyObject *obBuf;
	if ( PyArg_ParseTuple(args, "O", &obBuf)) {
		if (PyBuffer_Check(obBuf)) {
			PyBufferProcs *pb = NULL;
			pb = obBuf->ob_type->tp_as_buffer;
			void *buf = NULL;
			int size = (*pb->bf_getreadbuffer)(obBuf, 0, &buf);
			if (size != sizeof(nsIID) || buf==NULL) {
				PyErr_Format(PyExc_ValueError, "A buffer object to be converted to an IID must be exactly %d bytes long", sizeof(nsIID));
				return NULL;
			}
			nsIID iid;
			unsigned char *ptr = (unsigned char *)buf;
			iid.m0 = XPT_SWAB32(*((PRUint32 *)ptr));
			ptr = ((unsigned char *)buf) + offsetof(nsIID, m1);
			iid.m1 = XPT_SWAB16(*((PRUint16 *)ptr));
			ptr = ((unsigned char *)buf) + offsetof(nsIID, m2);
			iid.m2 = XPT_SWAB16(*((PRUint16 *)ptr));
			ptr = ((unsigned char *)buf) + offsetof(nsIID, m3);
			for (int i=0;i<8;i++) {
				iid.m3[i] = *((PRUint8 *)ptr);
				ptr += sizeof(PRUint8);
			}
			return new Py_nsIID(iid);
		}
	}
	PyErr_Clear();
	
	if ( !PyArg_ParseTuple(args, "O", &obIID) )
		return NULL;

	nsIID iid;
	if (!Py_nsIID::IIDFromPyObject(obIID, &iid))
		return NULL;
	return new Py_nsIID(iid);
}

 PRBool
Py_nsIID::IIDFromPyObject(PyObject *ob, nsIID *pRet) {
	PRBool ok = PR_TRUE;
	nsIID iid;
	if (ob==NULL) {
		PyErr_SetString(PyExc_RuntimeError, "The IID object is invalid!");
		return PR_FALSE;
	}
	if (PyString_Check(ob)) {
		ok = iid.Parse(PyString_AsString(ob));
		if (!ok) {
			PyErr_SetString(PyExc_ValueError, "The string is formatted as a valid nsID");
			return PR_FALSE;
		}
	} else if (ob->ob_type == &type) {
		iid = ((Py_nsIID *)ob)->m_iid;
	} else if (PyInstance_Check(ob)) {
		
		PyObject *use_ob = PyObject_GetAttrString(ob, "_iidobj_");
		if (use_ob==NULL) {
			PyErr_SetString(PyExc_TypeError, "Only instances with _iidobj_ attributes can be used as IID objects");
			return PR_FALSE;
		}
		if (use_ob->ob_type != &type) {
			Py_DECREF(use_ob);
			PyErr_SetString(PyExc_TypeError, "instance _iidobj_ attributes must be raw IID object");
			return PR_FALSE;
		}
		iid = ((Py_nsIID *)use_ob)->m_iid;
		Py_DECREF(use_ob);
	} else {
		PyErr_Format(PyExc_TypeError, "Objects of type '%s' can not be converted to an IID", ob->ob_type->tp_name);
		ok = PR_FALSE;
	}
	if (ok) *pRet = iid;
	return ok;
}






PYXPCOM_EXPORT PyTypeObject Py_nsIID::type =
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"IID",
	sizeof(Py_nsIID),
	0,
	PyTypeMethod_dealloc,                           
	0,                                              
	PyTypeMethod_getattr,                           
	0,                                              
	PyTypeMethod_compare,                           
	PyTypeMethod_repr,                              
	0,                                              
	0,                                              
	0,                                              
	PyTypeMethod_hash,                              
	0,                                              
	PyTypeMethod_str,                               
};

Py_nsIID::Py_nsIID(const nsIID &riid)
{
	ob_type = &type;
	_Py_NewReference(this);
	m_iid = riid;
}

PyObject *
Py_nsIID::PyTypeMethod_getattr(PyObject *self, char *name)
{
	Py_nsIID *me = (Py_nsIID *)self;
	if (strcmp(name, "name")==0) {
		char *iid_repr = nsnull;
		nsCOMPtr<nsIInterfaceInfoManager> iim(do_GetService(
		               NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
		if (iim!=nsnull)
			iim->GetNameForIID(&me->m_iid, &iid_repr);
		if (iid_repr==nsnull)
			iid_repr = me->m_iid.ToString();
		PyObject *ret;
		if (iid_repr != nsnull) {
			ret = PyString_FromString(iid_repr);
			nsMemory::Free(iid_repr);
		} else
			ret = PyString_FromString("<cant get IID info!>");
		return ret;
	}
	return PyErr_Format(PyExc_AttributeError, "IID objects have no attribute '%s'", name);
}

 int
Py_nsIID::PyTypeMethod_compare(PyObject *self, PyObject *other)
{
	Py_nsIID *s_iid = (Py_nsIID *)self;
	Py_nsIID *o_iid = (Py_nsIID *)other;
	int rc = memcmp(&s_iid->m_iid, &o_iid->m_iid, sizeof(s_iid->m_iid)); 
	return rc == 0 ? 0 : (rc < 0 ? -1 : 1);
}

 PyObject *
Py_nsIID::PyTypeMethod_repr(PyObject *self)
{
	Py_nsIID *s_iid = (Py_nsIID *)self;
	char buf[256];
	char *sziid = s_iid->m_iid.ToString();
	sprintf(buf, "_xpcom.IID('%s')", sziid);
	nsMemory::Free(sziid);
	return PyString_FromString(buf);
}

 PyObject *
Py_nsIID::PyTypeMethod_str(PyObject *self)
{
	Py_nsIID *s_iid = (Py_nsIID *)self;
	char *sziid = s_iid->m_iid.ToString();
	PyObject *ret = PyString_FromString(sziid);
	nsMemory::Free(sziid);
	return ret;
}

long
Py_nsIID::PyTypeMethod_hash(PyObject *self)
{
	const nsIID &iid = ((Py_nsIID *)self)->m_iid;

	long ret = iid.m0 + iid.m1 + iid.m2;
	for (int i=0;i<7;i++)
		ret += iid.m3[i];
	if ( ret == -1 )
		return -2;
	return ret;
}

 void
Py_nsIID::PyTypeMethod_dealloc(PyObject *ob)
{
	delete (Py_nsIID *)ob;
}
