




































#ifndef _WIZVERREG_H_
#define _WIZVERREG_H_

int   VR_CreateRegistry(char *installation, char *programPath, char *versionStr);
int   VR_Install(char *component_path, char *filepath, char *version, int bDirectory);
int   VR_Close(void);

#endif 

