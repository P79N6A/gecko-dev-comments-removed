









































#ifndef _VERREG_H_
#define _VERREG_H_

#include "NSReg.h"

typedef struct _version
{
   int32   major;
   int32   minor;
   int32   release;
   int32   build;
   int32   check;
} VERSION;



#define CR_NEWREGISTRY 1

PR_BEGIN_EXTERN_C






VR_INTERFACE(REGERR) VR_CreateRegistry(char *installation, char *programPath, char *versionStr);
VR_INTERFACE(REGERR) VR_SetRegDirectory(const char *path);
VR_INTERFACE(REGERR) VR_PackRegistry(void *userData,  nr_RegPackCallbackFunc pdCallbackFunction);
VR_INTERFACE(REGERR) VR_Close(void);


VR_INTERFACE(REGERR) VR_Install(char *component_path, char *filepath, char *version, int bDirectory);
VR_INTERFACE(REGERR) VR_Remove(char *component_path);
VR_INTERFACE(REGERR) VR_InRegistry(char *path);
VR_INTERFACE(REGERR) VR_ValidateComponent(char *path);
VR_INTERFACE(REGERR) VR_Enum(char *component_path, REGENUM *state, char *buffer, uint32 buflen);


VR_INTERFACE(REGERR) VR_GetVersion(char *component_path, VERSION *result);
VR_INTERFACE(REGERR) VR_GetPath(char *component_path, uint32 sizebuf, char *buf);
VR_INTERFACE(REGERR) VR_SetRefCount(char *component_path, int refcount);
VR_INTERFACE(REGERR) VR_GetRefCount(char *component_path, int *result);
VR_INTERFACE(REGERR) VR_GetDefaultDirectory(char *component_path, uint32 sizebuf, char *buf);
VR_INTERFACE(REGERR) VR_SetDefaultDirectory(char *component_path, char *directory);


VR_INTERFACE(REGERR) VR_UninstallCreateNode(char *regPackageName, char *userPackageName);
VR_INTERFACE(REGERR) VR_UninstallAddFileToList(char *regPackageName, char *vrName);
VR_INTERFACE(REGERR) VR_UninstallFileExistsInList(char *regPackageName, char *vrName);
VR_INTERFACE(REGERR) VR_UninstallEnumSharedFiles(char *component_path, REGENUM *state, char *buffer, uint32 buflen);
VR_INTERFACE(REGERR) VR_UninstallDeleteFileFromList(char *component_path, char *vrName);
VR_INTERFACE(REGERR) VR_UninstallDeleteSharedFilesKey(char *regPackageName);
VR_INTERFACE(REGERR) VR_UninstallDestroy(char *regPackageName);
VR_INTERFACE(REGERR) VR_EnumUninstall(REGENUM *state, char* userPackageName,
                                    int32 len1, char*regPackageName, int32 len2, PRBool bSharedList);
VR_INTERFACE(REGERR) VR_GetUninstallUserName(char *regPackageName, char *outbuf, uint32 buflen);

PR_END_EXTERN_C

#endif   



