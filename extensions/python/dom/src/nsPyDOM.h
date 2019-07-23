



































#ifndef __NSPYDOM_H__
#define __NSPYDOM_H__

#include "PyXPCOM.h"

class nsIScriptTimeoutHandler;
class nsIArray;

PyObject *PyObject_FromNSDOMInterface(PyObject *pycontext, nsISupports *pis,
                                      const nsIID &iid = NS_GET_IID(nsISupports),
                                      PRBool bMakeNicePyObject = PR_TRUE);

nsresult CreatePyTimeoutHandler(const nsAString &aExpr,
                                PyObject *aFunObj, PyObject *obArgs,
                                nsIScriptTimeoutHandler **aRet);

void PyInit_DOMnsISupports();






#define NS_IPYARGARRAY_IID \
 { /* {C169DFB6-BA7A-4337-AED6-EA791BB9C04E} */ \
 0xc169dfb6, 0xba7a, 0x4337, \
 { 0xae, 0xd6, 0xea, 0x79, 0x1b, 0xb9, 0xc0, 0x4e } }

class nsIPyArgArray: public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPYARGARRAY_IID)
  virtual PyObject *GetArgs() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPyArgArray, NS_IPYARGARRAY_IID)

nsresult NS_CreatePyArgv(PyObject *ob, nsIArray **aArray);

#endif 
