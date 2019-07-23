














































#include "PyXPCOM_std.h"
#include "nsStringAPI.h"
#include <nsIConsoleService.h>
#include "nspr.h" 

static char *PyTraceback_AsString(PyObject *exc_tb);





static void _PanicErrorWrite(const char *msg)
{
	nsCOMPtr<nsIConsoleService> consoleService = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
	if (consoleService)
		consoleService->LogStringMessage(NS_ConvertASCIItoUTF16(msg).get());
	PR_fprintf(PR_STDERR,"%s\n", msg);
}


static void HandleLogError(const char *pszMessageText)
{
	nsCAutoString streamout;

	_PanicErrorWrite("Failed to log an error record");
	if (PyXPCOM_FormatCurrentException(streamout))
		_PanicErrorWrite(streamout.get());
	_PanicErrorWrite("Original error follows:");
	_PanicErrorWrite(pszMessageText);
}

static const char *LOGGER_WARNING = "warning";
static const char *LOGGER_ERROR = "error";
#ifdef NS_DEBUG
static const char *LOGGER_DEBUG = "debug";
#endif

extern PRBool pyxpcom_initialized = PR_FALSE;


void DoLogMessage(const char *methodName, const char *pszMessageText)
{
	
	
	
	
	
	
	
	
	
	
	

	
	PyObject *exc_typ = NULL, *exc_val = NULL, *exc_tb = NULL;
	PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
	
	
	
	
	
	
	
	static PRBool initializedForLogging = PR_FALSE;
	if (PyXPCOM_ModuleInitialized && !initializedForLogging) {
		PyObject *mod = PyImport_ImportModule("logging");
		PyObject *logger = mod ?
		                   PyObject_CallMethod(mod, "getLogger", "s", "xpcom") :
		                   NULL;
		PyObject *handlers = PyObject_GetAttrString(logger, "handlers");
		if (handlers)
			initializedForLogging = PySequence_Check(handlers) &&
			                        PySequence_Length(handlers) > 0;
		Py_XDECREF(mod);
		Py_XDECREF(logger);
		Py_XDECREF(handlers);
		PyErr_Clear();
	}
	if (!initializedForLogging) {
		_PanicErrorWrite(pszMessageText);
		return;
	}




	nsCAutoString c("import logging\nlogging.getLogger('xpcom').");
	c += methodName;
	c += "('%s', ";
	
	PyObject *obMessage = PyString_FromString(pszMessageText);
	if (obMessage) {
		PyObject *repr = PyObject_Repr(obMessage);
		if (repr) {
			c += PyString_AsString(repr);
			Py_DECREF(repr);
		}
		Py_DECREF(obMessage);
	}
	c += ")\n";
	if (PyRun_SimpleString(c.get()) != 0) {
		HandleLogError(pszMessageText);
	}
	PyErr_Restore(exc_typ, exc_val, exc_tb);
}

void LogMessage(const char *methodName, const char *pszMessageText)
{
	
	
	PyObject *exc_typ = NULL, *exc_val = NULL, *exc_tb = NULL;
	PyErr_Fetch( &exc_typ, &exc_val, &exc_tb);
	DoLogMessage(methodName, pszMessageText);
	PyErr_Restore(exc_typ, exc_val, exc_tb);
}


void LogMessage(const char *methodName, nsACString &text)
{
	char *c = ToNewCString(text);
	LogMessage(methodName, c);
	nsCRT::free(c);
}


static void VLogF(const char *methodName, const char *fmt, va_list argptr)
{
	char buff[512];
	
	PR_vsnprintf(buff, sizeof(buff), fmt, argptr);

	LogMessage(methodName, buff);
}

PYXPCOM_EXPORT PRBool PyXPCOM_FormatCurrentException(nsCString &streamout)
{
	PRBool ok = PR_FALSE;
	PyObject *exc_typ = NULL, *exc_val = NULL, *exc_tb = NULL;
	PyErr_Fetch( &exc_typ, &exc_val, &exc_tb);
	PyErr_NormalizeException( &exc_typ, &exc_val, &exc_tb);
	if (exc_typ) {
		ok = PyXPCOM_FormatGivenException(streamout, exc_typ, exc_val,
						  exc_tb);
	}
	PyErr_Restore(exc_typ, exc_val, exc_tb);
	return ok;
}

PYXPCOM_EXPORT PRBool PyXPCOM_FormatGivenException(nsCString &streamout,
				    PyObject *exc_typ, PyObject *exc_val,
				    PyObject *exc_tb)
{
	if (!exc_typ)
		return PR_FALSE;
	streamout += "\n";

	if (exc_tb) {
		const char *szTraceback = PyTraceback_AsString(exc_tb);
		if (szTraceback == NULL)
			streamout += "Can't get the traceback info!";
		else {
			streamout += "Traceback (most recent call last):\n";
			streamout += szTraceback;
			PyMem_Free((void *)szTraceback);
		}
	}
	PyObject *temp = PyObject_Str(exc_typ);
	if (temp) {
		streamout += PyString_AsString(temp);
		Py_DECREF(temp);
	} else
		streamout += "Can't convert exception to a string!";
	streamout += ": ";
	if (exc_val != NULL) {
		temp = PyObject_Str(exc_val);
		if (temp) {
			streamout += PyString_AsString(temp);
			Py_DECREF(temp);
		} else
			streamout += "Can't convert exception value to a string!";
	}
	return PR_TRUE;
}

PYXPCOM_EXPORT void PyXPCOM_LogError(const char *fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);
	
	
	
	
	
	
	char buff[512];
	PR_vsnprintf(buff, sizeof(buff), fmt, marker);
	
	nsCAutoString streamout(buff);
	PyXPCOM_FormatCurrentException(streamout);
	LogMessage(LOGGER_ERROR, streamout);
}

PYXPCOM_EXPORT void PyXPCOM_LogWarning(const char *fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);
	VLogF(LOGGER_WARNING, fmt, marker);
}

PYXPCOM_EXPORT void PyXPCOM_Log(const char *level, const nsCString &msg)
{
	DoLogMessage(level, msg.get());
}

#ifdef DEBUG
PYXPCOM_EXPORT void PyXPCOM_LogDebug(const char *fmt, ...)
{
	va_list marker;
	va_start(marker, fmt);
	VLogF(LOGGER_DEBUG, fmt, marker);
}
#endif


PYXPCOM_EXPORT PyObject *PyXPCOM_BuildPyException(nsresult r)
{
	
	PyObject *evalue = Py_BuildValue("i", r);
	PyErr_SetObject(PyXPCOM_Error, evalue);
	Py_XDECREF(evalue);
	return NULL;
}

PYXPCOM_EXPORT nsresult PyXPCOM_SetCOMErrorFromPyException()
{
	if (!PyErr_Occurred())
		
		return NS_OK;
	nsresult rv = NS_ERROR_FAILURE;
	if (PyErr_ExceptionMatches(PyExc_MemoryError))
		rv = NS_ERROR_OUT_OF_MEMORY;
	
	

	
	
	
	PyErr_Clear();
	return rv;
}







#define TRACEBACK_FETCH_ERROR(what) {errMsg = what; goto done;}

char *PyTraceback_AsString(PyObject *exc_tb)
{
	char *errMsg = NULL; 
	char *result = NULL; 
	PyObject *modStringIO = NULL;
	PyObject *modTB = NULL;
	PyObject *obFuncStringIO = NULL;
	PyObject *obStringIO = NULL;
	PyObject *obFuncTB = NULL;
	PyObject *argsTB = NULL;
	PyObject *obResult = NULL;

	
	modStringIO = PyImport_ImportModule("cStringIO");
	if (modStringIO==NULL)
		TRACEBACK_FETCH_ERROR("cant import cStringIO\n");

	modTB = PyImport_ImportModule("traceback");
	if (modTB==NULL)
		TRACEBACK_FETCH_ERROR("cant import traceback\n");
	
	obFuncStringIO = PyObject_GetAttrString(modStringIO, "StringIO");
	if (obFuncStringIO==NULL)
		TRACEBACK_FETCH_ERROR("cant find cStringIO.StringIO\n");
	obStringIO = PyObject_CallObject(obFuncStringIO, NULL);
	if (obStringIO==NULL)
		TRACEBACK_FETCH_ERROR("cStringIO.StringIO() failed\n");
	
	obFuncTB = PyObject_GetAttrString(modTB, "print_tb");
	if (obFuncTB==NULL)
		TRACEBACK_FETCH_ERROR("cant find traceback.print_tb\n");

	argsTB = Py_BuildValue("OOO", 
			exc_tb  ? exc_tb  : Py_None,
			Py_None, 
			obStringIO);
	if (argsTB==NULL) 
		TRACEBACK_FETCH_ERROR("cant make print_tb arguments\n");

	obResult = PyObject_CallObject(obFuncTB, argsTB);
	if (obResult==NULL) 
		TRACEBACK_FETCH_ERROR("traceback.print_tb() failed\n");
	
	Py_DECREF(obFuncStringIO);
	obFuncStringIO = PyObject_GetAttrString(obStringIO, "getvalue");
	if (obFuncStringIO==NULL)
		TRACEBACK_FETCH_ERROR("cant find getvalue function\n");
	Py_DECREF(obResult);
	obResult = PyObject_CallObject(obFuncStringIO, NULL);
	if (obResult==NULL) 
		TRACEBACK_FETCH_ERROR("getvalue() failed.\n");

	
	if (!PyString_Check(obResult))
			TRACEBACK_FETCH_ERROR("getvalue() did not return a string\n");

	{ 
	char *tempResult = PyString_AsString(obResult);
	result = (char *)PyMem_Malloc(strlen(tempResult)+1);
	if (result==NULL)
		TRACEBACK_FETCH_ERROR("memory error duplicating the traceback string\n");

	strcpy(result, tempResult);
	} 
done:
	
	if (result==NULL && errMsg != NULL) {
		result = (char *)PyMem_Malloc(strlen(errMsg)+1);
		if (result != NULL)
			
			strcpy(result, errMsg);
	}
	Py_XDECREF(modStringIO);
	Py_XDECREF(modTB);
	Py_XDECREF(obFuncStringIO);
	Py_XDECREF(obStringIO);
	Py_XDECREF(obFuncTB);
	Py_XDECREF(argsTB);
	Py_XDECREF(obResult);
	return result;
}


PYXPCOM_EXPORT void PyXPCOM_MakePendingCalls()
{
	while (1) {
		int rc = Py_MakePendingCalls();
		if (rc == 0)
			break;
		
		
		PyXPCOM_LogError("Unhandled exception detected before entering Python.\n");
		PyErr_Clear();
		
	}
}
