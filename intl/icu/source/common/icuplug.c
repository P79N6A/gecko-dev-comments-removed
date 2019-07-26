














#include "unicode/icuplug.h"
#include "icuplugimp.h"
#include "cstring.h"
#include "cmemory.h"
#include "putilimp.h"
#include "ucln.h"
#include <stdio.h>
#ifdef __MVS__  
#define _POSIX_SOURCE
#include <cics.h> 
#endif

#ifndef UPLUG_TRACE
#define UPLUG_TRACE 0
#endif

#if UPLUG_TRACE
#include <stdio.h>
#define DBG(x) fprintf(stderr, "%s:%d: ",__FILE__,__LINE__); fprintf x
#endif





struct UPlugData {
  UPlugEntrypoint  *entrypoint; 
  uint32_t structSize;    
  uint32_t token;         
  void *lib;              
  char libName[UPLUG_NAME_MAX];   
  char sym[UPLUG_NAME_MAX];        
  char config[UPLUG_NAME_MAX];     
  void *context;          
  char name[UPLUG_NAME_MAX];   
  UPlugLevel  level; 
  UBool   awaitingLoad; 
  UBool   dontUnload; 
  UErrorCode pluginStatus; 
};



#define UPLUG_LIBRARY_INITIAL_COUNT 8
#define UPLUG_PLUGIN_INITIAL_COUNT 12









static int32_t uplug_removeEntryAt(void *list, int32_t listSize, int32_t memberSize, int32_t itemToRemove) {
  uint8_t *bytePtr = (uint8_t *)list;
    
  
  if(listSize<1) {
    return listSize;
  }
    
  
  if(listSize > itemToRemove+1) {
    memmove(bytePtr+(itemToRemove*memberSize), bytePtr+((itemToRemove+1)*memberSize), memberSize);
  }
    
  return listSize-1;
}




#if U_ENABLE_DYLOAD




struct UPlugLibrary;





typedef struct UPlugLibrary {
  void *lib;                           
  char name[UPLUG_NAME_MAX]; 
  uint32_t ref;                        
} UPlugLibrary;

static UPlugLibrary   staticLibraryList[UPLUG_LIBRARY_INITIAL_COUNT];
static UPlugLibrary * libraryList = staticLibraryList;
static int32_t libraryCount = 0;
static int32_t libraryMax = UPLUG_LIBRARY_INITIAL_COUNT;






static int32_t searchForLibraryName(const char *libName) {
  int32_t i;
    
  for(i=0;i<libraryCount;i++) {
    if(!uprv_strcmp(libName, libraryList[i].name)) {
      return i;
    }
  }
  return -1;
}

static int32_t searchForLibrary(void *lib) {
  int32_t i;
    
  for(i=0;i<libraryCount;i++) {
    if(lib==libraryList[i].lib) {
      return i;
    }
  }
  return -1;
}

U_INTERNAL char * U_EXPORT2
uplug_findLibrary(void *lib, UErrorCode *status) {
  int32_t libEnt;
  char *ret = NULL;
  if(U_FAILURE(*status)) {
    return NULL;
  }
  libEnt = searchForLibrary(lib);
  if(libEnt!=-1) { 
    ret = libraryList[libEnt].name;
  } else {
    *status = U_MISSING_RESOURCE_ERROR;
  }
  return ret;
}

U_INTERNAL void * U_EXPORT2
uplug_openLibrary(const char *libName, UErrorCode *status) {
  int32_t libEntry = -1;
  void *lib = NULL;
    
  if(U_FAILURE(*status)) return NULL;

  libEntry = searchForLibraryName(libName);
  if(libEntry == -1) {
    libEntry = libraryCount++;
    if(libraryCount >= libraryMax) {
      
      *status = U_MEMORY_ALLOCATION_ERROR;
#if UPLUG_TRACE
      DBG((stderr, "uplug_openLibrary() - out of library slots (max %d)\n", libraryMax));
#endif
      return NULL;
    }
    

    libraryList[libEntry].lib = uprv_dl_open(libName, status);
#if UPLUG_TRACE
    DBG((stderr, "uplug_openLibrary(%s,%s) libEntry %d, lib %p\n", libName, u_errorName(*status), libEntry, lib));
#endif
        
    if(libraryList[libEntry].lib == NULL || U_FAILURE(*status)) {
      
      libraryList[libEntry].lib = NULL; 
      libraryList[libEntry].name[0] = 0;
#if UPLUG_TRACE
      DBG((stderr, "uplug_openLibrary(%s,%s) libEntry %d, lib %p\n", libName, u_errorName(*status), libEntry, lib));
#endif
      
      libraryCount--;
    } else { 
      
      uprv_strncpy(libraryList[libEntry].name,libName,UPLUG_NAME_MAX);
      libraryList[libEntry].ref=1;
      lib = libraryList[libEntry].lib;
    }

  } else {
    lib = libraryList[libEntry].lib;
    libraryList[libEntry].ref++;
  }
  return lib;
}

U_INTERNAL void U_EXPORT2
uplug_closeLibrary(void *lib, UErrorCode *status) {
  int32_t i;
    
#if UPLUG_TRACE
  DBG((stderr, "uplug_closeLibrary(%p,%s) list %p\n", lib, u_errorName(*status), (void*)libraryList));
#endif
  if(U_FAILURE(*status)) return;
    
  for(i=0;i<libraryCount;i++) {
    if(lib==libraryList[i].lib) {
      if(--(libraryList[i].ref) == 0) {
        uprv_dl_close(libraryList[i].lib, status);
        libraryCount = uplug_removeEntryAt(libraryList, libraryCount, sizeof(*libraryList), i);
      }
      return;
    }
  }
  *status = U_INTERNAL_PROGRAM_ERROR; 
}

#endif

static UPlugData pluginList[UPLUG_PLUGIN_INITIAL_COUNT];
static int32_t pluginCount = 0;



  
static int32_t uplug_pluginNumber(UPlugData* d) {
  UPlugData *pastPlug = &pluginList[pluginCount];
  if(d<=pluginList) {
    return 0;
  } else if(d>=pastPlug) {
    return pluginCount;
  } else {
    return (d-pluginList)/sizeof(pluginList[0]);
  }
}


U_CAPI UPlugData * U_EXPORT2
uplug_nextPlug(UPlugData *prior) {
  if(prior==NULL) {
    return pluginList;
  } else {
    UPlugData *nextPlug = &prior[1];
    UPlugData *pastPlug = &pluginList[pluginCount];
    
    if(nextPlug>=pastPlug) {
      return NULL;
    } else {
      return nextPlug;
    }
  }
}






static void uplug_callPlug(UPlugData *plug, UPlugReason reason, UErrorCode *status) {
  UPlugTokenReturn token;
  if(plug==NULL||U_FAILURE(*status)) {
    return;
  }
  token = (*(plug->entrypoint))(plug, reason, status);
  if(token!=UPLUG_TOKEN) {
    *status = U_INTERNAL_PROGRAM_ERROR;
  }
}


static void uplug_unloadPlug(UPlugData *plug, UErrorCode *status) {
  if(plug->awaitingLoad) {  
    *status = U_INTERNAL_PROGRAM_ERROR;
    return; 
  }
  if(U_SUCCESS(plug->pluginStatus)) {
    
    uplug_callPlug(plug, UPLUG_REASON_UNLOAD, status);
  }
}

static void uplug_queryPlug(UPlugData *plug, UErrorCode *status) {
  if(!plug->awaitingLoad || !(plug->level == UPLUG_LEVEL_UNKNOWN) ) {  
    *status = U_INTERNAL_PROGRAM_ERROR;
    return; 
  }
  plug->level = UPLUG_LEVEL_INVALID;
  uplug_callPlug(plug, UPLUG_REASON_QUERY, status);
  if(U_SUCCESS(*status)) { 
    if(plug->level == UPLUG_LEVEL_INVALID) {
      plug->pluginStatus = U_PLUGIN_DIDNT_SET_LEVEL;
      plug->awaitingLoad = FALSE;
    }
  } else {
    plug->pluginStatus = U_INTERNAL_PROGRAM_ERROR;
    plug->awaitingLoad = FALSE;
  }
}


static void uplug_loadPlug(UPlugData *plug, UErrorCode *status) {
  if(!plug->awaitingLoad || (plug->level < UPLUG_LEVEL_LOW) ) {  
    *status = U_INTERNAL_PROGRAM_ERROR;
    return;
  }
  uplug_callPlug(plug, UPLUG_REASON_LOAD, status);
  plug->awaitingLoad = FALSE;
  if(!U_SUCCESS(*status)) {
    plug->pluginStatus = U_INTERNAL_PROGRAM_ERROR;
  }
}

static UPlugData *uplug_allocateEmptyPlug(UErrorCode *status)
{
  UPlugData *plug = NULL;

  if(U_FAILURE(*status)) {
    return NULL;
  }

  if(pluginCount == UPLUG_PLUGIN_INITIAL_COUNT) {
    *status = U_MEMORY_ALLOCATION_ERROR;
    return NULL;
  }

  plug = &pluginList[pluginCount++];

  plug->token = UPLUG_TOKEN;
  plug->structSize = sizeof(UPlugData);
  plug->name[0]=0;
  plug->level = UPLUG_LEVEL_UNKNOWN; 
  plug->awaitingLoad = TRUE;
  plug->dontUnload = FALSE;
  plug->pluginStatus = U_ZERO_ERROR;
  plug->libName[0] = 0;
  plug->config[0]=0;
  plug->sym[0]=0;
  plug->lib=NULL;
  plug->entrypoint=NULL;


  return plug;
}

static UPlugData *uplug_allocatePlug(UPlugEntrypoint *entrypoint, const char *config, void *lib, const char *symName,
                                     UErrorCode *status) {
  UPlugData *plug;

  if(U_FAILURE(*status)) {
    return NULL;
  }

  plug = uplug_allocateEmptyPlug(status);
  if(config!=NULL) {
    uprv_strncpy(plug->config, config, UPLUG_NAME_MAX);
  } else {
    plug->config[0] = 0;
  }
    
  if(symName!=NULL) {
    uprv_strncpy(plug->sym, symName, UPLUG_NAME_MAX);
  } else {
    plug->sym[0] = 0;
  }
    
  plug->entrypoint = entrypoint;
  plug->lib = lib;
  uplug_queryPlug(plug, status);
    
  return plug;
}

static void uplug_deallocatePlug(UPlugData *plug, UErrorCode *status) {
  UErrorCode subStatus = U_ZERO_ERROR;
  if(!plug->dontUnload) {
#if U_ENABLE_DYLOAD
    uplug_closeLibrary(plug->lib, &subStatus);
#endif
  }
  plug->lib = NULL;
  if(U_SUCCESS(*status) && U_FAILURE(subStatus)) {
    *status = subStatus;
  }
  
  if(U_SUCCESS(*status)) {
    
    pluginCount = uplug_removeEntryAt(pluginList, pluginCount, sizeof(plug[0]), uplug_pluginNumber(plug));
  } else {
    
    plug->awaitingLoad=FALSE;
    plug->entrypoint=0;
    plug->dontUnload=TRUE;
  }
}

static void uplug_doUnloadPlug(UPlugData *plugToRemove, UErrorCode *status) {
  if(plugToRemove != NULL) {
    uplug_unloadPlug(plugToRemove, status);
    uplug_deallocatePlug(plugToRemove, status);
  }
}

U_CAPI void U_EXPORT2
uplug_removePlug(UPlugData *plug, UErrorCode *status)  {
  UPlugData *cursor = NULL;
  UPlugData *plugToRemove = NULL;
  if(U_FAILURE(*status)) return;
    
  for(cursor=pluginList;cursor!=NULL;) {
    if(cursor==plug) {
      plugToRemove = plug;
      cursor=NULL;
    } else {
      cursor = uplug_nextPlug(cursor);
    }
  }
    
  uplug_doUnloadPlug(plugToRemove, status);
}




U_CAPI void U_EXPORT2 
uplug_setPlugNoUnload(UPlugData *data, UBool dontUnload)
{
  data->dontUnload = dontUnload;
}


U_CAPI void U_EXPORT2
uplug_setPlugLevel(UPlugData *data, UPlugLevel level) {
  data->level = level;
}


U_CAPI UPlugLevel U_EXPORT2
uplug_getPlugLevel(UPlugData *data) {
  return data->level;
}


U_CAPI void U_EXPORT2
uplug_setPlugName(UPlugData *data, const char *name) {
  uprv_strncpy(data->name, name, UPLUG_NAME_MAX);
}


U_CAPI const char * U_EXPORT2
uplug_getPlugName(UPlugData *data) {
  return data->name;
}


U_CAPI const char * U_EXPORT2
uplug_getSymbolName(UPlugData *data) {
  return data->sym;
}

U_CAPI const char * U_EXPORT2
uplug_getLibraryName(UPlugData *data, UErrorCode *status) {
  if(data->libName[0]) {
    return data->libName;
  } else {
#if U_ENABLE_DYLOAD
    return uplug_findLibrary(data->lib, status);
#else
    return NULL;
#endif
  }
}

U_CAPI void * U_EXPORT2
uplug_getLibrary(UPlugData *data) {
  return data->lib;
}

U_CAPI void * U_EXPORT2
uplug_getContext(UPlugData *data) {
  return data->context;
}


U_CAPI void U_EXPORT2
uplug_setContext(UPlugData *data, void *context) {
  data->context = context;
}

U_CAPI const char* U_EXPORT2
uplug_getConfiguration(UPlugData *data) {
  return data->config;
}

U_INTERNAL UPlugData* U_EXPORT2
uplug_getPlugInternal(int32_t n) { 
  if(n <0 || n >= pluginCount) {
    return NULL;
  } else { 
    return &(pluginList[n]);
  }
}


U_CAPI UErrorCode U_EXPORT2
uplug_getPlugLoadStatus(UPlugData *plug) {
  return plug->pluginStatus;
}







static UPlugData* uplug_initPlugFromEntrypointAndLibrary(UPlugEntrypoint *entrypoint, const char *config, void *lib, const char *sym,
                                                         UErrorCode *status) {
  UPlugData *plug = NULL;

  plug = uplug_allocatePlug(entrypoint, config, lib, sym, status);

  if(U_SUCCESS(*status)) {
    return plug;
  } else {
    uplug_deallocatePlug(plug, status);
    return NULL;
  }
}

U_CAPI UPlugData* U_EXPORT2
uplug_loadPlugFromEntrypoint(UPlugEntrypoint *entrypoint, const char *config, UErrorCode *status) {
  UPlugData* plug = uplug_initPlugFromEntrypointAndLibrary(entrypoint, config, NULL, NULL, status);
  uplug_loadPlug(plug, status);
  return plug;
}

#if U_ENABLE_DYLOAD

static UPlugData* 
uplug_initErrorPlug(const char *libName, const char *sym, const char *config, const char *nameOrError, UErrorCode loadStatus, UErrorCode *status)
{
  UPlugData *plug = uplug_allocateEmptyPlug(status);
  if(U_FAILURE(*status)) return NULL;

  plug->pluginStatus = loadStatus;
  plug->awaitingLoad = FALSE; 
  plug->dontUnload = TRUE; 

  if(sym!=NULL) {
    uprv_strncpy(plug->sym, sym, UPLUG_NAME_MAX);
  }

  if(libName!=NULL) {
    uprv_strncpy(plug->libName, libName, UPLUG_NAME_MAX);
  }

  if(nameOrError!=NULL) {
    uprv_strncpy(plug->name, nameOrError, UPLUG_NAME_MAX);
  }

  if(config!=NULL) {
    uprv_strncpy(plug->config, config, UPLUG_NAME_MAX);
  }

  return plug;
}




static UPlugData* 
uplug_initPlugFromLibrary(const char *libName, const char *sym, const char *config, UErrorCode *status) {
  void *lib = NULL;
  UPlugData *plug = NULL;
  if(U_FAILURE(*status)) { return NULL; }
  lib = uplug_openLibrary(libName, status);
  if(lib!=NULL && U_SUCCESS(*status)) {
    UPlugEntrypoint *entrypoint = NULL;
    entrypoint = (UPlugEntrypoint*)uprv_dlsym_func(lib, sym, status);

    if(entrypoint!=NULL&&U_SUCCESS(*status)) {
      plug = uplug_initPlugFromEntrypointAndLibrary(entrypoint, config, lib, sym, status);
      if(plug!=NULL&&U_SUCCESS(*status)) {
        plug->lib = lib; 
        lib = NULL; 
      }
    } else {
      UErrorCode subStatus = U_ZERO_ERROR;
      plug = uplug_initErrorPlug(libName,sym,config,"ERROR: Could not load entrypoint",(lib==NULL)?U_MISSING_RESOURCE_ERROR:*status,&subStatus);
    }
    if(lib!=NULL) { 
      UErrorCode subStatus = U_ZERO_ERROR;
      uplug_closeLibrary(lib, &subStatus); 
    }
  } else {
    UErrorCode subStatus = U_ZERO_ERROR;
    plug = uplug_initErrorPlug(libName,sym,config,"ERROR: could not load library",(lib==NULL)?U_MISSING_RESOURCE_ERROR:*status,&subStatus);
  }
  return plug;
}

U_CAPI UPlugData* U_EXPORT2
uplug_loadPlugFromLibrary(const char *libName, const char *sym, const char *config, UErrorCode *status) { 
  UPlugData *plug = NULL;
  if(U_FAILURE(*status)) { return NULL; }
  plug = uplug_initPlugFromLibrary(libName, sym, config, status);
  uplug_loadPlug(plug, status);

  return plug;
}

#endif

U_CAPI UPlugLevel U_EXPORT2 uplug_getCurrentLevel() {
  if(cmemory_inUse()) {
    return UPLUG_LEVEL_HIGH;
  } else {
    return UPLUG_LEVEL_LOW;
  }
}

static UBool U_CALLCONV uplug_cleanup(void)
{
  int32_t i;
    
  UPlugData *pluginToRemove;
  
  for(i=0;i<pluginCount;i++) {
    UErrorCode subStatus = U_ZERO_ERROR;
    pluginToRemove = &pluginList[i];
    
    uplug_doUnloadPlug(pluginToRemove, &subStatus);
  }
  
  return TRUE;
}

#if U_ENABLE_DYLOAD

static void uplug_loadWaitingPlugs(UErrorCode *status) {
  int32_t i;
  UPlugLevel currentLevel = uplug_getCurrentLevel();
    
  if(U_FAILURE(*status)) {
    return;
  }
#if UPLUG_TRACE
  DBG((stderr,  "uplug_loadWaitingPlugs() Level: %d\n", currentLevel));
#endif
  
  for(i=0;i<pluginCount;i++) {
    UErrorCode subStatus = U_ZERO_ERROR;
    UPlugData *pluginToLoad = &pluginList[i];
    if(pluginToLoad->awaitingLoad) {
      if(pluginToLoad->level == UPLUG_LEVEL_LOW) {
        if(currentLevel > UPLUG_LEVEL_LOW) {
          pluginToLoad->pluginStatus = U_PLUGIN_TOO_HIGH;
        } else {
          UPlugLevel newLevel;
          uplug_loadPlug(pluginToLoad, &subStatus);
          newLevel = uplug_getCurrentLevel();
          if(newLevel > currentLevel) {
            pluginToLoad->pluginStatus = U_PLUGIN_CHANGED_LEVEL_WARNING;
            currentLevel = newLevel;
          }
        }
        pluginToLoad->awaitingLoad = FALSE;
      } 
    }
  }    
  for(i=0;i<pluginCount;i++) {
    UErrorCode subStatus = U_ZERO_ERROR;
    UPlugData *pluginToLoad = &pluginList[i];
        
    if(pluginToLoad->awaitingLoad) {
      if(pluginToLoad->level == UPLUG_LEVEL_INVALID) { 
        pluginToLoad->pluginStatus = U_PLUGIN_DIDNT_SET_LEVEL;
      } else if(pluginToLoad->level == UPLUG_LEVEL_UNKNOWN) {
        pluginToLoad->pluginStatus = U_INTERNAL_PROGRAM_ERROR;
      } else {
        uplug_loadPlug(pluginToLoad, &subStatus);
      }
      pluginToLoad->awaitingLoad = FALSE;
    }
  }
    
#if UPLUG_TRACE
  DBG((stderr,  " Done Loading Plugs. Level: %d\n", (int32_t)uplug_getCurrentLevel()));
#endif
}


static char plugin_file[2048] = "";
#endif

U_INTERNAL const char* U_EXPORT2
uplug_getPluginFile() {
#if U_ENABLE_DYLOAD
  return plugin_file;
#else
  return NULL;
#endif
}


U_CAPI void U_EXPORT2
uplug_init(UErrorCode *status) {
#if !U_ENABLE_DYLOAD
  (void)status; 
#else
  const char *plugin_dir;

  if(U_FAILURE(*status)) return;
  plugin_dir = getenv("ICU_PLUGINS");

#if defined(DEFAULT_ICU_PLUGINS) 
  if(plugin_dir == NULL || !*plugin_dir) {
    plugin_dir = DEFAULT_ICU_PLUGINS;
  }
#endif

#if UPLUG_TRACE
  DBG((stderr, "ICU_PLUGINS=%s\n", plugin_dir));
#endif

  if(plugin_dir != NULL && *plugin_dir) {
    FILE *f;
        
        
#ifdef OS390BATCH







    uprv_strncpy(plugin_file,"//DD:ICUPLUG", 2047);        
#else
    uprv_strncpy(plugin_file, plugin_dir, 2047);
    uprv_strncat(plugin_file, U_FILE_SEP_STRING,2047);
    uprv_strncat(plugin_file, "icuplugins",2047);
    uprv_strncat(plugin_file, U_ICU_VERSION_SHORT ,2047);
    uprv_strncat(plugin_file, ".txt" ,2047);
#endif
        
#if UPLUG_TRACE
    DBG((stderr, "pluginfile= %s\n", plugin_file));
#endif
        
#ifdef __MVS__
    if (iscics()) 
    {
        f = NULL;
    }
    else
#endif
    {
         f = fopen(plugin_file, "r");
    }

    if(f != NULL) {
      char linebuf[1024];
      char *p, *libName=NULL, *symName=NULL, *config=NULL;
      int32_t line = 0;
            
            
      while(fgets(linebuf,1023,f)) {
        line++;

        if(!*linebuf || *linebuf=='#') {
          continue;
        } else {
          p = linebuf;
          while(*p&&isspace((int)*p))
            p++;
          if(!*p || *p=='#') continue;
          libName = p;
          while(*p&&!isspace((int)*p)) {
            p++;
          }
          if(!*p || *p=='#') continue; 
          *p=0; 
          p++;
          while(*p&&isspace((int)*p)) {
            p++;
          }
          if(!*p||*p=='#') continue; 
          symName = p;
          while(*p&&!isspace((int)*p)) {
            p++;
          }
                    
          if(*p) { 
            *p=0;
            ++p;
            while(*p&&isspace((int)*p)) {
              p++;
            }
            if(*p) {
              config = p;
            }
          }
                    
          
          if(config!=NULL&&*config!=0) {
            p = config+strlen(config);
            while(p>config&&isspace((int)*(--p))) {
              *p=0;
            }
          }
                
          
          { 
            UErrorCode subStatus = U_ZERO_ERROR;
            UPlugData *plug = uplug_initPlugFromLibrary(libName, symName, config, &subStatus);
            if(U_FAILURE(subStatus) && U_SUCCESS(*status)) {
              *status = subStatus;
            }
#if UPLUG_TRACE
            DBG((stderr, "PLUGIN libName=[%s], sym=[%s], config=[%s]\n", libName, symName, config));
            DBG((stderr, " -> %p, %s\n", (void*)plug, u_errorName(subStatus)));
#else
            (void)plug; 
#endif
          }
        }
      }
      fclose(f);
    } else {
#if UPLUG_TRACE
      DBG((stderr, "Can't open plugin file %s\n", plugin_file));
#endif
    }
  }
  uplug_loadWaitingPlugs(status);
#endif 
  ucln_registerCleanup(UCLN_UPLUG, uplug_cleanup);
}
