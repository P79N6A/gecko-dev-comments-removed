








































#include "nsSoftwareUpdate.h"
#include "nsSoftwareUpdateRun.h"

#include "nsInstall.h"

#include "nsNetUtil.h"

#include "nspr.h"
#include "plstr.h"
#include "jsapi.h"

#include "nsIZipReader.h"
#include "nsInstallTrigger.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsStringEnumerator.h"

#include "nsIJAR.h"
#include "nsIPrincipal.h"

#include "nsIExtensionManager.h"

static NS_DEFINE_CID(kSoftwareUpdateCID,  NS_SoftwareUpdate_CID);

extern JSObject *InitXPInstallObjects(JSContext *jscontext, 
                                      nsIFile* jarfile, const PRUnichar* url, 
                                      const PRUnichar* args, PRUint32 flags, 
                                      CHROMEREG_IFACE* registry, 
                                      nsIZipReader* hZip);
extern nsresult InitInstallVersionClass(JSContext *jscontext, JSObject *global, void** prototype);
extern nsresult InitInstallTriggerGlobalClass(JSContext *jscontext, JSObject *global, void** prototype);


JS_STATIC_DLL_CALLBACK(void) XPInstallErrorReporter(JSContext *cx, const char *message, JSErrorReport *report);
static PRInt32  GetInstallScriptFromJarfile(nsIZipReader* hZip, char** scriptBuffer, PRUint32 *scriptLength);
static PRInt32  OpenAndValidateArchive(nsIZipReader* hZip, nsIFile* jarFile, nsIPrincipal* aPrincipal);

static nsresult SetupInstallContext(nsIZipReader* hZip, nsIFile* jarFile, const PRUnichar* url, const PRUnichar* args, 
                                    PRUint32 flags, CHROMEREG_IFACE* reg, JSRuntime *jsRT, JSContext **jsCX, JSObject **jsGlob);

extern "C" void PR_CALLBACK RunInstallOnThread(void *data);


nsresult VerifySigning(nsIZipReader* hZip, nsIPrincipal* aPrincipal)
{
    if (!aPrincipal) 
        return NS_OK; 

    PRBool hasCert;
    aPrincipal->GetHasCertificate(&hasCert);
    if (!hasCert)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIJAR> jar(do_QueryInterface(hZip));
    if (!jar)
        return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIPrincipal> principal;
    nsresult rv = jar->GetCertificatePrincipal(nsnull, getter_AddRefs(principal));
    if (NS_FAILED(rv) || !principal)
        return NS_ERROR_FAILURE;
    
    PRUint32 entryCount = 0;

    
    nsCOMPtr<nsIUTF8StringEnumerator> entries;
    rv = hZip->FindEntries(nsnull, getter_AddRefs(entries));
    if (NS_FAILED(rv))
        return rv;

    PRBool more;
    nsCAutoString name;
    while (NS_SUCCEEDED(entries->HasMore(&more)) && more)
    {
        rv = entries->GetNext(name);
        if (NS_FAILED(rv)) return rv;
        
        
        
        if ((name.Last() == '/') || 
            (PL_strncasecmp("META-INF/", name.get(), 9) == 0))
            continue;

        
        entryCount++;

        
        rv = jar->GetCertificatePrincipal(name.get(), getter_AddRefs(principal));
        if (NS_FAILED(rv) || !principal) return NS_ERROR_FAILURE;

        PRBool equal;
        rv = principal->Equals(aPrincipal, &equal);
        if (NS_FAILED(rv) || !equal) return NS_ERROR_FAILURE;
    }

    
    PRUint32 manifestEntryCount;
    rv = jar->GetManifestEntriesCount(&manifestEntryCount);
    if (NS_FAILED(rv))
        return rv;

    if (entryCount != manifestEntryCount)
        return NS_ERROR_FAILURE;  

    return NS_OK;
}









static void
XPInstallErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    nsresult rv;

    
    nsCOMPtr<nsIConsoleService> consoleService
        (do_GetService("@mozilla.org/consoleservice;1"));

    



    nsCOMPtr<nsIScriptError>
        errorObject(do_CreateInstance("@mozilla.org/scripterror;1"));

    if (consoleService != nsnull && errorObject != nsnull && report != nsnull) {
        




        PRUint32 column = report->uctokenptr - report->uclinebuf;

        rv = errorObject->Init(NS_REINTERPRET_CAST(const PRUnichar*, report->ucmessage),
                               NS_ConvertASCIItoUTF16(report->filename).get(),
                               NS_REINTERPRET_CAST(const PRUnichar*, report->uclinebuf),
                               report->lineno, column, report->flags,
                               "XPInstall JavaScript");
        if (NS_SUCCEEDED(rv)) {
            rv = consoleService->LogMessage(errorObject);
            if (NS_SUCCEEDED(rv)) {
              
              
              
            }
        }
    }

    
    nsCOMPtr<nsISoftwareUpdate> softwareUpdate =
             do_GetService(kSoftwareUpdateCID, &rv);

    if (NS_FAILED(rv))
    {
        NS_WARNING("shouldn't have RunInstall() if we can't get SoftwareUpdate");
        return;
    }

    nsCOMPtr<nsIXPIListener> listener;
    softwareUpdate->GetMasterListener(getter_AddRefs(listener));

    if(listener)
    {
        nsAutoString logMessage;
        if (report)
        {
            logMessage.AssignLiteral("Line: ");
            logMessage.AppendInt(report->lineno, 10);
            logMessage.AppendLiteral("\t");
            if (report->ucmessage)
                logMessage.Append( NS_REINTERPRET_CAST(const PRUnichar*, report->ucmessage) );
            else
                logMessage.AppendWithConversion( message );
        }
        else
            logMessage.AssignWithConversion( message );

        listener->OnLogComment( logMessage.get() );
    }
}












static PRInt32
OpenAndValidateArchive(nsIZipReader* hZip, nsIFile* jarFile, nsIPrincipal* aPrincipal)
{
    if (!jarFile)
        return nsInstall::DOWNLOAD_ERROR;

    nsCOMPtr<nsIFile> jFile;
    nsresult rv =jarFile->Clone(getter_AddRefs(jFile));
    if (NS_SUCCEEDED(rv))
        rv = hZip->Open(jFile);

    if (NS_FAILED(rv))
        return nsInstall::CANT_READ_ARCHIVE;

    
    rv = hZip->Test(nsnull);
    if (NS_FAILED(rv))
    {
        NS_WARNING("CRC check of archive failed!");
        return nsInstall::CANT_READ_ARCHIVE;
    }

    rv = VerifySigning(hZip, aPrincipal);
    if (NS_FAILED(rv))
    {
        NS_WARNING("Signing check of archive failed!");
        return nsInstall::INVALID_SIGNATURE;
    }
 
    return nsInstall::SUCCESS;
}









static PRInt32
GetInstallScriptFromJarfile(nsIZipReader* hZip, char** scriptBuffer, PRUint32 *scriptLength)
{
    PRInt32 result = NS_OK;

    *scriptBuffer = nsnull;
    *scriptLength = 0;

    
    nsCOMPtr<nsIInputStream> instream;
    nsresult rv = hZip->GetInputStream("install.js", getter_AddRefs(instream));
    if ( NS_SUCCEEDED(rv) )
    {
        
        char* buffer;
        PRUint32 bufferLength;
        PRUint32 readLength;
        result = nsInstall::CANT_READ_ARCHIVE;

        rv = instream->Available(&bufferLength);
        if (NS_SUCCEEDED(rv))
        {
            buffer = new char[bufferLength + 1];

            if (buffer != nsnull)
            {
                rv = instream->Read(buffer, bufferLength, &readLength);

                if (NS_SUCCEEDED(rv) && readLength > 0)
                {
                    *scriptBuffer = buffer;
                    *scriptLength = readLength;
                    result = NS_OK;
                }
                else
                {
                    delete [] buffer;
                }
            }
        }
        instream->Close();
    }
    else
    {
        result = nsInstall::NO_INSTALL_SCRIPT;
    }

    return result;
}















static nsresult SetupInstallContext(nsIZipReader* hZip,
                                    nsIFile* jarFile,
                                    const PRUnichar* url,
                                    const PRUnichar* args,
                                    PRUint32 flags,
                                    CHROMEREG_IFACE* reg,
                                    JSRuntime *rt,
                                    JSContext **jsCX,
                                    JSObject **jsGlob)
{
    JSContext   *cx;
    JSObject    *glob;

    *jsCX   = nsnull;
    *jsGlob = nsnull;

    if (!rt)
        return NS_ERROR_OUT_OF_MEMORY;

    cx = JS_NewContext(rt, 8192);
    if (!cx)
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    JS_SetErrorReporter(cx, XPInstallErrorReporter);

    JS_BeginRequest(cx);
    glob = InitXPInstallObjects(cx, jarFile, url, args, flags, reg, hZip);
    if (!glob)
    {
        JS_DestroyContext(cx);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    JS_InitStandardClasses(cx, glob);

    
    InitInstallVersionClass(cx, glob, nsnull);
    InitInstallTriggerGlobalClass(cx, glob, nsnull);
    JS_EndRequest(cx);
    *jsCX   = cx;
    *jsGlob = glob;

    return NS_OK;
}










PRInt32 RunInstall(nsInstallInfo *installInfo)
{
    if (installInfo->GetFlags() & XPI_NO_NEW_THREAD)
    {
        RunInstallOnThread((void *)installInfo);
    }
    else
    {
        PR_CreateThread(PR_USER_THREAD,
                        RunInstallOnThread,
                        (void*)installInfo,
                        PR_PRIORITY_NORMAL,
                        PR_GLOBAL_THREAD,
                        PR_UNJOINABLE_THREAD,
                        0);
    }
    return 0;
}










extern "C" void PR_CALLBACK RunInstallOnThread(void *data)
{
    nsInstallInfo *installInfo = (nsInstallInfo*)data;

    char        *scriptBuffer = nsnull;
    PRUint32    scriptLength;

    JSRuntime   *rt;
    JSContext   *cx;
    JSObject    *glob;

    static NS_DEFINE_IID(kZipReaderCID,  NS_ZIPREADER_CID);

    nsresult rv;
    nsCOMPtr<nsIZipReader> hZip = do_CreateInstance(kZipReaderCID, &rv);
    if (NS_FAILED(rv))
        return;

    
    
    PRInt32     finalStatus;

    nsCOMPtr<nsIXPIListener> listener;

    nsCOMPtr<nsISoftwareUpdate> softwareUpdate =
             do_GetService(kSoftwareUpdateCID, &rv);

    if (NS_FAILED(rv))
    {
        NS_WARNING("shouldn't have RunInstall() if we can't get SoftwareUpdate");
        return;
    }

    softwareUpdate->SetActiveListener( installInfo->GetListener() );
    softwareUpdate->GetMasterListener(getter_AddRefs(listener));

    if(listener)
        listener->OnInstallStart( installInfo->GetURL() );

    nsCOMPtr<nsIFile> jarpath = installInfo->GetFile();

    finalStatus = OpenAndValidateArchive( hZip,
                                          jarpath,
                                          installInfo->mPrincipal);

    if (finalStatus == nsInstall::SUCCESS)
    {
#ifdef MOZ_XUL_APP
        if (NS_SUCCEEDED(hZip->Test("install.rdf")) && !(nsSoftwareUpdate::GetProgramDirectory()))
        {
            hZip->Close();
            
            nsIExtensionManager* em = installInfo->GetExtensionManager();
            if (em)
            {
                rv = em->InstallItemFromFile(jarpath, 
                                             NS_INSTALL_LOCATION_APPPROFILE);
                if (NS_FAILED(rv))
                    finalStatus = nsInstall::EXECUTION_ERROR;
            } else {
                finalStatus = nsInstall::UNEXPECTED_ERROR;
            }
            
            
        } else
#endif
        {
            
            
            finalStatus = GetInstallScriptFromJarfile( hZip,
                                                       &scriptBuffer,
                                                       &scriptLength);
            if ( finalStatus == NS_OK && scriptBuffer )
            {
                
                rt = JS_NewRuntime(4L * 1024L * 1024L);

                rv = SetupInstallContext( hZip, jarpath,
                                          installInfo->GetURL(),
                                          installInfo->GetArguments(),
                                          installInfo->GetFlags(),
                                          installInfo->GetChromeRegistry(),
                                          rt, &cx, &glob);

                if (NS_SUCCEEDED(rv))
                {
                    
                    jsval rval;
                    jsval installedFiles;
                    JS_BeginRequest(cx); 
                                        
                    PRBool ok = JS_EvaluateScript(  cx,
                                                    glob,
                                                    scriptBuffer,
                                                    scriptLength,
                                                    nsnull,
                                                    0,
                                                    &rval);


                    if(!ok)
                    {
                        
                        if(JS_GetProperty(cx, glob, "_installedFiles", &installedFiles) &&
                          JSVAL_TO_BOOLEAN(installedFiles))
                        {
                            nsInstall *a = (nsInstall*)JS_GetPrivate(cx, glob);
                            a->InternalAbort(nsInstall::SCRIPT_ERROR);
                        }

                        finalStatus = nsInstall::SCRIPT_ERROR;
                    }
                    else
                    {
                        
                        
                        

                        if(JS_GetProperty(cx, glob, "_installedFiles", &installedFiles) &&
                          JSVAL_TO_BOOLEAN(installedFiles))
                        {
                            
                            nsInstall *a = (nsInstall*)JS_GetPrivate(cx, glob);
                            a->InternalAbort(nsInstall::MALFORMED_INSTALL);
                        }

                        jsval sent;
                        if ( JS_GetProperty( cx, glob, "_finalStatus", &sent ) )
                            finalStatus = JSVAL_TO_INT(sent);
                        else
                            finalStatus = nsInstall::UNEXPECTED_ERROR;
                    }
                    JS_EndRequest(cx); 
                    JS_DestroyContextMaybeGC(cx);
                }
                else
                {
                    
                    finalStatus = nsInstall::UNEXPECTED_ERROR;
                }

                
                JS_DestroyRuntime(rt);
            }
        }
        
        hZip = 0;
    }

    if(listener)
        listener->OnInstallDone( installInfo->GetURL(), finalStatus );

    if (scriptBuffer) delete [] scriptBuffer;

    softwareUpdate->SetActiveListener(0);
    softwareUpdate->InstallJarCallBack();
}











extern "C" void PR_CALLBACK RunChromeInstallOnThread(void *data)
{
    nsresult rv;

    NS_ASSERTION(data, "No nsInstallInfo passed to Chrome Install");
    nsInstallInfo *info = (nsInstallInfo*)data;
    nsIXPIListener* listener = info->GetListener();

    if (listener)
        listener->OnInstallStart(info->GetURL());

    
    CHROMEREG_IFACE* reg = info->GetChromeRegistry();
    NS_ASSERTION(reg, "We shouldn't get here without a chrome registry.");

    if (reg)
    {
#ifdef MOZ_XUL_APP
        if (info->GetType() == CHROME_SKIN) {
            static NS_DEFINE_CID(kZipReaderCID,  NS_ZIPREADER_CID);
            nsCOMPtr<nsIZipReader> hZip = do_CreateInstance(kZipReaderCID, &rv);
            if (NS_SUCCEEDED(rv) && hZip) {
                rv = hZip->Open(info->GetFile());
                if (NS_SUCCEEDED(rv))
                {
                    rv = hZip->Test("install.rdf");
                    nsIExtensionManager* em = info->GetExtensionManager();
                    if (NS_SUCCEEDED(rv) && em) {
                        rv = em->InstallItemFromFile(info->GetFile(), 
                                                     NS_INSTALL_LOCATION_APPPROFILE);
                    }
                }
                hZip->Close();
            }
            
            
            info->GetFile()->Remove(PR_FALSE);
        }
#else
        PRBool isSkin    = (info->GetType() & CHROME_SKIN);
        PRBool isLocale  = (info->GetType() & CHROME_LOCALE);
        PRBool isContent = (info->GetType() & CHROME_CONTENT);
        PRBool selected  = (info->GetFlags() != 0);

        const nsCString& spec = info->GetFileJARSpec();

        if ( isContent )
            rv = reg->InstallPackage(spec.get(), PR_TRUE);

        if ( isSkin )
        {
            rv = reg->InstallSkin(spec.get(), PR_TRUE, PR_FALSE);
                
            if (NS_SUCCEEDED(rv) && selected)
            {
                NS_ConvertUTF16toUTF8 utf8Args(info->GetArguments());
                rv = reg->SelectSkin(utf8Args, PR_TRUE);
            }
        }

        if ( isLocale )
        {
            rv = reg->InstallLocale(spec.get(), PR_TRUE);

            if (NS_SUCCEEDED(rv) && selected)
            {
                NS_ConvertUTF16toUTF8 utf8Args(info->GetArguments());
                rv = reg->SelectLocale(utf8Args, PR_TRUE);
            }
        }

        
        if ( isSkin && selected )
            reg->RefreshSkins();

#ifdef RELOAD_CHROME_WORKS

            if ( isContent || (isLocale && selected) )
                reg->ReloadChrome();
#endif
#endif
    }

    if (listener)
        listener->OnInstallDone(info->GetURL(), nsInstall::SUCCESS);

    delete info;
}
