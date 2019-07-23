




































#ifndef _NPPLAT_H_
#define _NPPLAT_H_

#include "npapi.h"
#include "npupp.h"






#ifdef XP_WIN
#include "windows.h"
#endif 






#ifdef XP_UNIX
#include <stdio.h>
#endif 






#ifdef XP_MAC

#include <Processes.h>
#include <Gestalt.h>
#include <CodeFragments.h>
#include <Timer.h>
#include <Resources.h>
#include <ToolUtils.h>

#include "jri.h"










#ifdef __MWERKS__
#ifndef powerc
#pragma pointers_in_D0
#endif
#endif

#ifdef __MWERKS__
#ifndef powerc
#pragma pointers_in_A0
#endif
#endif




#ifdef __MWERKS__
	#ifdef __cplusplus
	extern "C" {
	#endif
		#ifndef powerc
			extern void	__InitCode__(void);
		#else
			extern void __sinit(void);
			#define __InitCode__ __sinit
		#endif
		extern void	__destroy_global_chain(void);
	#ifdef __cplusplus
	}
	#endif 
#endif






NPError Private_Initialize(void);
void    Private_Shutdown(void);
NPError Private_New(NPMIMEType pluginType, NPP instance, uint16 mode, int16 argc, char* argn[], char* argv[], NPSavedData* saved);
NPError Private_Destroy(NPP instance, NPSavedData** save);
NPError Private_SetWindow(NPP instance, NPWindow* window);
NPError Private_NewStream(NPP instance, NPMIMEType type, NPStream* stream, NPBool seekable, uint16* stype);
NPError Private_DestroyStream(NPP instance, NPStream* stream, NPError reason);
int32   Private_WriteReady(NPP instance, NPStream* stream);
int32   Private_Write(NPP instance, NPStream* stream, int32 offset, int32 len, void* buffer);
void    Private_StreamAsFile(NPP instance, NPStream* stream, const char* fname);
void    Private_Print(NPP instance, NPPrint* platformPrint);
int16   Private_HandleEvent(NPP instance, void* event);
void    Private_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData);
jref    Private_GetJavaClass(void);
NPError Private_GetValue(NPP instance, NPPVariable variable, void *result);
NPError Private_SetValue(NPP instance, NPNVariable variable, void *value);

#endif

#ifndef HIBYTE
#define HIBYTE(i) (i >> 8)
#endif

#ifndef LOBYTE
#define LOBYTE(i) (i & 0xff)
#endif

#endif
