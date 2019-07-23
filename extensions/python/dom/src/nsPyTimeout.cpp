



































#include "nsPyDOM.h"
#include "nsIProgrammingLanguage.h"
#include "nsIScriptTimeoutHandler.h"
#include "nsIArray.h"

class nsPyTimeoutHandler: public nsIScriptTimeoutHandler
{
public:
  
  NS_DECL_ISUPPORTS

  nsPyTimeoutHandler(const nsAString &aExpr, PyObject *aFunObj,
                     PyObject *obArgv) {
    mFunObj = aFunObj;
    Py_XINCREF(mFunObj);
    mExpr = aExpr;
    NS_ASSERTION(PyTuple_Check(obArgv), "Should be a tuple!");
    
    
    
    mObArgv = obArgv;
	Py_INCREF(mObArgv);
    
    mLateness = 0;
  }
  ~nsPyTimeoutHandler() {
    Py_XDECREF(mFunObj);
    Py_XDECREF(mObArgv);
  }

  virtual const PRUnichar *GetHandlerText() {
    return mExpr.get();
  }
  virtual void *GetScriptObject() {
    return mFunObj;
  }
  virtual void GetLocation(const char **aFileName, PRUint32 *aLineNo) {
    *aFileName = mFileName.get();
    *aLineNo = mLineNo;
  }
  virtual nsIArray *GetArgv();

  virtual PRUint32 GetScriptTypeID() {
        return nsIProgrammingLanguage::PYTHON;
  }
  virtual PRUint32 GetScriptVersion() {
        return 0;
  }

  
  
  virtual void SetLateness(PRIntervalTime aHowLate) {
    mLateness = aHowLate;
  }


private:

  
  nsCAutoString mFileName;
  PRUint32 mLineNo;
  PRIntervalTime mLateness;

  nsCOMPtr<nsIArray> mArrayArgv;
  PyObject *mObArgv;
  
  nsAutoString mExpr;
  PyObject *mFunObj;
};



NS_INTERFACE_MAP_BEGIN(nsPyTimeoutHandler)
  NS_INTERFACE_MAP_ENTRY(nsIScriptTimeoutHandler)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPyTimeoutHandler)
NS_IMPL_RELEASE(nsPyTimeoutHandler)


nsIArray *nsPyTimeoutHandler::GetArgv() {
  
  
  
  CEnterLeavePython _celp;
  if (!PyTuple_Check(mObArgv)) {
    NS_ERROR("Must be a tuple");
    return nsnull;
  }
  int argc_orig = PyTuple_Size(mObArgv);
  PyObject *obNew = PyTuple_New(argc_orig+1);
  for (int i=0;i<argc_orig;i++) {
    PyTuple_SET_ITEM(obNew, i, PyTuple_GET_ITEM(mObArgv, i));
    Py_INCREF(PyTuple_GET_ITEM(obNew, i));
  }
  PyTuple_SET_ITEM(obNew, argc_orig, PyInt_FromLong(mLateness));
  nsresult nr = NS_CreatePyArgv(obNew, getter_AddRefs(mArrayArgv));
  NS_ASSERTION(NS_SUCCEEDED(nr), "Failed to create argv!?");
  Py_DECREF(obNew); 
  return mArrayArgv;
}


nsresult CreatePyTimeoutHandler(const nsAString &aExpr,
                                PyObject *aFunObj, PyObject *obArgs,
                                nsIScriptTimeoutHandler **aRet) {

  *aRet = new nsPyTimeoutHandler(aExpr, aFunObj, obArgs);
  if (!aRet)
    return NS_ERROR_OUT_OF_MEMORY;
  return (*aRet)->QueryInterface(NS_GET_IID(nsIScriptTimeoutHandler),
                                 reinterpret_cast<void **>(aRet));
}
