












































#include "nsplugin.h"
#include "ns4xPlugin.h"
#include "ns4xPluginInstance.h"
#include "nsIServiceManager.h"
#include "nsIMemory.h"
#include "nsIPluginStreamListener.h"
#include "nsPluginsDir.h"
#include "nsPluginsDirUtils.h"
#include "nsObsoleteModuleLoading.h"
#include "prmem.h"
#include "prenv.h"
#include "prerror.h"
#include <sys/stat.h>
#include "nsString.h"
#include "nsILocalFile.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#ifdef AIX
#include "nsPluginLogging.h"
#include "prprf.h"
#define LOG(args)  PLUGIN_LOG(PLUGIN_LOG_NORMAL, args)
#endif

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

static PRBool LoadExtraSharedLib(const char *name, char **soname, PRBool tryToGetSoname)
{
    PRBool ret = PR_TRUE;
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
        char *sonamesListFromPref = PREF_PLUGINS_SONAME;
        char *sonameList = NULL;
        PRBool prefSonameListIsSet = PR_TRUE;
        res = prefs->GetCharPref(sonamesListFromPref, &sonameList);
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
                
                PRBool head = PR_TRUE;
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
                PRBool tryToGetSoname = PR_TRUE;
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
                
                
                
                prefs->SetCharPref(sonamesListFromPref, (const char *)sonameListToSave);
            }
            PL_strfree(sonameList);
        }
    }
}
#endif 







PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
    nsCAutoString filename;
    if (NS_FAILED(file->GetNativeLeafName(filename)))
        return PR_FALSE;

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

#ifdef AIX








static char *javaLibPath = NULL;

static void SetJavaLibPath(const nsCString& pluginPath)
{
    
    
    if (javaLibPath)
        return;

    nsCAutoString javaDir, newLibPath;

    PRInt32 pos = pluginPath.RFindChar('/');
    if (pos == kNotFound || pos == 0)
        return;

    javaDir = Substring(pluginPath, 0, pos);
    LOG(("AIX: Java dir is %s\n", javaDir.get()));

    
    newLibPath += javaDir;

    
    
    PRFileInfo info;
    javaDir.AppendLiteral("/classic");
    if (PR_GetFileInfo(javaDir.get(), &info) == PR_SUCCESS &&
        info.type == PR_FILE_DIRECTORY)
    {
        newLibPath.Append(':');
        newLibPath.Append(javaDir);
    }

    
    const char *currentLibPath = PR_GetEnv("LIBPATH");
    LOG(("AIX: current LIBPATH=%s\n", currentLibPath));
    if (currentLibPath && *currentLibPath) {
        newLibPath.Append(':');
        newLibPath.Append(currentLibPath);
    }

    
    
    
    javaLibPath = PR_smprintf("LIBPATH=%s", newLibPath.get());
    if (javaLibPath) {
        LOG(("AIX: new LIBPATH=%s\n", newLibPath.get()));
        PR_SetEnv(javaLibPath);
    }
}
#endif





nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
    PRLibSpec libSpec;
    libSpec.type = PR_LibSpec_Pathname;
    PRBool exists = PR_FALSE;
    mPlugin->Exists(&exists);
    if (!exists)
        return NS_ERROR_FILE_NOT_FOUND;

    nsresult rv;
    nsCAutoString path;
    rv = mPlugin->GetNativePath(path);
    if (NS_FAILED(rv))
        return rv;

#ifdef AIX
    nsCAutoString leafName;
    rv = mPlugin->GetNativeLeafName(leafName);
    if (NS_FAILED(rv))
        return rv;

    if (StringBeginsWith(leafName, NS_LITERAL_CSTRING("libjavaplugin_oji")))
        SetJavaLibPath(path);
#endif

    libSpec.value.pathname = path.get();

#if defined(MOZ_WIDGET_GTK2)

    
    
    
    
    
    
    
    
    
    
    


#if defined(SOLARIS) || defined(HPUX)
    
    pLibrary = outLibrary = PR_LoadLibraryWithFlags(libSpec, PR_LD_NOW);
#else
    
    pLibrary = outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
#endif
    if (!pLibrary) {
        LoadExtraSharedLibs();
        
        pLibrary = outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
        if (!pLibrary)
            DisplayPR_LoadLibraryErrorMessage(libSpec.value.pathname);
    }
#else
    pLibrary = outLibrary = PR_LoadLibraryWithFlags(libSpec, 0);
#endif  

#ifdef NS_DEBUG
    printf("LoadPlugin() %s returned %lx\n", 
           libSpec.value.pathname, (unsigned long)pLibrary);
#endif
    
    return NS_OK;
}




nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
    nsresult rv;
    const char* mimedescr = 0, *name = 0, *description = 0;

    
    
    nsIServiceManagerObsolete* mgr;
    nsServiceManager::GetGlobalServiceManager((nsIServiceManager**)&mgr);

    nsFactoryProc nsGetFactory =
        (nsFactoryProc) PR_FindFunctionSymbol(pLibrary, "NSGetFactory");

    nsCOMPtr<nsIPlugin> plugin;

    if (nsGetFactory) {
        
        
        
        
        static NS_DEFINE_CID(kPluginCID, NS_PLUGIN_CID);

        nsCOMPtr<nsIFactory> factory;
        rv = nsGetFactory(mgr, kPluginCID, nsnull, nsnull, 
			  getter_AddRefs(factory));

        if (NS_FAILED(rv)) {
            
            
            
            
            rv = ns4xPlugin::CreatePlugin(mgr, 0, 0, pLibrary,
                                          getter_AddRefs(plugin));
            if (NS_FAILED(rv))
                return rv;
        } else {
            plugin = do_QueryInterface(factory);
        }
    } else {
        
        
        rv = ns4xPlugin::CreatePlugin(mgr, 0, 0, pLibrary, 
				      getter_AddRefs(plugin));
        if (NS_FAILED(rv)) return rv;
    }

    if (plugin) {
        plugin->GetMIMEDescription(&mimedescr);
#ifdef NS_DEBUG
        printf("GetMIMEDescription() returned \"%s\"\n", mimedescr);
#endif
        if (NS_FAILED(rv = ParsePluginMimeDescription(mimedescr, info)))
            return rv;
        nsCAutoString filename;
        if (NS_FAILED(rv = mPlugin->GetNativePath(filename)))
            return rv;
        info.fFileName = PL_strdup(filename.get());
        plugin->GetValue(nsPluginVariable_NameString, &name);
        if (!name)
            name = PL_strrchr(info.fFileName, '/') + 1;
        info.fName = PL_strdup(name);

        plugin->GetValue(nsPluginVariable_DescriptionString, &description);
        if (!description)
            description = "";
        info.fDescription = PL_strdup(description);
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

    if (info.fFileName != nsnull)
        PL_strfree(info.fFileName);

    return NS_OK;
}
