




































#include "nsPyDOM.h"
#include "nsIArray.h"

class nsPyArgArray : public nsIPyArgArray, public nsIArray {
public:
  nsPyArgArray(PyObject *ob);
  ~nsPyArgArray();
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIARRAY

  
  PyObject *GetArgs() {return mObject;}
protected:
  PyObject *mObject;
};

nsPyArgArray::nsPyArgArray(PyObject *ob) :
    mObject(ob)
{
  NS_ASSERTION(PySequence_Check(ob), "You won't get far without a sequence!");
  Py_INCREF(ob);
}

nsPyArgArray::~nsPyArgArray()
{
  Py_DECREF(mObject);
}


NS_INTERFACE_MAP_BEGIN(nsPyArgArray)
  NS_INTERFACE_MAP_ENTRY(nsIArray)
  NS_INTERFACE_MAP_ENTRY(nsIPyArgArray)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPyArgArray)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsPyArgArray)
NS_IMPL_RELEASE(nsPyArgArray)


NS_IMETHODIMP nsPyArgArray::GetLength(PRUint32 *aLength)
{
  CEnterLeavePython _celp;
  int size = PySequence_Length(mObject);
  if (size==-1) {
    return PyXPCOM_SetCOMErrorFromPyException();
  }
  *aLength = (PRUint32) size;
  return NS_OK;
}


NS_IMETHODIMP nsPyArgArray::QueryElementAt(PRUint32 index, const nsIID & uuid, void * *result)
{
  *result = nsnull;
  
  

  if (uuid.Equals(NS_GET_IID(nsIVariant)) || uuid.Equals(NS_GET_IID(nsISupports))) {
    CEnterLeavePython _celp;
    PyObject *sub = PySequence_GetItem(mObject, index);
    if (sub==NULL) {
        return PyXPCOM_SetCOMErrorFromPyException();
    }
    nsresult rv = PyObject_AsVariant(sub, (nsIVariant **)result);
    Py_DECREF(sub);
    return rv;
  }
  NS_WARNING("nsPyArgArray only handles nsIVariant");
  return NS_ERROR_NO_INTERFACE;
}


NS_IMETHODIMP nsPyArgArray::IndexOf(PRUint32 startIndex, nsISupports *element, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsPyArgArray::Enumerate(nsISimpleEnumerator **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


nsresult NS_CreatePyArgv(PyObject *ob, nsIArray **aArray)
{
  nsPyArgArray *ret = new nsPyArgArray(ob);
  if (ret == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  return ret->QueryInterface(NS_GET_IID(nsIArray), (void **)aArray);
}
