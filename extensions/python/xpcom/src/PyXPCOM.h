















































#ifndef __PYXPCOM_H__
#define __PYXPCOM_H__

#include "nsIAllocator.h"
#include "nsIWeakReference.h"
#include "nsIInterfaceInfoManager.h"
#include "nsIClassInfo.h"
#include "nsIComponentManager.h"
#include "nsIComponentManagerObsolete.h"
#include "nsIServiceManager.h"
#include "nsIInputStream.h"
#include "nsIVariant.h"
#include "nsIModule.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"

#include "nsCRT.h"
#include "nsXPTCUtils.h"
#include "xpt_xdr.h"

#ifdef HAVE_LONG_LONG
	
	
#	undef HAVE_LONG_LONG
#endif 

#ifdef _POSIX_C_SOURCE 
#	undef _POSIX_C_SOURCE
#endif 

#include <Python.h>



#ifdef BUILD_PYXPCOM
    
#   define PYXPCOM_EXPORT NS_EXPORT
#else
    
#   define PYXPCOM_EXPORT NS_IMPORT
#endif 


extern PYXPCOM_EXPORT nsIID Py_nsIID_NULL;

class Py_nsISupports;









#define NS_PYXPCOM_NO_SUCH_METHOD \
  NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_PYXPCOM, 0)


extern PYXPCOM_EXPORT PyObject *PyXPCOM_Error;





extern PYXPCOM_EXPORT PRBool PyXPCOM_ModuleInitialized;





PYXPCOM_EXPORT PyObject *PyXPCOM_BuildPyException(nsresult res);



PYXPCOM_EXPORT nsresult PyXPCOM_SetCOMErrorFromPyException();


PYXPCOM_EXPORT PRBool PyXPCOM_FormatCurrentException(nsCString &streamout);

PYXPCOM_EXPORT PRBool PyXPCOM_FormatGivenException(nsCString &streamout,
                                PyObject *exc_typ, PyObject *exc_val,
                                PyObject *exc_tb);









PYXPCOM_EXPORT void PyXPCOM_LogWarning(const char *fmt, ...);




PYXPCOM_EXPORT void PyXPCOM_LogError(const char *fmt, ...);


PYXPCOM_EXPORT void PyXPCOM_Log(const char *level, const nsCString &msg);

#ifdef DEBUG


PYXPCOM_EXPORT void PyXPCOM_LogDebug(const char *fmt, ...);
#define PYXPCOM_LOG_DEBUG PyXPCOM_LogDebug
#else
#define PYXPCOM_LOG_DEBUG()
#endif 



PYXPCOM_EXPORT PyObject *PyObject_FromNSString( const nsACString &s,
                                                PRBool bAssumeUTF8 = PR_FALSE );
PYXPCOM_EXPORT PyObject *PyObject_FromNSString( const nsAString &s );
PYXPCOM_EXPORT PyObject *PyObject_FromNSString( const PRUnichar *s,
                                                PRUint32 len = (PRUint32)-1);



PYXPCOM_EXPORT PRBool PyObject_AsNSString( PyObject *ob, nsAString &aStr);


PYXPCOM_EXPORT nsresult PyObject_AsVariant( PyObject *ob, nsIVariant **aRet);
PYXPCOM_EXPORT PyObject *PyObject_FromVariant( Py_nsISupports *parent,
                                               nsIVariant *v);


PYXPCOM_EXPORT PyObject *PyObject_FromNSInterface( nsISupports *aInterface,
                                                   const nsIID &iid, 
                                                   PRBool bMakeNicePyObject = PR_TRUE);









typedef Py_nsISupports* (* PyXPCOM_I_CTOR)(nsISupports *, const nsIID &);





class PYXPCOM_EXPORT PyXPCOM_TypeObject : public PyTypeObject {
public:
	PyXPCOM_TypeObject( 
		const char *name, 
		PyXPCOM_TypeObject *pBaseType, 
		int typeSize, 
		struct PyMethodDef* methodList,
		PyXPCOM_I_CTOR ctor);
	~PyXPCOM_TypeObject();

	PyMethodChain chain;
	PyXPCOM_TypeObject *baseType;
	PyXPCOM_I_CTOR ctor;

	static PRBool IsType(PyTypeObject *t);
	
	static void Py_dealloc(PyObject *ob);
	static PyObject *Py_repr(PyObject *ob);
	static PyObject *Py_str(PyObject *ob);
	static PyObject *Py_getattr(PyObject *self, char *name);
	static int Py_setattr(PyObject *op, char *name, PyObject *v);
	static int Py_cmp(PyObject *ob1, PyObject *ob2);
	static long Py_hash(PyObject *self);
};










class PYXPCOM_EXPORT Py_nsISupports : public PyObject
{
public:
	
	
	
	static PRBool Check( PyObject *ob, const nsIID &checkIID = Py_nsIID_NULL) {
		Py_nsISupports *self = static_cast<Py_nsISupports *>(ob);
		if (ob==NULL || !PyXPCOM_TypeObject::IsType(ob->ob_type ))
			return PR_FALSE;
		if (!checkIID.Equals(Py_nsIID_NULL))
			return self->m_iid.Equals(checkIID) != 0;
		return PR_TRUE;
	}
	
	static nsISupports *GetI(PyObject *self, nsIID *ret_iid = NULL);
	nsCOMPtr<nsISupports> m_obj;
	nsIID m_iid;

	
	
	
	
	
	
	
	
	
	
	
	static PyObject *PyObjectFromInterface(nsISupports *ps, 
	                                       const nsIID &iid, 
	                                       PRBool bMakeNicePyObject = PR_TRUE,
	                                       PRBool bIsInternalCall = PR_FALSE);

	
	
	
	
	
	
	
	
	
	static PRBool InterfaceFromPyObject(
		PyObject *ob,
		const nsIID &iid,
		nsISupports **ppret,
		PRBool bNoneOK,
		PRBool bTryAutoWrap = PR_TRUE);

	
	
	
	static PRBool InterfaceFromPyISupports(PyObject *ob, 
	                                       const nsIID &iid, 
	                                       nsISupports **ppv);

	static Py_nsISupports *Constructor(nsISupports *pInitObj, const nsIID &iid);
	
	static PyObject *QueryInterface(PyObject *self, PyObject *args);

	
	static NS_EXPORT_STATIC_MEMBER_(PyXPCOM_TypeObject) *type;
	static NS_EXPORT_STATIC_MEMBER_(PyMethodDef) methods[];
	static PyObject *mapIIDToType;
	static void SafeRelease(Py_nsISupports *ob);
	static void RegisterInterface( const nsIID &iid, PyTypeObject *t);
	static void InitType();

	virtual ~Py_nsISupports();
	virtual PyObject *getattr(const char *name);
	virtual int setattr(const char *name, PyObject *val);
	
	
	
	virtual PyObject *MakeInterfaceResult(nsISupports *ps, const nsIID &iid,
	                                      PRBool bMakeNicePyObject = PR_TRUE) {
		return PyObjectFromInterface(ps, iid, bMakeNicePyObject);
	}

protected:
	
	
	Py_nsISupports(nsISupports *p, 
		            const nsIID &iid, 
			    PyTypeObject *type);

	
	
	static PyObject *MakeDefaultWrapper(PyObject *pyis, const nsIID &iid);

};


class PYXPCOM_EXPORT Py_nsIID : public PyObject
{
public:
	Py_nsIID(const nsIID &riid);
	nsIID m_iid;

	PRBool 
	IsEqual(const nsIID &riid) {
		return m_iid.Equals(riid);
	}

	PRBool
	IsEqual(PyObject *ob) {
		return ob && 
		       ob->ob_type== &type && 
		       m_iid.Equals(((Py_nsIID *)ob)->m_iid);
	}

	PRBool
	IsEqual(Py_nsIID &iid) {
		return m_iid.Equals(iid.m_iid);
	}

	static PyObject *
	PyObjectFromIID(const nsIID &iid) {
		return new Py_nsIID(iid);
	}

	static PRBool IIDFromPyObject(PyObject *ob, nsIID *pRet);
	
	static PyObject *PyTypeMethod_getattr(PyObject *self, char *name);
	static int PyTypeMethod_compare(PyObject *self, PyObject *ob);
	static PyObject *PyTypeMethod_repr(PyObject *self);
	static long PyTypeMethod_hash(PyObject *self);
	static PyObject *PyTypeMethod_str(PyObject *self);
	static void PyTypeMethod_dealloc(PyObject *self);
	static NS_EXPORT_STATIC_MEMBER_(PyTypeObject) type;
	static NS_EXPORT_STATIC_MEMBER_(PyMethodDef) methods[];
};




class PythonTypeDescriptor; 

class PYXPCOM_EXPORT PyXPCOM_InterfaceVariantHelper {
public:
	PyXPCOM_InterfaceVariantHelper(Py_nsISupports *parent);
	~PyXPCOM_InterfaceVariantHelper();
	PRBool Init(PyObject *obParams);
	PRBool FillArray();

	PyObject *MakePythonResult();

	nsXPTCVariant *m_var_array;
	int m_num_array;
protected:
	PyObject *MakeSinglePythonResult(int index);
	PRBool FillInVariant(const PythonTypeDescriptor &, int, int);
	PRBool PrepareOutVariant(const PythonTypeDescriptor &td, int value_index);
	PRBool SetSizeIs( int var_index, PRBool is_arg1, PRUint32 new_size);
	PRUint32 GetSizeIs( int var_index, PRBool is_arg1);

	PyObject *m_pyparams; 
	PyObject *m_typedescs; 
	PythonTypeDescriptor *m_python_type_desc_array;
	void **m_buffer_array;
	Py_nsISupports *m_parent;

};








#define NS_IINTERNALPYTHON_IID_STR "AC7459FC-E8AB-4f2e-9C4F-ADDC53393A20"
#define NS_IINTERNALPYTHON_IID \
	{ 0xac7459fc, 0xe8ab, 0x4f2e, { 0x9c, 0x4f, 0xad, 0xdc, 0x53, 0x39, 0x3a, 0x20 } }

class PyXPCOM_GatewayWeakReference;





class nsIInternalPython : public nsISupports {
public: 
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_IINTERNALPYTHON_IID)
	
	virtual PyObject *UnwrapPythonObject(void) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIInternalPython, NS_IINTERNALPYTHON_IID)



class PYXPCOM_EXPORT PyG_Base : public nsIInternalPython, public nsISupportsWeakReference
{
public:
	NS_DECL_ISUPPORTS
	NS_DECL_NSISUPPORTSWEAKREFERENCE
	PyObject *UnwrapPythonObject(void);

	
	static nsresult CreateNew(PyObject *pPyInstance, 
		                  const nsIID &iid, 
				  void **ppResult);

	
	
	static PRBool AutoWrapPythonInstance(PyObject *ob, 
		                           const nsIID &iid, 
					   nsISupports **ppret);


	
	
	PyObject *MakeInterfaceParam(nsISupports *pis, 
	                                     const nsIID *piid, 
					     int methodIndex = -1,
					     const XPTParamDescriptor *d = NULL, 
					     int paramIndex = -1);

	
	
	virtual void *ThisAsIID( const nsIID &iid ) = 0;

	
	
	nsresult HandleNativeGatewayError(const char *szMethodName);

	
	nsIID m_iid;
	PyObject * m_pPyObject;
	
	
	
	
	
	nsCOMPtr<nsIWeakReference> m_pWeakRef;
#ifdef NS_BUILD_REFCNT_LOGGING
	char refcntLogRepr[64]; 
#endif
protected:
	PyG_Base(PyObject *instance, const nsIID &iid);
	virtual ~PyG_Base();
	PyG_Base *m_pBaseObject; 
	nsresult InvokeNativeViaPolicy(	const char *szMethodName,
			PyObject **ppResult = NULL,
			const char *szFormat = NULL,
			...
			);
	nsresult InvokeNativeViaPolicyInternal(	const char *szMethodName,
			PyObject **ppResult,
			const char *szFormat,
			va_list va);
};

class PYXPCOM_EXPORT PyXPCOM_XPTStub : public PyG_Base, public nsAutoXPTCStub
{
friend class PyG_Base;
public:
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr)
		{return PyG_Base::QueryInterface(aIID, aInstancePtr);}
	NS_IMETHOD_(nsrefcnt) AddRef(void) {return PyG_Base::AddRef();}
	NS_IMETHOD_(nsrefcnt) Release(void) {return PyG_Base::Release();}

	
	NS_IMETHOD CallMethod(PRUint16 methodIndex,
                          const XPTMethodDescriptor* info,
                          nsXPTCMiniVariant* params);

	virtual void *ThisAsIID(const nsIID &iid);
protected:
	PyXPCOM_XPTStub(PyObject *instance, const nsIID &iid);
private:
};


#define PYGATEWAY_BASE_SUPPORT(INTERFACE, GATEWAY_BASE)                    \
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr)      \
		{return PyG_Base::QueryInterface(aIID, aInstancePtr);}     \
	NS_IMETHOD_(nsrefcnt) AddRef(void) {return PyG_Base::AddRef();}    \
	NS_IMETHOD_(nsrefcnt) Release(void) {return PyG_Base::Release();}  \
	virtual void *ThisAsIID(const nsIID &iid) {                        \
		if (iid.Equals(NS_GET_IID(INTERFACE))) return (INTERFACE *)this; \
		return GATEWAY_BASE::ThisAsIID(iid);                       \
	}                                                                  \

extern PYXPCOM_EXPORT void AddDefaultGateway(PyObject *instance, nsISupports *gateway);

extern PYXPCOM_EXPORT PRInt32 _PyXPCOM_GetGatewayCount(void);
extern PYXPCOM_EXPORT PRInt32 _PyXPCOM_GetInterfaceCount(void);











class PYXPCOM_EXPORT PyXPCOM_GatewayWeakReference : public nsIWeakReference {
public:
	PyXPCOM_GatewayWeakReference(PyG_Base *base);
	virtual ~PyXPCOM_GatewayWeakReference();
	NS_DECL_ISUPPORTS
	NS_DECL_NSIWEAKREFERENCE
	PyG_Base *m_pBase; 
#ifdef NS_BUILD_REFCNT_LOGGING
	char refcntLogRepr[41];
#endif
};



class PYXPCOM_EXPORT PyXPCOM_GatewayVariantHelper
{
public:
	PyXPCOM_GatewayVariantHelper( PyG_Base *gateway,
	                              int methodIndex,
	                              const XPTMethodDescriptor *info, 
	                              nsXPTCMiniVariant* params );
	~PyXPCOM_GatewayVariantHelper();
	PyObject *MakePyArgs();
	nsresult ProcessPythonResult(PyObject *ob);
	PyG_Base *m_gateway;
private:
	nsresult BackFillVariant( PyObject *ob, int index);
	PyObject *MakeSingleParam(int index, PythonTypeDescriptor &td);
	PRBool GetIIDForINTERFACE_ID(int index, const nsIID **ppret);
	nsresult GetArrayType(PRUint8 index, PRUint8 *ret, nsIID **ppiid);
	PRUint32 GetSizeIs( int var_index, PRBool is_arg1);
	PRBool SetSizeIs( int var_index, PRBool is_arg1, PRUint32 new_size);
	PRBool CanSetSizeIs( int var_index, PRBool is_arg1 );
	nsIInterfaceInfo *GetInterfaceInfo(); 


	nsXPTCMiniVariant* m_params;
	const XPTMethodDescriptor *m_info;
	int m_method_index;
	PythonTypeDescriptor *m_python_type_desc_array;
	int m_num_type_descs;
	nsCOMPtr<nsIInterfaceInfo> m_interface_info;
};


PyObject *PyObject_FromXPTType( const nsXPTType *d);

PyObject *PyObject_FromXPTTypeDescriptor( const XPTTypeDescriptor *d);

PyObject *PyObject_FromXPTParamDescriptor( const XPTParamDescriptor *d);
PyObject *PyObject_FromXPTMethodDescriptor( const XPTMethodDescriptor *d);
PyObject *PyObject_FromXPTConstant( const XPTConstDescriptor *d);




void PyXPCOM_DLLAddRef();
void PyXPCOM_DLLRelease();
























PYXPCOM_EXPORT void PyXPCOM_AcquireGlobalLock(void);
PYXPCOM_EXPORT void PyXPCOM_ReleaseGlobalLock(void);






class CEnterLeaveXPCOMFramework {
public:
	CEnterLeaveXPCOMFramework() {PyXPCOM_AcquireGlobalLock();}
	~CEnterLeaveXPCOMFramework() {PyXPCOM_ReleaseGlobalLock();}
};



PYXPCOM_EXPORT void PyXPCOM_EnsurePythonEnvironment(void);

PYXPCOM_EXPORT void PyXPCOM_MakePendingCalls();



inline PRBool PyXPCOM_Globals_Ensure() {
    PyXPCOM_EnsurePythonEnvironment();
    return PR_TRUE;
}










class CEnterLeavePython {
public:
	CEnterLeavePython() {
		state = PyGILState_Ensure();
		
		
		if (PyThreadState_Get()->gilstate_counter==1) {
			PyXPCOM_MakePendingCalls();
		}
	}
	~CEnterLeavePython() {
		PyGILState_Release(state);
	}
	PyGILState_STATE state;
};




#define PyXPCOM_INTERFACE_DECLARE(ClassName, InterfaceName, Methods )     \
                                                                          \
extern struct PyMethodDef Methods[];                                      \
                                                                          \
class ClassName : public Py_nsISupports                                   \
{                                                                         \
public:                                                                   \
	static PYXPCOM_EXPORT PyXPCOM_TypeObject *type;                                  \
	static Py_nsISupports *Constructor(nsISupports *pInitObj, const nsIID &iid) { \
		return new ClassName(pInitObj, iid);                      \
	}                                                                 \
	static void InitType() {                                          \
		type = new PyXPCOM_TypeObject(                            \
				#InterfaceName,                           \
				Py_nsISupports::type,                     \
				sizeof(ClassName),                        \
				Methods,                                  \
				Constructor);                             \
		const nsIID &iid = NS_GET_IID(InterfaceName);             \
		RegisterInterface(iid, type);                             \
	}                                                                 \
protected:                                                                \
	ClassName(nsISupports *p, const nsIID &iid) :                     \
		Py_nsISupports(p, iid, type) {                            \
		/* The IID _must_ be the IID of the interface we are wrapping! */    \
		NS_ABORT_IF_FALSE(iid.Equals(NS_GET_IID(InterfaceName)), "Bad IID"); \
	}                                                                 \
};                                                                        \
                                                                          \
// End of PyXPCOM_INTERFACE_DECLARE macro

#define PyXPCOM_ATTR_INTERFACE_DECLARE(ClassName, InterfaceName, Methods )\
                                                                          \
extern struct PyMethodDef Methods[];                                      \
                                                                          \
class ClassName : public Py_nsISupports                                   \
{                                                                         \
public:                                                                   \
	static PYXPCOM_EXPORT PyXPCOM_TypeObject *type;                                  \
	static Py_nsISupports *Constructor(nsISupports *pInitObj, const nsIID &iid) { \
		return new ClassName(pInitObj, iid);                      \
	}                                                                 \
	static void InitType() {                                          \
		type = new PyXPCOM_TypeObject(                            \
				#InterfaceName,                           \
				Py_nsISupports::type,                     \
				sizeof(ClassName),                        \
				Methods,                                  \
				Constructor);                             \
		const nsIID &iid = NS_GET_IID(InterfaceName);             \
		RegisterInterface(iid, type);                             \
}                                                                         \
	virtual PyObject *getattr(const char *name);                      \
	virtual int setattr(const char *name, PyObject *val);             \
protected:                                                                \
	ClassName(nsISupports *p, const nsIID &iid) :                     \
		Py_nsISupports(p, iid, type) {                            \
		/* The IID _must_ be the IID of the interface we are wrapping! */    \
		NS_ABORT_IF_FALSE(iid.Equals(NS_GET_IID(InterfaceName)), "Bad IID"); \
	}                                                                 \
};                                                                        \
                                                                          \
// End of PyXPCOM_ATTR_INTERFACE_DECLARE macro

#define PyXPCOM_INTERFACE_DEFINE(ClassName, InterfaceName, Methods )      \
NS_EXPORT_STATIC_MEMBER_(PyXPCOM_TypeObject *) ClassName::type = NULL;



PyXPCOM_INTERFACE_DECLARE(Py_nsIComponentManager, nsIComponentManager, PyMethods_IComponentManager)
PyXPCOM_INTERFACE_DECLARE(Py_nsIInterfaceInfoManager, nsIInterfaceInfoManager, PyMethods_IInterfaceInfoManager)
PyXPCOM_INTERFACE_DECLARE(Py_nsIEnumerator, nsIEnumerator, PyMethods_IEnumerator)
PyXPCOM_INTERFACE_DECLARE(Py_nsISimpleEnumerator, nsISimpleEnumerator, PyMethods_ISimpleEnumerator)
PyXPCOM_INTERFACE_DECLARE(Py_nsIInterfaceInfo, nsIInterfaceInfo, PyMethods_IInterfaceInfo)
PyXPCOM_INTERFACE_DECLARE(Py_nsIInputStream, nsIInputStream, PyMethods_IInputStream)
PyXPCOM_ATTR_INTERFACE_DECLARE(Py_nsIClassInfo, nsIClassInfo, PyMethods_IClassInfo)
PyXPCOM_ATTR_INTERFACE_DECLARE(Py_nsIVariant, nsIVariant, PyMethods_IVariant)

PyXPCOM_INTERFACE_DECLARE(Py_nsIComponentManagerObsolete, nsIComponentManagerObsolete, PyMethods_IComponentManagerObsolete)
#endif
