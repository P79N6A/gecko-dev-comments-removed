














































#include "PyXPCOM_std.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsILocalFile.h"
#include "nsITimelineService.h"

#include "nspr.h" 

#ifdef XP_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "windows.h"
#endif

#ifdef XP_UNIX
#include <dlfcn.h>
#include <sys/stat.h>
#endif

static PRLock *g_lockMain = nsnull;

PYXPCOM_EXPORT PyObject *PyXPCOM_Error = NULL;
PYXPCOM_EXPORT PRBool PyXPCOM_ModuleInitialized = PR_FALSE;

PyXPCOM_INTERFACE_DEFINE(Py_nsIComponentManager, nsIComponentManager, PyMethods_IComponentManager)
PyXPCOM_INTERFACE_DEFINE(Py_nsIInterfaceInfoManager, nsIInterfaceInfoManager, PyMethods_IInterfaceInfoManager)
PyXPCOM_INTERFACE_DEFINE(Py_nsIEnumerator, nsIEnumerator, PyMethods_IEnumerator)
PyXPCOM_INTERFACE_DEFINE(Py_nsISimpleEnumerator, nsISimpleEnumerator, PyMethods_ISimpleEnumerator)
PyXPCOM_INTERFACE_DEFINE(Py_nsIInterfaceInfo, nsIInterfaceInfo, PyMethods_IInterfaceInfo)
PyXPCOM_INTERFACE_DEFINE(Py_nsIInputStream, nsIInputStream, PyMethods_IInputStream)
PyXPCOM_INTERFACE_DEFINE(Py_nsIClassInfo, nsIClassInfo, PyMethods_IClassInfo)
PyXPCOM_INTERFACE_DEFINE(Py_nsIVariant, nsIVariant, PyMethods_IVariant)




PYXPCOM_EXPORT void
PyXPCOM_AcquireGlobalLock(void)
{
	NS_PRECONDITION(g_lockMain != nsnull, "Cant acquire a NULL lock!");
	PR_Lock(g_lockMain);
}

PYXPCOM_EXPORT void
PyXPCOM_ReleaseGlobalLock(void)
{
	NS_PRECONDITION(g_lockMain != nsnull, "Cant release a NULL lock!");
	PR_Unlock(g_lockMain);
}



void AddStandardPaths()
{
	
	nsresult rv;
	nsCOMPtr<nsIFile> aFile;
	
	
	
	
	
	rv = NS_GetSpecialDirectory(NS_GRE_DIR, getter_AddRefs(aFile));
	if (NS_FAILED(rv)) {
		PyXPCOM_LogError("The Python XPCOM loader could not locate the 'bin' directory");
		return;
	}
	aFile->Append(NS_LITERAL_STRING("python"));
	nsAutoString pathBuf;
	aFile->GetPath(pathBuf);
	PyObject *obPath = PySys_GetObject("path");
	if (!obPath) {
		PyXPCOM_LogError("The Python XPCOM loader could not get the Python sys.path variable");
		return;
	}
	
	NS_LossyConvertUTF16toASCII pathCBuf(pathBuf);
#ifdef NS_DEBUG
	PR_fprintf(PR_STDERR,"The Python XPCOM loader is adding '%s' to sys.path\n",
	           pathCBuf.get());
#endif
	PyObject *newStr = PyString_FromString(pathCBuf.get());
	PyList_Insert(obPath, 0, newStr);
	Py_XDECREF(newStr);
	
	
	nsCAutoString cmdBuf(NS_LITERAL_CSTRING("import site;site.addsitedir(r'"));
	cmdBuf.Append(pathCBuf);
	cmdBuf.Append(NS_LITERAL_CSTRING("')\n"));
	if (0 != PyRun_SimpleString((char *)cmdBuf.get())) {
		PyXPCOM_LogError("The directory '%s' could not be added as a site directory", pathCBuf.get());
		PyErr_Clear();
	}
	
	
	PyObject *mod = PyImport_ImportModule("sitepyxpcom");
	if (NULL==mod) {
		if (!PyErr_ExceptionMatches(PyExc_ImportError))
			PyXPCOM_LogError("Failed to import 'sitepyxpcom'");
		PyErr_Clear();
	} else
		Py_DECREF(mod);
}

static PRBool bIsInitialized = PR_FALSE;


PYXPCOM_EXPORT void
PyXPCOM_EnsurePythonEnvironment(void)
{
	
	
	if (bIsInitialized)
		return;
	CEnterLeaveXPCOMFramework _celf;
	if (bIsInitialized)
		return; 

#if defined(XP_UNIX) && !defined(XP_MACOSX)
	












	dlopen(PYTHON_SO,RTLD_NOW | RTLD_GLOBAL);
#endif

	PRBool bDidInitPython = !Py_IsInitialized(); 
	if (bDidInitPython) {
		NS_TIMELINE_START_TIMER("PyXPCOM: Python initializing");
		Py_Initialize(); 
#ifndef NS_DEBUG
		Py_OptimizeFlag = 1;
#endif 
		
		
		PyEval_InitThreads();

		NS_TIMELINE_STOP_TIMER("PyXPCOM: Python initializing");
		NS_TIMELINE_MARK_TIMER("PyXPCOM: Python initializing");
	}
	
	NS_TIMELINE_START_TIMER("PyXPCOM: Python threadstate setup");
	PyGILState_STATE state = PyGILState_Ensure();
#ifdef MOZ_TIMELINE
	
	if (NULL==PyImport_ImportModule("timeline_hook")) {
		if (!PyErr_ExceptionMatches(PyExc_ImportError))
			PyXPCOM_LogError("Failed to import 'timeline_hook'");
		PyErr_Clear(); 
	}
#endif

	
	if (PySys_GetObject("argv")==NULL) {
		PyObject *path = PyList_New(0);
		PyObject *str = PyString_FromString("");
		PyList_Append(path, str);
		PySys_SetObject("argv", path);
		Py_XDECREF(path);
		Py_XDECREF(str);
	}

	
	AddStandardPaths();

	
	if (PyXPCOM_Error == NULL) {
		PRBool rc = PR_FALSE;
		PyObject *mod = NULL;

		mod = PyImport_ImportModule("xpcom");
		if (mod!=NULL) {
			PyXPCOM_Error = PyObject_GetAttrString(mod, "Exception");
			Py_DECREF(mod);
		}
		rc = (PyXPCOM_Error != NULL);
	}

	
	Py_nsISupports::InitType();
	Py_nsIComponentManager::InitType();
	Py_nsIInterfaceInfoManager::InitType();
	Py_nsIEnumerator::InitType();
	Py_nsISimpleEnumerator::InitType();
	Py_nsIInterfaceInfo::InitType();
	Py_nsIInputStream::InitType();
	Py_nsIClassInfo::InitType();
	Py_nsIVariant::InitType();

	bIsInitialized = PR_TRUE;
	
	
	
	PyImport_ImportModule("xpcom");

	
	
	
	PyGILState_Release(bDidInitPython ? PyGILState_UNLOCKED : state);

	NS_TIMELINE_STOP_TIMER("PyXPCOM: Python threadstate setup");
	NS_TIMELINE_MARK_TIMER("PyXPCOM: Python threadstate setup");
}

void pyxpcom_construct(void)
{
	
	
	g_lockMain = PR_NewLock();
	return; 
}

void pyxpcom_destruct(void)
{
	PR_DestroyLock(g_lockMain);
}


struct DllInitializer {
	DllInitializer() {
		pyxpcom_construct();
	}
	~DllInitializer() {
		pyxpcom_destruct();
	}
} dll_initializer;
