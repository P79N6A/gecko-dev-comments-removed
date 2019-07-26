







#include <ctype.h>
#include "pkcs11.h"
#include "seccomon.h"
#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pki3hack.h"
#include "secerr.h"
   
#include "utilpars.h" 


static  SECMODModule *
secmod_NewModule(void)
{
    SECMODModule *newMod;
    PRArenaPool *arena;


    


    arena = PORT_NewArena(512);
    if (arena == NULL) {
	return NULL;
    }

    newMod = (SECMODModule *)PORT_ArenaAlloc(arena,sizeof (SECMODModule));
    if (newMod == NULL) {
	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }

    


    newMod->arena = arena;
    newMod->internal = PR_FALSE;
    newMod->loaded = PR_FALSE;
    newMod->isFIPS = PR_FALSE;
    newMod->dllName = NULL;
    newMod->commonName = NULL;
    newMod->library = NULL;
    newMod->functionList = NULL;
    newMod->slotCount = 0;
    newMod->slots = NULL;
    newMod->slotInfo = NULL;
    newMod->slotInfoCount = 0;
    newMod->refCount = 1;
    newMod->ssl[0] = 0;
    newMod->ssl[1] = 0;
    newMod->libraryParams = NULL;
    newMod->moduleDBFunc = NULL;
    newMod->parent = NULL;
    newMod->isCritical = PR_FALSE;
    newMod->isModuleDB = PR_FALSE;
    newMod->moduleDBOnly = PR_FALSE;
    newMod->trustOrder = 0;
    newMod->cipherOrder = 0;
    newMod->evControlMask = 0;
    newMod->refLock = PZ_NewLock(nssILockRefLock);
    if (newMod->refLock == NULL) {
	PORT_FreeArena(arena,PR_FALSE);
	return NULL;
    }
    return newMod;
    
}































#define SECMOD_FLAG_MODULE_DB_IS_MODULE_DB  0x01 /* must be set if any of the 
						  *other flags are set */
#define SECMOD_FLAG_MODULE_DB_SKIP_FIRST    0x02
#define SECMOD_FLAG_MODULE_DB_DEFAULT_MODDB 0x04














#define SECMOD_FLAG_INTERNAL_IS_INTERNAL       0x01 /* must be set if any of 
						     *the other flags are set */
#define SECMOD_FLAG_INTERNAL_KEY_SLOT          0x02




SECMODModule *
SECMOD_CreateModule(const char *library, const char *moduleName, 
				const char *parameters, const char *nss)
{
    SECMODModule *mod = secmod_NewModule();
    char *slotParams,*ciphers;
    
    char *nssc = (char *)nss;
    if (mod == NULL) return NULL;

    mod->commonName = PORT_ArenaStrdup(mod->arena,moduleName ? moduleName : "");
    if (library) {
	mod->dllName = PORT_ArenaStrdup(mod->arena,library);
    }
    
    if (parameters) {
	mod->libraryParams = PORT_ArenaStrdup(mod->arena,parameters);
    }
    mod->internal   = NSSUTIL_ArgHasFlag("flags","internal",nssc);
    mod->isFIPS     = NSSUTIL_ArgHasFlag("flags","FIPS",nssc);
    mod->isCritical = NSSUTIL_ArgHasFlag("flags","critical",nssc);
    slotParams      = NSSUTIL_ArgGetParamValue("slotParams",nssc);
    mod->slotInfo   = NSSUTIL_ArgParseSlotInfo(mod->arena,slotParams,
							&mod->slotInfoCount);
    if (slotParams) PORT_Free(slotParams);
    
    mod->trustOrder  = NSSUTIL_ArgReadLong("trustOrder",nssc,
					NSSUTIL_DEFAULT_TRUST_ORDER,NULL);
    
    mod->cipherOrder = NSSUTIL_ArgReadLong("cipherOrder",nssc,
					NSSUTIL_DEFAULT_CIPHER_ORDER,NULL);
    
    mod->isModuleDB   = NSSUTIL_ArgHasFlag("flags","moduleDB",nssc);
    mod->moduleDBOnly = NSSUTIL_ArgHasFlag("flags","moduleDBOnly",nssc);
    if (mod->moduleDBOnly) mod->isModuleDB = PR_TRUE;

    





    if (mod->isModuleDB) {
	char flags = SECMOD_FLAG_MODULE_DB_IS_MODULE_DB;
	if (NSSUTIL_ArgHasFlag("flags","skipFirst",nssc)) {
	    flags |= SECMOD_FLAG_MODULE_DB_SKIP_FIRST;
	}
	if (NSSUTIL_ArgHasFlag("flags","defaultModDB",nssc)) {
	    flags |= SECMOD_FLAG_MODULE_DB_DEFAULT_MODDB;
	}
	
	mod->isModuleDB = (PRBool) flags;
    }

    if (mod->internal) {
	char flags = SECMOD_FLAG_INTERNAL_IS_INTERNAL;

	if (NSSUTIL_ArgHasFlag("flags", "internalKeySlot", nssc)) {
	    flags |= SECMOD_FLAG_INTERNAL_KEY_SLOT;
	}
	mod->internal = (PRBool) flags;
    }

    ciphers = NSSUTIL_ArgGetParamValue("ciphers",nssc);
    NSSUTIL_ArgParseCipherFlags(&mod->ssl[0],ciphers);
    if (ciphers) PORT_Free(ciphers);

    secmod_PrivateModuleCount++;

    return mod;
}

PRBool
SECMOD_GetSkipFirstFlag(SECMODModule *mod)
{
   char flags = (char) mod->isModuleDB;

   return (flags & SECMOD_FLAG_MODULE_DB_SKIP_FIRST) ? PR_TRUE : PR_FALSE;
}

PRBool
SECMOD_GetDefaultModDBFlag(SECMODModule *mod)
{
   char flags = (char) mod->isModuleDB;

   return (flags & SECMOD_FLAG_MODULE_DB_DEFAULT_MODDB) ? PR_TRUE : PR_FALSE;
}

PRBool
secmod_IsInternalKeySlot(SECMODModule *mod)
{
   char flags = (char) mod->internal;

   return (flags & SECMOD_FLAG_INTERNAL_KEY_SLOT) ? PR_TRUE : PR_FALSE;
}

void
secmod_SetInternalKeySlotFlag(SECMODModule *mod, PRBool val)
{
   char flags = (char) mod->internal;

   if (val)  {
	flags |= SECMOD_FLAG_INTERNAL_KEY_SLOT;
   } else {
	flags &= ~SECMOD_FLAG_INTERNAL_KEY_SLOT;
   }
   mod->internal = flags;
}










static char *
secmod_doDescCopy(char *target, int *targetLen, const char *desc,
			int descLen, char *value)
{
    int diff, esc_len;

    esc_len = NSSUTIL_EscapeSize(value, '\"') - 1;
    diff = esc_len - strlen(value);
    if (diff > 0) {
	

	char *newPtr = PORT_Realloc(target, *targetLen * diff);
	if (!newPtr) {
	    return target; 
	}
	*targetLen += diff;
	target = newPtr;
	value = NSSUTIL_Escape(value, '\"');
	if (value == NULL) {
	    return target; 
	}
    }
    PORT_Memcpy(target, desc, descLen);
    target += descLen;
    *target++='\"';
    PORT_Memcpy(target, value, esc_len);
    target += esc_len;
    *target++='\"';
    if (diff > 0) {
	PORT_Free(value);
    }
    return target;
}

#define SECMOD_SPEC_COPY(new, start, end)    \
  if (end > start) {                         \
	int _cnt = end - start;	             \
	PORT_Memcpy(new, start, _cnt);       \
	new += _cnt;                         \
  }
#define SECMOD_TOKEN_DESCRIPTION "tokenDescription="
#define SECMOD_SLOT_DESCRIPTION   "slotDescription="











char *
secmod_ParseModuleSpecForTokens(PRBool convert, PRBool isFIPS, 
				char *moduleSpec, char ***children, 
				CK_SLOT_ID **ids)
{
    int        newSpecLen   = PORT_Strlen(moduleSpec)+2;
    char       *newSpec     = PORT_Alloc(newSpecLen);
    char       *newSpecPtr  = newSpec;
    char       *modulePrev  = moduleSpec;
    char       *target      = NULL;
    char *tmp = NULL;
    char       **childArray = NULL;
    char       *tokenIndex;
    CK_SLOT_ID *idArray     = NULL;
    int        tokenCount = 0;
    int        i;

    if (newSpec == NULL) {
	return NULL;
    }

    *children = NULL;
    if (ids) {
	*ids = NULL;
    }
    moduleSpec = NSSUTIL_ArgStrip(moduleSpec);
    SECMOD_SPEC_COPY(newSpecPtr, modulePrev, moduleSpec);

    





















    



    while (*moduleSpec) {
	int next;
	modulePrev = moduleSpec;
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, target, "tokens=",
			modulePrev = moduleSpec;  )
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, tmp, "cryptoTokenDescription=",
			if (convert) { modulePrev = moduleSpec; } );
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, tmp, "cryptoSlotDescription=",
			if (convert) { modulePrev = moduleSpec; } );
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, tmp, "dbTokenDescription=",
			if (convert) {
			    modulePrev = moduleSpec; 
			    if (!isFIPS) {
				newSpecPtr = secmod_doDescCopy(newSpecPtr, 
				    &newSpecLen, SECMOD_TOKEN_DESCRIPTION, 
				    sizeof(SECMOD_TOKEN_DESCRIPTION)-1, tmp);
			    }
			});
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, tmp, "dbSlotDescription=",
			if (convert) {
			    modulePrev = moduleSpec;  
			    if (!isFIPS) {
				newSpecPtr = secmod_doDescCopy(newSpecPtr, 
				    &newSpecLen, SECMOD_SLOT_DESCRIPTION, 
				    sizeof(SECMOD_SLOT_DESCRIPTION)-1, tmp);
			    }
			} );
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, tmp, "FIPSTokenDescription=",
			if (convert) {
			    modulePrev = moduleSpec;  
			    if (isFIPS) {
				newSpecPtr = secmod_doDescCopy(newSpecPtr, 
				    &newSpecLen, SECMOD_TOKEN_DESCRIPTION, 
				    sizeof(SECMOD_TOKEN_DESCRIPTION)-1, tmp);
			    }
			} );
	NSSUTIL_HANDLE_STRING_ARG(moduleSpec, tmp, "FIPSSlotDescription=",
			if (convert) {
			    modulePrev = moduleSpec;  
			    if (isFIPS) {
				newSpecPtr = secmod_doDescCopy(newSpecPtr, 
				    &newSpecLen, SECMOD_SLOT_DESCRIPTION, 
				    sizeof(SECMOD_SLOT_DESCRIPTION)-1, tmp);
			    }
			} );
	NSSUTIL_HANDLE_FINAL_ARG(moduleSpec)
	SECMOD_SPEC_COPY(newSpecPtr, modulePrev, moduleSpec);
    }
    if (tmp) {
	PORT_Free(tmp);
	tmp = NULL;
    }
    *newSpecPtr = 0;

    
    if (target == NULL) {
	return newSpec;
    }

    
    
    for (tokenIndex = NSSUTIL_ArgStrip(target); *tokenIndex;
	tokenIndex = NSSUTIL_ArgStrip(NSSUTIL_ArgSkipParameter(tokenIndex))) {
	tokenCount++;
    }

    childArray = PORT_NewArray(char *, tokenCount+1);
    if (childArray == NULL) {
	
	PORT_Free(target);
	return newSpec;
    }
    if (ids) {
	idArray = PORT_NewArray(CK_SLOT_ID, tokenCount+1);
	if (idArray == NULL) {
	    PORT_Free(childArray);
	    PORT_Free(target);
	    return newSpec;
	}
    }

    
    for (tokenIndex = NSSUTIL_ArgStrip(target), i=0 ; 
			*tokenIndex && (i < tokenCount); 
			tokenIndex=NSSUTIL_ArgStrip(tokenIndex)) {
	int next;
	char *name = NSSUTIL_ArgGetLabel(tokenIndex, &next);
	tokenIndex += next;

 	if (idArray) {
	   idArray[i] = NSSUTIL_ArgDecodeNumber(name);
	}

	PORT_Free(name); 

	
	if (!NSSUTIL_ArgIsBlank(*tokenIndex)) {
	    childArray[i++] = NSSUTIL_ArgFetchValue(tokenIndex, &next);
	    tokenIndex += next;
	}
    }

    PORT_Free(target);
    childArray[i] = 0;
    if (idArray) {
	idArray[i] = 0;
    }

    
    *children = childArray;
    if (ids) {
	*ids = idArray;
    }
    return newSpec;
}


static char *
secmod_getConfigDir(char *spec, char **certPrefix, char **keyPrefix,
			  PRBool *readOnly)
{
    char * config = NULL;

    *certPrefix = NULL;
    *keyPrefix = NULL;
    *readOnly = NSSUTIL_ArgHasFlag("flags","readOnly",spec);

    spec = NSSUTIL_ArgStrip(spec);
    while (*spec) {
	int next;
	NSSUTIL_HANDLE_STRING_ARG(spec, config, "configdir=", ;)
	NSSUTIL_HANDLE_STRING_ARG(spec, *certPrefix, "certPrefix=", ;)
	NSSUTIL_HANDLE_STRING_ARG(spec, *keyPrefix, "keyPrefix=", ;)
	NSSUTIL_HANDLE_FINAL_ARG(spec)
    }
    return config;
}

struct SECMODConfigListStr {
    char *config;
    char *certPrefix;
    char *keyPrefix;
    PRBool isReadOnly;
};




SECMODConfigList *
secmod_GetConfigList(PRBool isFIPS, char *spec, int *count)
{
    char **children;
    CK_SLOT_ID *ids;
    char *strippedSpec;
    int childCount;
    SECMODConfigList *conflist = NULL;
    int i;

    strippedSpec = secmod_ParseModuleSpecForTokens(PR_TRUE, isFIPS, 
						spec,&children,&ids);
    if (strippedSpec == NULL) {
	return NULL;
    }

    for (childCount=0; children && children[childCount]; childCount++) ;
    *count = childCount+1; 
    conflist = PORT_NewArray(SECMODConfigList,*count);
    if (conflist == NULL) {
	*count = 0;
	goto loser;
    }

    conflist[0].config = secmod_getConfigDir(strippedSpec, 
					    &conflist[0].certPrefix, 
					    &conflist[0].keyPrefix,
					    &conflist[0].isReadOnly);
    for (i=0; i < childCount; i++) {
	conflist[i+1].config = secmod_getConfigDir(children[i], 
					    &conflist[i+1].certPrefix, 
					    &conflist[i+1].keyPrefix,
					    &conflist[i+1].isReadOnly);
    }

loser:
    secmod_FreeChildren(children, ids);
    PORT_Free(strippedSpec);
    return conflist;
}





static PRBool
secmod_configIsDBM(char *configDir)
{
    char *env;

    
    if (strncmp(configDir, "dbm:", 4) == 0) {
	return PR_TRUE;
    }
    
    if ((strncmp(configDir, "sql:",4) == 0) 
	|| (strncmp(configDir, "rdb:", 4) == 0)
	|| (strncmp(configDir, "extern:", 7) == 0)) {
	return PR_FALSE;
    }
    env = PR_GetEnv("NSS_DEFAULT_DB_TYPE");
    
    if ((env == NULL) || (strcmp(env,"dbm") == 0)) {
	return PR_TRUE;
    }
    
    return PR_FALSE;
}




static PRBool
secmod_matchPrefix(char *prefix1, char *prefix2)
{
    if ((prefix1 == NULL) || (*prefix1 == 0)) {
	if ((prefix2 == NULL) || (*prefix2 == 0)) {
	    return PR_TRUE;
	}
	return PR_FALSE;
    }
    if (strcmp(prefix1, prefix2) == 0) {
	return PR_TRUE;
    }
    return PR_FALSE;
}




PRBool
secmod_MatchConfigList(char *spec, SECMODConfigList *conflist, int count)
{
    char *config;
    char *certPrefix;
    char *keyPrefix;
    PRBool isReadOnly;
    PRBool ret=PR_FALSE;
    int i;

    config = secmod_getConfigDir(spec, &certPrefix, &keyPrefix, &isReadOnly);
    if (!config) {
	ret=PR_TRUE;
	goto done;
    }

    




    if (secmod_configIsDBM(config)) {
	isReadOnly = 1;
    }
    for (i=0; i < count; i++) {
	if ((strcmp(config,conflist[i].config) == 0)  &&
	    secmod_matchPrefix(certPrefix, conflist[i].certPrefix) &&
	    secmod_matchPrefix(keyPrefix, conflist[i].keyPrefix) &&
	    


	    (isReadOnly || !conflist[i].isReadOnly)) {
	    ret = PR_TRUE;
	    goto done;
	}
    }

    ret = PR_FALSE;
done:
    PORT_Free(config);
    PORT_Free(certPrefix);
    PORT_Free(keyPrefix);
    return ret;
}

void
secmod_FreeConfigList(SECMODConfigList *conflist, int count)
{
    int i;
    for (i=0; i < count; i++) {
	PORT_Free(conflist[i].config);
	PORT_Free(conflist[i].certPrefix);
	PORT_Free(conflist[i].keyPrefix);
    }
    PORT_Free(conflist);
}

void
secmod_FreeChildren(char **children, CK_SLOT_ID *ids)
{
    char **thisChild;

    if (!children) {
	return;
    }

    for (thisChild = children; thisChild && *thisChild; thisChild++ ) {
	PORT_Free(*thisChild);
    }
    PORT_Free(children);
    if (ids) {
	PORT_Free(ids);
    }
    return;
}





static int
secmod_getChildLength(char *child, CK_SLOT_ID id)
{
    int length = NSSUTIL_DoubleEscapeSize(child, '>', ']');
    if (id == 0) {
	length++;
    }
    while (id) {
	length++;
	id = id >> 4;
    }
    length += 6; 
    return length;
}





static SECStatus
secmod_mkTokenChild(char **next, int *length, char *child, CK_SLOT_ID id)
{
    int len;
    char *escSpec;

    len = PR_snprintf(*next, *length, " 0x%x=<",id);
    if (len < 0) {
	return SECFailure;
    }
    *next += len;
    *length -= len;
    escSpec = NSSUTIL_DoubleEscape(child, '>', ']');
    if (escSpec == NULL) {
	return SECFailure;
    }
    if (*child && (*escSpec == 0)) {
	PORT_Free(escSpec);
	return SECFailure;
    }
    len = strlen(escSpec);
    if (len+1 > *length) {
	PORT_Free(escSpec);
	return SECFailure;
    }
    PORT_Memcpy(*next,escSpec, len);
    *next += len;
    *length -= len;
    PORT_Free(escSpec);
    **next = '>';
    (*next)++;
    (*length)--;
    return SECSuccess;
}

#define TOKEN_STRING " tokens=["

char *
secmod_MkAppendTokensList(PRArenaPool *arena, char *oldParam, char *newToken, 
			CK_SLOT_ID newID, char **children, CK_SLOT_ID *ids)
{
    char *rawParam = NULL;	
    char *newParam = NULL;	
    char *nextParam = NULL;	
    char **oldChildren = NULL;
    CK_SLOT_ID *oldIds = NULL;
    void *mark = NULL;         

    int length, i, tmpLen;
    SECStatus rv;

    
    rawParam = secmod_ParseModuleSpecForTokens(PR_FALSE,PR_FALSE, 
					oldParam,&oldChildren,&oldIds);
    if (!rawParam) {
	goto loser;
    }

    
    

    length = strlen(rawParam) + sizeof(TOKEN_STRING) + 1;
    
    for (i=0; oldChildren && oldChildren[i]; i++) {
	length += secmod_getChildLength(oldChildren[i], oldIds[i]);
    }

    
    length += secmod_getChildLength(newToken, newID);

    
    for (i=0; children && children[i]; i++) {
	if (ids[i] == -1) {
	    continue;
	}
	length += secmod_getChildLength(children[i], ids[i]);
    }

    
    mark = PORT_ArenaMark(arena);
    if (!mark) {
	goto loser;
    }
    newParam =  PORT_ArenaAlloc(arena,length);
    if (!newParam) {
	goto loser;
    }

    PORT_Strcpy(newParam, oldParam);
    tmpLen = strlen(oldParam);
    nextParam = newParam + tmpLen;
    length -= tmpLen;
    PORT_Memcpy(nextParam, TOKEN_STRING, sizeof(TOKEN_STRING)-1);
    nextParam += sizeof(TOKEN_STRING)-1;
    length -= sizeof(TOKEN_STRING)-1;

    for (i=0; oldChildren && oldChildren[i]; i++) {
	rv = secmod_mkTokenChild(&nextParam,&length,oldChildren[i],oldIds[i]);
	if (rv != SECSuccess) {
	    goto loser;
	}
    }

    rv = secmod_mkTokenChild(&nextParam, &length, newToken, newID);
    if (rv != SECSuccess) {
	goto loser;
    }

    for (i=0; children && children[i]; i++) {
	if (ids[i] == -1) {
	    continue;
	}
	rv = secmod_mkTokenChild(&nextParam, &length, children[i], ids[i]);
	if (rv != SECSuccess) {
	    goto loser;
	}
    }

    if (length < 2) {
	goto loser;
    }

    *nextParam++ = ']';
    *nextParam++ = 0;

    
    PORT_ArenaUnmark(arena, mark);
    mark = NULL;

loser:
    if (mark) {
	PORT_ArenaRelease(arena, mark);
	newParam = NULL; 

    }
    if (rawParam) {
	PORT_Free(rawParam);
    }
    if (oldChildren) {
	secmod_FreeChildren(oldChildren, oldIds);
    }
    return newParam;
}
    
static char *
secmod_mkModuleSpec(SECMODModule * module)
{
    char *nss = NULL, *modSpec = NULL, **slotStrings = NULL;
    int slotCount, i, si;
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();

    
    slotCount = 0;

    SECMOD_GetReadLock(moduleLock);
    if (module->slotCount) {
	for (i=0; i < module->slotCount; i++) {
	    if (module->slots[i]->defaultFlags !=0) {
		slotCount++;
	    }
	}
    } else {
	slotCount = module->slotInfoCount;
    }

    slotStrings = (char **)PORT_ZAlloc(slotCount*sizeof(char *));
    if (slotStrings == NULL) {
        SECMOD_ReleaseReadLock(moduleLock);
	goto loser;
    }


    
    if (module->slotCount) {
	for (i=0, si= 0; i < module->slotCount; i++) {
	    if (module->slots[i]->defaultFlags) {
		PORT_Assert(si < slotCount);
		if (si >= slotCount) break;
		slotStrings[si] = NSSUTIL_MkSlotString(module->slots[i]->slotID,
			module->slots[i]->defaultFlags,
			module->slots[i]->timeout,
			module->slots[i]->askpw,
			module->slots[i]->hasRootCerts,
			module->slots[i]->hasRootTrust);
		si++;
	    }
	}
     } else {
	for (i=0; i < slotCount; i++) {
		slotStrings[i] = NSSUTIL_MkSlotString(
			module->slotInfo[i].slotID,
			module->slotInfo[i].defaultFlags,
			module->slotInfo[i].timeout,
			module->slotInfo[i].askpw,
			module->slotInfo[i].hasRootCerts,
			module->slotInfo[i].hasRootTrust);
	}
    }

    SECMOD_ReleaseReadLock(moduleLock);
    nss = NSSUTIL_MkNSSString(slotStrings,slotCount,module->internal, 
		       module->isFIPS, module->isModuleDB,
		       module->moduleDBOnly, module->isCritical,
		       module->trustOrder, module->cipherOrder,
		       module->ssl[0],module->ssl[1]);
    modSpec= NSSUTIL_MkModuleSpec(module->dllName,module->commonName,
						module->libraryParams,nss);
    PORT_Free(slotStrings);
    PR_smprintf_free(nss);
loser:
    return (modSpec);
}
    

char **
SECMOD_GetModuleSpecList(SECMODModule *module)
{
    SECMODModuleDBFunc func = (SECMODModuleDBFunc) module->moduleDBFunc;
    if (func) {
	return (*func)(SECMOD_MODULE_DB_FUNCTION_FIND,
		module->libraryParams,NULL);
    }
    return NULL;
}

SECStatus
SECMOD_AddPermDB(SECMODModule *module)
{
    SECMODModuleDBFunc func;
    char *moduleSpec;
    char **retString;

    if (module->parent == NULL) return SECFailure;

    func  = (SECMODModuleDBFunc) module->parent->moduleDBFunc;
    if (func) {
	moduleSpec = secmod_mkModuleSpec(module);
	retString = (*func)(SECMOD_MODULE_DB_FUNCTION_ADD,
		module->parent->libraryParams,moduleSpec);
	PORT_Free(moduleSpec);
	if (retString != NULL) return SECSuccess;
    }
    return SECFailure;
}

SECStatus
SECMOD_DeletePermDB(SECMODModule *module)
{
    SECMODModuleDBFunc func;
    char *moduleSpec;
    char **retString;

    if (module->parent == NULL) return SECFailure;

    func  = (SECMODModuleDBFunc) module->parent->moduleDBFunc;
    if (func) {
	moduleSpec = secmod_mkModuleSpec(module);
	retString = (*func)(SECMOD_MODULE_DB_FUNCTION_DEL,
		module->parent->libraryParams,moduleSpec);
	PORT_Free(moduleSpec);
	if (retString != NULL) return SECSuccess;
    }
    return SECFailure;
}

SECStatus
SECMOD_FreeModuleSpecList(SECMODModule *module, char **moduleSpecList)
{
    SECMODModuleDBFunc func = (SECMODModuleDBFunc) module->moduleDBFunc;
    char **retString;
    if (func) {
	retString = (*func)(SECMOD_MODULE_DB_FUNCTION_RELEASE,
		module->libraryParams,moduleSpecList);
	if (retString != NULL) return SECSuccess;
    }
    return SECFailure;
}




SECMODModule *
SECMOD_LoadModule(char *modulespec,SECMODModule *parent, PRBool recurse)
{
    char *library = NULL, *moduleName = NULL, *parameters = NULL, *nss= NULL;
    SECStatus status;
    SECMODModule *module = NULL;
    SECMODModule *oldModule = NULL;
    SECStatus rv;

    
    SECMOD_Init();

    status = NSSUTIL_ArgParseModuleSpec(modulespec, &library, &moduleName, 
							&parameters, &nss);
    if (status != SECSuccess) {
	goto loser;
    }

    module = SECMOD_CreateModule(library, moduleName, parameters, nss);
    if (library) PORT_Free(library);
    if (moduleName) PORT_Free(moduleName);
    if (parameters) PORT_Free(parameters);
    if (nss) PORT_Free(nss);
    if (!module) {
	goto loser;
    }
    if (parent) {
    	module->parent = SECMOD_ReferenceModule(parent);
	if (module->internal && secmod_IsInternalKeySlot(parent)) {
	    module->internal = parent->internal;
	}
    }

    
    rv = secmod_LoadPKCS11Module(module, &oldModule);
    if (rv != SECSuccess) {
	goto loser;
    }

    

    if (oldModule) {
	

	SECMOD_DestroyModule(module);
	return oldModule;
    }

    if (recurse && module->isModuleDB) {
	char ** moduleSpecList;
	PORT_SetError(0);

	moduleSpecList = SECMOD_GetModuleSpecList(module);
	if (moduleSpecList) {
	    char **index;

	    index = moduleSpecList;
	    if (*index && SECMOD_GetSkipFirstFlag(module)) {
		index++;
	    }

	    for (; *index; index++) {
		SECMODModule *child;
		if (0 == PORT_Strcmp(*index, modulespec)) {
		    
		    PORT_SetError(SEC_ERROR_NO_MODULE);
		    rv = SECFailure;
		    break;
		}
		child = SECMOD_LoadModule(*index,module,PR_TRUE);
		if (!child) break;
		if (child->isCritical && !child->loaded) {
		    int err = PORT_GetError();
		    if (!err)  
			err = SEC_ERROR_NO_MODULE;
		    SECMOD_DestroyModule(child);
		    PORT_SetError(err);
		    rv = SECFailure;
		    break;
		}
		SECMOD_DestroyModule(child);
	    }
	    SECMOD_FreeModuleSpecList(module,moduleSpecList);
	} else {
	    if (!PORT_GetError())
		PORT_SetError(SEC_ERROR_NO_MODULE);
	    rv = SECFailure;
	}
    }

    if (rv != SECSuccess) {
	goto loser;
    }


    
    if (!module->moduleDBOnly) {
	SECMOD_AddModuleToList(module);
    } else {
	SECMOD_AddModuleToDBOnlyList(module);
    }
   
    
    return module;

loser:
    if (module) {
	if (module->loaded) {
	    SECMOD_UnloadModule(module);
	}
	SECMOD_AddModuleToUnloadList(module);
    }
    return module;
}




SECMODModule *
SECMOD_LoadUserModule(char *modulespec,SECMODModule *parent, PRBool recurse)
{
    SECStatus rv = SECSuccess;
    SECMODModule * newmod = SECMOD_LoadModule(modulespec, parent, recurse);
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();

    if (newmod) {
	SECMOD_GetReadLock(moduleLock);
        rv = STAN_AddModuleToDefaultTrustDomain(newmod);
	SECMOD_ReleaseReadLock(moduleLock);
        if (SECSuccess != rv) {
            SECMOD_DestroyModule(newmod);
            return NULL;
        }
    }
    return newmod;
}





SECStatus SECMOD_UnloadUserModule(SECMODModule *mod)
{
    SECStatus rv = SECSuccess;
    int atype = 0;
    SECMODListLock *moduleLock = SECMOD_GetDefaultModuleListLock();
    if (!mod) {
        return SECFailure;
    }

    SECMOD_GetReadLock(moduleLock);
    rv = STAN_RemoveModuleFromDefaultTrustDomain(mod);
    SECMOD_ReleaseReadLock(moduleLock);
    if (SECSuccess != rv) {
        return SECFailure;
    }
    return SECMOD_DeleteModuleEx(NULL, mod, &atype, PR_FALSE);
}

