




































#include <string.h>
#include <stdlib.h>

#include "prlink.h"
#include "prio.h"
#include "prlog.h"
#include "plstr.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcModuleReg.h"
#include "ipcModule.h"
#include "ipcCommandModule.h"
#include "ipcd.h"



struct ipcModuleRegEntry
{
    nsID              target;
    ipcModuleMethods *methods;
    PRLibrary        *lib;
};

#define IPC_MAX_MODULE_COUNT 64

static ipcModuleRegEntry ipcModules[IPC_MAX_MODULE_COUNT];
static int ipcModuleCount = 0;



static PRStatus
AddModule(const nsID &target, ipcModuleMethods *methods, const char *libPath)
{
    if (ipcModuleCount == IPC_MAX_MODULE_COUNT) {
        LOG(("too many modules!\n"));
        return PR_FAILURE;
    }

    if (!methods) {
        PR_NOT_REACHED("null module methods");
        return PR_FAILURE;
    }

    
    
    
    
    
    
    ipcModules[ipcModuleCount].target = target;
    ipcModules[ipcModuleCount].methods = methods;
    ipcModules[ipcModuleCount].lib = PR_LoadLibrary(libPath);

    ++ipcModuleCount;
    return PR_SUCCESS;
}

static void
InitModuleFromLib(const char *modulesDir, const char *fileName)
{
    LOG(("InitModuleFromLib [%s]\n", fileName));

    static const ipcDaemonMethods gDaemonMethods =
    {
        IPC_DAEMON_METHODS_VERSION,
        IPC_DispatchMsg,
        IPC_SendMsg,
        IPC_GetClientByID,
        IPC_GetClientByName,
        IPC_EnumClients,
        IPC_GetClientID,
        IPC_ClientHasName,
        IPC_ClientHasTarget,
        IPC_EnumClientNames,
        IPC_EnumClientTargets
    };

    int dLen = strlen(modulesDir);
    int fLen = strlen(fileName);

    char *buf = (char *) malloc(dLen + 1 + fLen + 1);
    memcpy(buf, modulesDir, dLen);
    buf[dLen] = IPC_PATH_SEP_CHAR;
    memcpy(buf + dLen + 1, fileName, fLen);
    buf[dLen + 1 + fLen] = '\0';

    PRLibrary *lib = PR_LoadLibrary(buf);
    if (lib) {
        ipcGetModulesFunc func =
            (ipcGetModulesFunc) PR_FindFunctionSymbol(lib, "IPC_GetModules");

        LOG(("  func=%p\n", (void*) func));

        if (func) {
            const ipcModuleEntry *entries = NULL;
            int count = func(&gDaemonMethods, &entries);
            for (int i=0; i<count; ++i) {
                if (AddModule(entries[i].target, entries[i].methods, buf) == PR_SUCCESS) {
                    if (entries[i].methods->init)
                        entries[i].methods->init();
                }
            }
        }
        PR_UnloadLibrary(lib);
    }

    free(buf);
}





void
IPC_InitModuleReg(const char *exePath)
{
    if (!(exePath && *exePath))
        return;

    
    
    
    char *p = PL_strrchr(exePath, IPC_PATH_SEP_CHAR);
    if (p == NULL) {
        LOG(("unexpected exe path\n"));
        return;
    }

    int baseLen = p - exePath;
    int finalLen = baseLen + 1 + sizeof(IPC_MODULES_DIR);

    
    char *modulesDir = (char*) malloc(finalLen);
    memcpy(modulesDir, exePath, baseLen);
    modulesDir[baseLen] = IPC_PATH_SEP_CHAR;
    memcpy(modulesDir + baseLen + 1, IPC_MODULES_DIR, sizeof(IPC_MODULES_DIR));

    LOG(("loading libraries in %s\n", modulesDir));
    
    
    
    PRDir *dir = PR_OpenDir(modulesDir);
    if (dir) {
        PRDirEntry *ent;
        while ((ent = PR_ReadDir(dir, PR_SKIP_BOTH)) != NULL) {
            
            
            

            const char *p = strrchr(ent->name, '.');
            if (p && PL_strcasecmp(p, MOZ_DLL_SUFFIX) == 0)
                InitModuleFromLib(modulesDir, ent->name);
        }
        PR_CloseDir(dir);
    }

    free(modulesDir);
}

void
IPC_ShutdownModuleReg()
{
    
    
    
    while (ipcModuleCount) {
        ipcModuleRegEntry &entry = ipcModules[--ipcModuleCount];
        if (entry.methods->shutdown)
            entry.methods->shutdown();
        if (entry.lib)
            PR_UnloadLibrary(entry.lib);
    }
}

void
IPC_NotifyModulesClientUp(ipcClient *client)
{
    for (int i = 0; i < ipcModuleCount; ++i) {
        ipcModuleRegEntry &entry = ipcModules[i];
        if (entry.methods->clientUp)
            entry.methods->clientUp(client);
    }
}

void
IPC_NotifyModulesClientDown(ipcClient *client)
{
    for (int i = 0; i < ipcModuleCount; ++i) {
        ipcModuleRegEntry &entry = ipcModules[i];
        if (entry.methods->clientDown)
            entry.methods->clientDown(client);
    }
}

PRStatus
IPC_DispatchMsg(ipcClient *client, const nsID &target, const void *data, PRUint32 dataLen)
{
    
    for (int i=0; i<ipcModuleCount; ++i) {
        ipcModuleRegEntry &entry = ipcModules[i];
        if (entry.target.Equals(target)) {
            if (entry.methods->handleMsg)
                entry.methods->handleMsg(client, target, data, dataLen);
        }
    }
    return PR_SUCCESS;
}
