





































#include "nsNPAPIPlugin.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIMemory.h"
#include "nsIPluginStreamListener.h"
#include "nsPluginsDir.h"
#include "nsPluginsDirUtils.h"
#include "prmem.h"
#include "prenv.h"
#include "prerror.h"
#include <sys/stat.h>
#include "nsString.h"
#include "nsILocalFile.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#define LOCAL_PLUGIN_DLL_SUFFIX ".so"
#if defined(__hpux)
#define DEFAULT_X11_PATH "/usr/lib/X11R6/"
#undef LOCAL_PLUGIN_DLL_SUFFIX
#define LOCAL_PLUGIN_DLL_SUFFIX ".sl"
#define LOCAL_PLUGIN_DLL_ALT_SUFFIX ".so"
#elif defined(_AIX)
#define DEFAULT_X11_PATH "/usr/lib"
#define LOCAL_PLUGIN_DLL_ALT_SUFFIX ".a"
#elif defined(SOLARIS)
#define DEFAULT_X11_PATH "/usr/openwin/lib/"
#elif defined(LINUX)
#define DEFAULT_X11_PATH "/usr/X11R6/lib/"
#elif defined(__APPLE__)
#define DEFAULT_X11_PATH "/usr/X11R6/lib"
#undef LOCAL_PLUGIN_DLL_SUFFIX
#define LOCAL_PLUGIN_DLL_SUFFIX ".dylib"
#define LOCAL_PLUGIN_DLL_ALT_SUFFIX ".so"
#else
#define DEFAULT_X11_PATH ""
#endif

#if defined(MOZ_WIDGET_GTK2)

#define PLUGIN_MAX_LEN_OF_TMP_ARR 512

static void DisplayPR_LoadLibraryErrorMessage(const char *libName)
{
    char errorMsg[PLUGIN_MAX_LEN_OF_TMP_ARR] = "Cannot get error from NSPR.";
    if (PR_GetErrorTextLength() < (int) sizeof(errorMsg))
        PR_GetErrorText(errorMsg);

    fprintf(stderr, "LoadPlugin: failed to initialize shared library %s [%s]\n",
        libName, errorMsg);
}

static void SearchForSoname(const char* name, char** soname)
{
    if (!(name && soname))
        return;
    PRDir *fdDir = PR_OpenDir(DEFAULT_X11_PATH);
    if (!fdDir)
        return;       

    int n = PL_strlen(name);
    PRDirEntry *dirEntry;
    while ((dirEntry = PR_ReadDir(fdDir, PR_SKIP_BOTH))) {
        if (!PL_strncmp(dirEntry->name, name, n)) {
            if (dirEntry->name[n] == '.' && dirEntry->name[n+1] && !dirEntry->name[n+2]) {
                
                char out[PLUGIN_MAX_LEN_OF_TMP_ARR] = DEFAULT_X11_PATH;
                PL_strcat(out, dirEntry->name);
                *soname = PL_strdup(out);
               break;
            }
        }
    }

    PR_CloseDir(fdDir);
}

static bool LoadExtraSharedLib(const char *name, char **soname, bool tryToGetSoname)
{
    bool ret = true;
    PRLibSpec tempSpec;
    PRLibrary *handle;
    tempSpec.type = PR_LibSpec_Pathname;
    tempSpec.value.pathname = name;
    handle = PR_LoadLibraryWithFlags(tempSpec, PR_LD_NOW|PR_LD_GLOBAL);
    if (!handle) {
        ret = PR_FALSE;
        DisplayPR_LoadLibraryErrorMessage(name);
        if (tryToGetSoname) {
            SearchForSoname(name, soname);
            if (*soname) {
                ret = LoadExtraSharedLib((const char *) *soname, NULL, PR_FALSE);
            }
        }
    }
    return ret;
}

#define PLUGIN_MAX_NUMBER_OF_EXTRA_LIBS 32
#define PREF_PLUGINS_SONAME "plugin.soname.list"
#if defined(SOLARIS) || defined(HPUX)
#define DEFAULT_EXTRA_LIBS_LIST "libXt" LOCAL_PLUGIN_DLL_SUFFIX ":libXext" LOCAL_PLUGIN_DLL_SUFFIX ":libXm" LOCAL_PLUGIN_DLL_SUFFIX
#else
#define DEFAULT_EXTRA_LIBS_LIST "libXt" LOCAL_PLUGIN_DLL_SUFFIX ":libXext" LOCAL_PLUGIN_DLL_SUFFIX
#endif







static void LoadExtraSharedLibs()
{
    
    nsresult res;
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &res));
    if (NS_SUCCEEDED(res) && (prefs != nsnull)) {
        char *sonameList = NULL;
        bool prefSonameListIsSet = true;
        res = prefs->GetCharPref(PREF_PLUGINS_SONAME, &sonameList);
        if (!sonameList) {
            
            prefSonameListIsSet = PR_FALSE;
            sonameList = PL_strdup(DEFAULT_EXTRA_LIBS_LIST);
        }
        if (sonameList) {
            char *arrayOfLibs[PLUGIN_MAX_NUMBER_OF_EXTRA_LIBS] = {0};
            int numOfLibs = 0;
            char *nextToken;
            char *p = nsCRT::strtok(sonameList,":",&nextToken);
            if (p) {
                while (p && numOfLibs < PLUGIN_MAX_NUMBER_OF_EXTRA_LIBS) {
                    arrayOfLibs[numOfLibs++] = p;
                    p = nsCRT::strtok(nextToken,":",&nextToken);
                }
            } else 
                arrayOfLibs[numOfLibs++] = sonameList;

            char sonameListToSave[PLUGIN_MAX_LEN_OF_TMP_ARR] = "";
            for (int i=0; i<numOfLibs; i++) {
                
                bool head = true;
                p = arrayOfLibs[i];
                while (*p) {
                    if (*p == ' ' || *p == '\t') {
                        if (head) {
                            arrayOfLibs[i] = ++p;
                        } else {
                            *p = 0;
                        }
                    } else {
                        head = PR_FALSE;
                        p++;
                    }
                }
                if (!arrayOfLibs[i][0]) {
                    continue; 
                }
                bool tryToGetSoname = true;
                if (PL_strchr(arrayOfLibs[i], '/')) {
                    
                    struct stat st;
                    if (stat((const char*) arrayOfLibs[i], &st)) {
                        
                        arrayOfLibs[i] = PL_strrchr(arrayOfLibs[i], '/') + 1;
                    } else
                        tryToGetSoname = PR_FALSE;
                }
                char *soname = NULL;
                if (LoadExtraSharedLib(arrayOfLibs[i], &soname, tryToGetSoname)) {
                    
                    p = soname ? soname : arrayOfLibs[i];
                    int n = PLUGIN_MAX_LEN_OF_TMP_ARR -
                        (PL_strlen(sonameListToSave) + PL_strlen(p));
                    if (n > 0) {
                        PL_strcat(sonameListToSave, p);
                        PL_strcat(sonameListToSave,":");
                    }
                    if (soname) {
                        PL_strfree(soname); 
                    }
                    if (numOfLibs > 1)
                        arrayOfLibs[i][PL_strlen(arrayOfLibs[i])] = ':'; 
                }
            }

            
            if (sonameListToSave[0]) 
                for (p = &sonameListToSave[PL_strlen(sonameListToSave) - 1]; *p == ':'; p--)
                    *p = 0; 

            if (!prefSonameListIsSet || PL_strcmp(sonameList, sonameListToSave)) {
                
                
                
                prefs->SetCharPref(PREF_PLUGINS_SONAME, (const char *)sonameListToSave);
            }
            PL_strfree(sonameList);
        }
    }
}
#endif 



bool nsPluginsDir::IsPluginFile(nsIFile* file)
{
    nsCAutoString filename;
    if (NS_FAILED(file->GetNativeLeafName(filename)))
        return PR_FALSE;

#ifdef ANDROID
    
    
    
    
    const char *cFile = filename.get();
    if (strstr(cFile, "libstagefright") != NULL)
        return PR_FALSE;
#endif

    NS_NAMED_LITERAL_CSTRING(dllSuffix, LOCAL_PLUGIN_DLL_SUFFIX);
    if (filename.Length() > dllSuffix.Length() &&
        StringEndsWith(filename, dllSuffix))
        return PR_TRUE;
    
#ifdef LOCAL_PLUGIN_DLL_ALT_SUFFIX
    NS_NAMED_LITERAL_CSTRING(dllAltSuffix, LOCAL_PLUGIN_DLL_ALT_SUFFIX);
    if (filename.Length() > dllAltSuffix.Length() &&
        StringEndsWith(filename, dllAltSuffix))
        return PR_TRUE;
#endif
    return PR_FALSE;
}



nsPluginFile::nsPluginFile(nsIFile* file)
: mPlugin(file)
{
}

nsPluginFile::~nsPluginFile()
{
}

nsresult nsPluginFile::LoadPlugin(PRLibrary **outLibrary)
{
    PRLibSpec libSpec;
    libSpec.type = PR_LibSpec_Pathname;
    bool exists = false;
    mPlugin->Exists(&exists);
    if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;

    nsresult rv;
    nsCAutoString path;
    rv = mPlugin->GetNativePath(path);
    if (NS_FAILED(rv))
        return rv;

    libSpec.value.pathname = path.get();

#if defined(MOZ_WIDGET_GTK2)

    
    
    
    
    
    
    
    
    
    


#if defined(SOLARIS) || defined(HPUX)
    
    *outLibrary = PR_LoadLibraryWithFlags(libSpec, PR_LD_NOW);
    pLibrary = *outLibrary;
#else
    
    *outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
    pLibrary = *outLibrary;
#endif
    if (!pLibrary) {
        LoadExtraSharedLibs();
        
        *outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
        pLibrary = *outLibrary;
        if (!pLibrary) {
            DisplayPR_LoadLibraryErrorMessage(libSpec.value.pathname);
            return NS_ERROR_FAILURE;
        }
    }
#else
    *outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
    pLibrary = *outLibrary;
#endif  

#ifdef NS_DEBUG
    printf("LoadPlugin() %s returned %lx\n", 
           libSpec.value.pathname, (unsigned long)pLibrary);
#endif
    
    return NS_OK;
}

nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info, PRLibrary **outLibrary)
{
    *outLibrary = nsnull;

    info.fVersion = nsnull;

    
    nsresult rv = LoadPlugin(outLibrary);
    if (NS_FAILED(rv))
        return rv;
  
    const char* (*npGetPluginVersion)() =
        (const char* (*)()) PR_FindFunctionSymbol(pLibrary, "NP_GetPluginVersion");
    if (npGetPluginVersion) {
        info.fVersion = PL_strdup(npGetPluginVersion());
    }

    const char* (*npGetMIMEDescription)() =
        (const char* (*)()) PR_FindFunctionSymbol(pLibrary, "NP_GetMIMEDescription");
    if (!npGetMIMEDescription) {
        return NS_ERROR_FAILURE;
    }

    const char* mimedescr = npGetMIMEDescription();
    if (!mimedescr) {
        return NS_ERROR_FAILURE;
    }

    rv = ParsePluginMimeDescription(mimedescr, info);
    if (NS_FAILED(rv)) {
        return rv;
    }

    nsCAutoString path;
    if (NS_FAILED(rv = mPlugin->GetNativePath(path)))
        return rv;
    info.fFullPath = PL_strdup(path.get());

    nsCAutoString fileName;
    if (NS_FAILED(rv = mPlugin->GetNativeLeafName(fileName)))
        return rv;
    info.fFileName = PL_strdup(fileName.get());

    NP_GetValueFunc npGetValue = (NP_GetValueFunc)PR_FindFunctionSymbol(pLibrary, "NP_GetValue");
    if (!npGetValue) {
        return NS_ERROR_FAILURE;
    }

    const char *name = NULL;
    NPError nperr = npGetValue(NULL, NPPVpluginNameString, &name);
    if (name) {
        info.fName = PL_strdup(name);
    }
    else {
        info.fName = PL_strdup(fileName.get());
    }

    const char *description = NULL;
    nperr = npGetValue(NULL, NPPVpluginDescriptionString, &description);
    if (description) {
        info.fDescription = PL_strdup(description);
    }
    else {
        info.fDescription = PL_strdup("");
    }

    return NS_OK;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
    if (info.fName != nsnull)
        PL_strfree(info.fName);

    if (info.fDescription != nsnull)
        PL_strfree(info.fDescription);

    for (PRUint32 i = 0; i < info.fVariantCount; i++) {
        if (info.fMimeTypeArray[i] != nsnull)
            PL_strfree(info.fMimeTypeArray[i]);

        if (info.fMimeDescriptionArray[i] != nsnull)
            PL_strfree(info.fMimeDescriptionArray[i]);

        if (info.fExtensionArray[i] != nsnull)
            PL_strfree(info.fExtensionArray[i]);
    }

    PR_FREEIF(info.fMimeTypeArray);
    PR_FREEIF(info.fMimeDescriptionArray);
    PR_FREEIF(info.fExtensionArray);

    if (info.fFullPath != nsnull)
        PL_strfree(info.fFullPath);

    if (info.fFileName != nsnull)
        PL_strfree(info.fFileName);

    if (info.fVersion != nsnull)
        PL_strfree(info.fVersion);

    return NS_OK;
}
