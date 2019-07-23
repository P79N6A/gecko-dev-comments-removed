















































#include "PyXPCOM_std.h"
#include <nsIInterfaceInfoManager.h>

PyXPCOM_XPTStub::PyXPCOM_XPTStub(PyObject *instance, const nsIID &iid)
	: PyG_Base(instance, iid)
{
	if (NS_FAILED(InitStub(iid)))
		NS_ERROR("InitStub must not fail!");
}

void *PyXPCOM_XPTStub::ThisAsIID(const nsIID &iid)
{
	
	
	if (iid.Equals(NS_GET_IID(nsISupports)) || iid.Equals(m_iid)) {
		return mXPTCStub;
	}
	
	return PyG_Base::ThisAsIID(iid);
}



NS_IMETHODIMP
PyXPCOM_XPTStub::CallMethod(PRUint16 methodIndex,
                          const XPTMethodDescriptor* info,
                          nsXPTCMiniVariant* params)
{
	nsresult rc = NS_ERROR_FAILURE;
	NS_PRECONDITION(info, "NULL methodinfo pointer");
	NS_PRECONDITION(params, "NULL variant pointer");
	CEnterLeavePython _celp;
	PyObject *obParams = NULL;
	PyObject *result = NULL;
	PyObject *obThisObject = NULL;
	PyObject *obMI = PyObject_FromXPTMethodDescriptor(info);
	PyXPCOM_GatewayVariantHelper arg_helper(this, methodIndex, info, params);
	if (obMI==NULL)
		goto done;
	
	obThisObject = PyObject_FromNSInterface((nsISupports *)ThisAsIID(m_iid),
	                                        m_iid, PR_FALSE);
	obParams = arg_helper.MakePyArgs();
	if (obParams==NULL)
		goto done;
	result = PyObject_CallMethod(m_pPyObject, 
	                                       "_CallMethod_",
					       "OiOO",
					       obThisObject,
					       (int)methodIndex,
					       obMI,
					       obParams);
	if (result!=NULL) {
		rc = arg_helper.ProcessPythonResult(result);
		
		NS_ABORT_IF_FALSE( ((NS_FAILED(rc)!=0)^(PyErr_Occurred()!=0)) == 0, "We must have failure with a Python error, or success without a Python error.");
	}
done:
	if (PyErr_Occurred()) {
		
		
		
		
		
		
		
		

		
		
		

		
		

		PRBool bProcessMainError = PR_TRUE; 
		PyObject *exc_typ, *exc_val, *exc_tb;
		PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
		PyErr_NormalizeException( &exc_typ, &exc_val, &exc_tb);

		PyObject *err_result = PyObject_CallMethod(m_pPyObject, 
	                                       "_CallMethodException_",
					       "OiOO(OOO)",
					       obThisObject,
					       (int)methodIndex,
					       obMI,
					       obParams,
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
			PyXPCOM_LogError("The function '%s' failed\n", info->name);
			rc = PyXPCOM_SetCOMErrorFromPyException();
		}
		
		
		PyErr_Clear();
	}

	Py_XDECREF(obMI);
	Py_XDECREF(obParams);
	Py_XDECREF(obThisObject);
	Py_XDECREF(result);
	return rc;
}
