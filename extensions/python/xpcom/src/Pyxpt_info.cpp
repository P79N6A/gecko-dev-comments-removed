














































#include "PyXPCOM_std.h"

PyObject *PyObject_FromXPTType( const nsXPTType *d)
{
	if (d==nsnull) {
		Py_INCREF(Py_None);
		return Py_None;
	}
    
	return Py_BuildValue("bzzz", 
		d->flags,
		NULL, NULL, NULL);
}

PyObject *PyObject_FromXPTTypeDescriptor( const XPTTypeDescriptor *d)
{
	if (d==nsnull) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return Py_BuildValue("bbbh", 
		d->prefix.flags,
		d->argnum,
		d->argnum2,
		d->type.iface 
		);
}

PyObject *PyObject_FromXPTParamDescriptor( const XPTParamDescriptor *d)
{
	if (d==nsnull) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	PyObject *ob = PyObject_FromXPTTypeDescriptor(&d->type);
	PyObject *ret = Py_BuildValue("bO", d->flags, ob);
	Py_DECREF(ob);
	return ret;
}

PyObject *PyObject_FromXPTMethodDescriptor( const XPTMethodDescriptor *d)
{
	if (d==nsnull) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	PyObject *ob_params = PyTuple_New(d->num_args);
	if (ob_params==NULL)
		return NULL;
	for (int i=0;i<d->num_args;i++)
		PyTuple_SET_ITEM(ob_params, i, PyObject_FromXPTParamDescriptor(d->params+i));
	PyObject *ob_ret = PyObject_FromXPTParamDescriptor(d->result);
	PyObject *ret = Py_BuildValue("bsOO", d->flags, d->name, ob_params, ob_ret);
	Py_XDECREF(ob_ret);
	Py_XDECREF(ob_params);
	return ret;
}

PyObject *PyObject_FromXPTConstant( const XPTConstDescriptor *c)
{
	if (c==nsnull) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	PyObject *ob_type = PyObject_FromXPTTypeDescriptor(&c->type);
	if (ob_type==NULL)
		return NULL;
	PyObject *v = NULL;
	switch (c->type.prefix.flags) {
		case TD_INT8:
			v = PyInt_FromLong( c->value.i8 );
			break;
		case TD_INT16:
			v = PyInt_FromLong( c->value.i16 );
			break;
		case TD_INT32:
			v = PyInt_FromLong( c->value.i32 );
			break;
		case TD_INT64:
			v = PyLong_FromLongLong(c->value.i64);
			break;
		case TD_UINT8:
			v = PyInt_FromLong( c->value.ui8 );
			break;
		case TD_UINT16:
			v = PyInt_FromLong( c->value.ui16 );
			break;
		case TD_UINT32:
			v = PyInt_FromLong( c->value.ui32 );
			break;
		case TD_UINT64:
			v = PyLong_FromUnsignedLongLong(c->value.ui64);
			break;
		case TD_FLOAT:
			v = PyFloat_FromDouble(c->value.flt);
			break;
		case TD_DOUBLE:
			v = PyFloat_FromDouble(c->value.dbl);
			break;
		case TD_BOOL:
			v = c->value.bul ? Py_True : Py_False;
			Py_INCREF(v);
			break;
		case TD_CHAR:
			v = PyString_FromStringAndSize(&c->value.ch, 1);
			break;
		case TD_WCHAR:
			v = PyObject_FromNSString((PRUnichar *)&c->value.wch, 1);
			break;
	
		case TD_PNSIID:
			v = Py_nsIID::PyObjectFromIID(*c->value.iid);
			break;
	
		case TD_PSTRING:
			v = PyString_FromString(c->value.str);
			break;
		case TD_PWSTRING:
			v = PyObject_FromNSString((PRUnichar *)c->value.wstr, nsCRT::strlen((PRUnichar *)c->value.wstr));
			break;
	
	
	
	
	
	
	
	
		default:
			v = PyString_FromString("Unknown type code!!");
			break;

	}
	PyObject *ret = Py_BuildValue("sbO", c->name, ob_type, v);
	Py_DECREF(ob_type);
	Py_DECREF(v);
	return ret;
}
