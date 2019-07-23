














































#include "nsPluginsDir.h"
#include "prlink.h"
#include "plstr.h"
#include "prmem.h"
#include "nsReadableUtils.h"
#include "nsString.h"

#include <File.h>
#include <AppFileInfo.h>
#include <Message.h>
#include <String.h>




 
static char* GetFileName(const char* pathname)
{
        const char* filename = nsnull;
                
        
        filename = PL_strrchr(pathname, '/');
        if(filename)
                ++filename;
        else
                filename = pathname;

        return PL_strdup(filename);
}

static nsresult GetMimeExtensions(const char *mimeType, char *extensions, int extLen)
{
    
    if (!mimeType || !extensions || extLen < 1) return NS_ERROR_FAILURE;
    extensions[0] = '\0';
    
    
    BMimeType mime(mimeType) ;
    if (mime.InitCheck() != B_OK)
        return NS_ERROR_FAILURE;
    
    
    
    BString extStr("");
    BMessage extMsg;
    mime.GetFileExtensions(&extMsg);
    uint32 type;
    int32 types_num;
    if (extMsg.GetInfo("extensions", &type, &types_num) != B_OK
        || type != B_STRING_TYPE || types_num == 0)
        return NS_ERROR_FAILURE;
    
    for (int i = 0 ; i < types_num ; i ++) {
        const char *ext;
        if (extMsg.FindString("extensions", i, &ext) != B_OK) {
            break;
        }
        if (i > 0)
            extStr.Append(",");
        extStr.Append(ext);
    }
    PL_strncpyz(extensions, extStr.String(), extLen) ;
    
    return NS_OK;
}





PRBool nsPluginsDir::IsPluginFile(nsIFile* file)
{
	return PR_TRUE;
}





nsPluginFile::nsPluginFile(nsIFile* spec)
:	mPlugin(spec)
{
	
}

nsPluginFile::~nsPluginFile()
{
	
}





nsresult nsPluginFile::LoadPlugin(PRLibrary* &outLibrary)
{
        nsCAutoString path;
        nsresult rv = mPlugin->GetNativePath(path);
        if (NS_OK != rv) {
            return rv;
        }
        pLibrary = outLibrary = PR_LoadLibrary(path.get());

#ifdef NS_DEBUG
        printf("LoadPlugin() %s returned %lx\n",path,(unsigned long)pLibrary);
#endif

        return NS_OK;
}

typedef char* (*BeOS_Plugin_GetMIMEDescription)();





nsresult nsPluginFile::GetPluginInfo(nsPluginInfo& info)
{
    nsCAutoString fpath;
    nsresult rv = mPlugin->GetNativePath(fpath);
    if (NS_OK != rv) {
        return rv;
    }
    const char *path = fpath.get();
    int i;

#ifdef NS_PLUGIN_BEOS_DEBUG
    printf("nsPluginFile::GetPluginInfo() an attempt to load MIME String\n");
    printf("path = <%s>\n", path);
#endif

    
    BFile file(path, B_READ_ONLY);
    if (file.InitCheck() != B_OK)
        return NS_ERROR_FAILURE;

    BAppFileInfo appinfo(&file);
    if (appinfo.InitCheck() != B_OK)
        return NS_ERROR_FAILURE;

    BMessage msg;
    if (appinfo.GetSupportedTypes(&msg) != B_OK)
        return NS_ERROR_FAILURE;

    uint32 type;
    int32 types_num;
    if (msg.GetInfo("types", &type, &types_num) != B_OK
        || type != B_STRING_TYPE)
        return NS_ERROR_FAILURE;

    
    info.fMimeTypeArray =(char **)PR_Malloc(types_num * sizeof(char *));
    info.fMimeDescriptionArray =(char **)PR_Malloc(types_num * sizeof(char *));
    info.fExtensionArray =(char **)PR_Malloc(types_num * sizeof(char *));

    for (i = 0 ; i < types_num ; i ++) {
        
        const char *mtype;
        if (msg.FindString("types", i, &mtype) != B_OK) {
            types_num = i;
            break;
        }
        
        
        char desc[B_MIME_TYPE_LENGTH+1] = "";
        BMimeType mime(mtype) ;
        if (mime.InitCheck() == B_OK)
            mime.GetShortDescription(desc);
        
        
        char extensions[B_MIME_TYPE_LENGTH+1] = "";
        GetMimeExtensions(mtype, extensions, B_MIME_TYPE_LENGTH+1);

        #ifdef NS_PLUGIN_BEOS_DEBUG
            printf("  mime = %30s | %10s | %15s |\n", 
                mtype, extensions, desc);
        #endif
        
        info.fMimeTypeArray[i] = PL_strdup( mtype ? mtype : (char *)"" ) ;
        info.fMimeDescriptionArray[i] = PL_strdup( desc ) ;
        info.fExtensionArray[i] = PL_strdup( extensions );
    }

    
    version_info vinfo;
    if (appinfo.GetVersionInfo(&vinfo, B_APP_VERSION_KIND) == B_OK
        && *vinfo.short_info) {
        
        info.fName = ToNewCString(NS_ConvertUTF8toUTF16(vinfo.short_info));
        info.fDescription = ToNewCString(NS_ConvertUTF8toUTF16(vinfo.long_info));
    } else {
        
        info.fName = GetFileName(path);
        info.fDescription = PL_strdup("");
    }

    info.fVariantCount = types_num;
    info.fFileName = PL_strdup(path);


#ifdef NS_PLUGIN_BEOS_DEBUG
    printf("info.fFileName = %s\n", info.fFileName);
    printf("info.fName = %s\n", info.fName);
    printf("info.fDescription = %s\n", info.fDescription);
#endif

    return NS_OK;
}

nsresult nsPluginFile::FreePluginInfo(nsPluginInfo& info)
{
    if (info.fName)
        PL_strfree(info.fName);

    if (info.fDescription)
        PL_strfree(info.fDescription);

    for (PRUint32 i = 0; i < info.fVariantCount; i++) {
        if (info.fMimeTypeArray[i])
            PL_strfree(info.fMimeTypeArray[i]);

        if (info.fMimeDescriptionArray[i])
            PL_strfree(info.fMimeDescriptionArray[i]);

        if (info.fExtensionArray[i])
            PL_strfree(info.fExtensionArray[i]);
    }

    PR_FREEIF(info.fMimeTypeArray);
    PR_FREEIF(info.fMimeDescriptionArray);
    PR_FREEIF(info.fExtensionArray);

    if (info.fFileName)
        PL_strfree(info.fFileName);

    return NS_OK;
}
