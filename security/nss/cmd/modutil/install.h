



































#ifndef PK11INSTALL_H
#define PK11INSTALL_H

#include <prio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Pk11Install_ErrorHandler)(char *);

typedef enum {
	PK11_INSTALL_NO_ERROR=0,
	PK11_INSTALL_DIR_DOESNT_EXIST,
	PK11_INSTALL_FILE_DOESNT_EXIST,
	PK11_INSTALL_FILE_NOT_READABLE,
	PK11_INSTALL_ERROR_STRING,
	PK11_INSTALL_JAR_ERROR,
	PK11_INSTALL_NO_INSTALLER_SCRIPT,
	PK11_INSTALL_DELETE_TEMP_FILE,
	PK11_INSTALL_OPEN_SCRIPT_FILE,
	PK11_INSTALL_SCRIPT_PARSE,
	PK11_INSTALL_SEMANTIC,
	PK11_INSTALL_SYSINFO,
	PK11_INSTALL_NO_PLATFORM,
	PK11_INSTALL_BOGUS_REL_DIR,
	PK11_INSTALL_NO_MOD_FILE,
	PK11_INSTALL_ADD_MODULE,
	PK11_INSTALL_JAR_EXTRACT,
	PK11_INSTALL_DIR_NOT_WRITEABLE,
	PK11_INSTALL_CREATE_DIR,
	PK11_INSTALL_REMOVE_DIR,
	PK11_INSTALL_EXEC_FILE,
	PK11_INSTALL_WAIT_PROCESS,
	PK11_INSTALL_PROC_ERROR,
	PK11_INSTALL_USER_ABORT,
	PK11_INSTALL_UNSPECIFIED
} Pk11Install_Error;
#define PK11_INSTALL_SUCCESS PK11_INSTALL_NO_ERROR









void 
Pk11Install_Init();








Pk11Install_ErrorHandler
Pk11Install_SetErrorHandler(Pk11Install_ErrorHandler handler);











void 
Pk11Install_Release();














Pk11Install_Error
Pk11Install_DoInstall(char *jarFile, const char *installDir,
	const char *tempDir, PRFileDesc *feedback, short force,
	PRBool noverify);

#ifdef __cplusplus
}
#endif

#endif
