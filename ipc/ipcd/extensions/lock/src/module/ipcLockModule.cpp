




































#include <stdlib.h>
#include <stdio.h>
#include "ipcModuleUtil.h"
#include "ipcLockProtocol.h"
#include "plhash.h"
#include "plstr.h"

#ifdef DEBUG
#define LOG(args) printf args
#else
#define LOG(args)
#endif

static const nsID kLockTargetID = IPC_LOCK_TARGETID;

static void
ipcLockModule_Send(PRUint32 cid, const char *key, PRUint8 opcode)
{
    ipcLockMsg msg = { opcode, 0, key };
    PRUint32 bufLen;
    PRUint8 *buf = IPC_FlattenLockMsg(&msg, &bufLen);
    if (!buf)
        return;
    IPC_SendMsg(cid, kLockTargetID, buf, bufLen);
    free(buf);
}






static PLHashTable *gLockTable = NULL;



struct ipcLockContext
{
    PRUint32               mOwnerID;     
    struct ipcLockContext *mNextPending; 
                                         

    ipcLockContext(PRUint32 ownerID)
        : mOwnerID(ownerID)
        , mNextPending(NULL) {}
};



PR_STATIC_CALLBACK(void *)
ipcLockModule_AllocTable(void *pool, PRSize size)
{
    return malloc(size);
}

PR_STATIC_CALLBACK(void)
ipcLockModule_FreeTable(void *pool, void *item)
{
    free(item);
}

PR_STATIC_CALLBACK(PLHashEntry *)
ipcLockModule_AllocEntry(void *pool, const void *key)
{
    return (PLHashEntry *) malloc(sizeof(PLHashEntry));
}

PR_STATIC_CALLBACK(void)
ipcLockModule_FreeEntry(void *pool, PLHashEntry *he, PRUintn flag)
{
    PL_strfree((char *) he->key);
    free(he);
}

static const PLHashAllocOps ipcLockModule_AllocOps = {
    ipcLockModule_AllocTable,
    ipcLockModule_FreeTable,
    ipcLockModule_AllocEntry,
    ipcLockModule_FreeEntry
};



static void
ipcLockModule_AcquireLock(PRUint32 cid, PRUint8 flags, const char *key)
{
    LOG(("$$$ acquiring lock [key=%s]\n", key));

    if (!gLockTable)
        return;

    ipcLockContext *ctx;
    
    ctx = (ipcLockContext *) PL_HashTableLookup(gLockTable, key);
    if (ctx) {
        
        
        
        
        while (ctx->mOwnerID != cid && ctx->mNextPending)
            ctx = ctx->mNextPending;
        if (ctx->mOwnerID != cid) {
            
            
            
            
            if (flags & IPC_LOCK_FL_NONBLOCKING)
                ipcLockModule_Send(cid, key, IPC_LOCK_OP_STATUS_BUSY);
            else
                ctx->mNextPending = new ipcLockContext(cid);
        }
    }
    else {
        
        
        
        
        ctx = new ipcLockContext(cid);
        if (!ctx)
            return;

        PL_HashTableAdd(gLockTable, PL_strdup(key), ctx);

        ipcLockModule_Send(cid, key, IPC_LOCK_OP_STATUS_ACQUIRED);
    }
}

static PRBool
ipcLockModule_ReleaseLockHelper(PRUint32 cid, const char *key, ipcLockContext *ctx)
{
    LOG(("$$$ releasing lock [key=%s]\n", key));

    PRBool removeEntry = PR_FALSE;

    
    
    
    if (ctx->mOwnerID == cid) {
        if (ctx->mNextPending) {
            
            
            
            
            
            
            ipcLockContext *next = ctx->mNextPending;
            ctx->mOwnerID = next->mOwnerID;
            ctx->mNextPending = next->mNextPending;
            delete next;
            
            
            
            ipcLockModule_Send(ctx->mOwnerID, key, IPC_LOCK_OP_STATUS_ACQUIRED);
        }
        else {
            delete ctx;
            removeEntry = PR_TRUE;
        }
    }
    else {
        ipcLockContext *prev;
        for (;;) {
            prev = ctx;
            ctx = ctx->mNextPending;
            if (!ctx)
                break;
            if (ctx->mOwnerID == cid) {
                
                prev->mNextPending = ctx->mNextPending;
                delete ctx;
                break;
            }
        }
    }

    return removeEntry;
}

static void
ipcLockModule_ReleaseLock(PRUint32 cid, const char *key)
{
    if (!gLockTable)
        return;

    ipcLockContext *ctx;

    ctx = (ipcLockContext *) PL_HashTableLookup(gLockTable, key);
    if (ctx && ipcLockModule_ReleaseLockHelper(cid, key, ctx))
        PL_HashTableRemove(gLockTable, key);
}

PR_STATIC_CALLBACK(PRIntn)
ipcLockModule_ReleaseByCID(PLHashEntry *he, PRIntn i, void *arg)
{
    PRUint32 cid = *(PRUint32 *) arg;

    ipcLockContext *ctx = (ipcLockContext *) he->value;
    if (ctx->mOwnerID != cid)
        return HT_ENUMERATE_NEXT;

    LOG(("$$$ ipcLockModule_ReleaseByCID [cid=%u key=%s he=%p]\n",
        cid, (char*)he->key, (void*)he));

    if (ipcLockModule_ReleaseLockHelper(cid, (const char *) he->key, ctx))
        return HT_ENUMERATE_REMOVE;

    return HT_ENUMERATE_NEXT;
}



static void
ipcLockModule_Init()
{
    LOG(("$$$ ipcLockModule_Init\n"));

    gLockTable = PL_NewHashTable(32,
                                 PL_HashString,
                                 PL_CompareStrings,
                                 PL_CompareValues,
                                 &ipcLockModule_AllocOps,
                                 NULL);
}

static void
ipcLockModule_Shutdown()
{
    LOG(("$$$ ipcLockModule_Shutdown\n"));
    
    if (gLockTable) {
        
 
        PL_HashTableDestroy(gLockTable);
        gLockTable = NULL;
    }
}

static void
ipcLockModule_HandleMsg(ipcClientHandle client,
                        const nsID     &target,
                        const void     *data,
                        PRUint32        dataLen)
{
    PRUint32 cid = IPC_GetClientID(client);

    LOG(("$$$ ipcLockModule_HandleMsg [cid=%u]\n", cid));

    ipcLockMsg msg;
    IPC_UnflattenLockMsg((const PRUint8 *) data, dataLen, &msg);

    switch (msg.opcode) {
    case IPC_LOCK_OP_ACQUIRE:
        ipcLockModule_AcquireLock(cid, msg.flags, msg.key);
        break;
    case IPC_LOCK_OP_RELEASE:
        ipcLockModule_ReleaseLock(cid, msg.key);
        break;
    default:
        PR_NOT_REACHED("invalid opcode");
    }
}

static void
ipcLockModule_ClientUp(ipcClientHandle client)
{
    LOG(("$$$ ipcLockModule_ClientUp [%u]\n", IPC_GetClientID(client)));
}

static void
ipcLockModule_ClientDown(ipcClientHandle client)
{
    PRUint32 cid = IPC_GetClientID(client);

    LOG(("$$$ ipcLockModule_ClientDown [%u]\n", cid));

    
    
    

    PL_HashTableEnumerateEntries(gLockTable, ipcLockModule_ReleaseByCID, &cid);
}



static ipcModuleMethods gLockMethods =
{
    IPC_MODULE_METHODS_VERSION,
    ipcLockModule_Init,
    ipcLockModule_Shutdown,
    ipcLockModule_HandleMsg,
    ipcLockModule_ClientUp,
    ipcLockModule_ClientDown
};

static ipcModuleEntry gLockModuleEntry[] =
{
    { IPC_LOCK_TARGETID, &gLockMethods }
};

IPC_IMPL_GETMODULES(ipcLockModule, gLockModuleEntry)
