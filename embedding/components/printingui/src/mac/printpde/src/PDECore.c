



















































#include <Carbon/Carbon.h>
#include <Print/PMPrintingDialogExtensions.h>

#include "PDECustom.h"
#include "PDEUtilities.h"


enum SyncDirection {
    kSyncTicketFromPane = FALSE,
    kSyncPaneFromTicket = TRUE
};










static HRESULT   MyQueryInterface   (void*, REFIID, LPVOID*);
static ULONG     MyIUnknownRetain   (void*);
static ULONG     MyIUnknownRelease  (void*);

static OSStatus  MyPMRetain         (PMPlugInHeaderInterface*);
static OSStatus  MyPMRelease        (PMPlugInHeaderInterface**);
static OSStatus  MyPMGetAPIVersion  (PMPlugInHeaderInterface*, PMPlugInAPIVersion*);

static OSStatus  MyPrologue   (PMPDEContext*, OSType*, CFStringRef*, CFStringRef*, UInt32*, UInt32*);
static OSStatus  MyInitialize (PMPDEContext, PMPDEFlags*, PMPDERef, ControlRef, PMPrintSession);
static OSStatus  MySync       (PMPDEContext, PMPrintSession, Boolean);
static OSStatus  MyGetSummary (PMPDEContext, CFArrayRef*, CFArrayRef*);
static OSStatus  MyOpen       (PMPDEContext);
static OSStatus  MyClose      (PMPDEContext);
static OSStatus  MyTerminate  (PMPDEContext, OSStatus);








typedef struct
{
    const IUnknownVTbl *vtable;
    SInt32 refCount;
    CFUUIDRef factoryID;

} MyIUnknownInstance;

typedef struct
{
    const PlugInIntfVTable *vtable;
    SInt32 refCount;

} MyPDEInstance;


#pragma mark -














extern void* MyCFPlugInFactory (
    CFAllocatorRef allocator, 
    CFUUIDRef typeUUID
)

{
    
    static const IUnknownVTbl sMyIUnknownVTable =
    {
        NULL, 
        MyQueryInterface,
        MyIUnknownRetain,
        MyIUnknownRelease
    };
    
    CFBundleRef         myBundle    = NULL;
    CFDictionaryRef     myTypes     = NULL;
    CFStringRef         requestType = NULL;
    CFArrayRef          factories   = NULL;
    CFStringRef         factory     = NULL;
    CFUUIDRef           factoryID   = NULL;
    MyIUnknownInstance  *instance   = NULL;

    myBundle = MyGetBundle();

    if (myBundle != NULL)
    {
        myTypes = CFBundleGetValueForInfoDictionaryKey (
            myBundle, CFSTR("CFPlugInTypes"));
    
        if (myTypes != NULL) 
        {
            
            
            requestType = CFUUIDCreateString (allocator, typeUUID);
            if (requestType != NULL)
            {
                factories = CFDictionaryGetValue (myTypes, requestType);
                CFRelease (requestType);
                if (factories != NULL) 
                {   
                    factory = CFArrayGetValueAtIndex (factories, 0);
                    if (factory != NULL) 
                    {
                       
                        factoryID = CFUUIDCreateFromString (
                            allocator, factory);
                        if (factoryID != NULL)
                        {
                            
                            instance = malloc (sizeof(MyIUnknownInstance));
                            if (instance != NULL)
                            {
                                instance->vtable = &sMyIUnknownVTable;
                                instance->refCount = 1;
                                instance->factoryID = factoryID;
                                CFPlugInAddInstanceForFactory (factoryID);
                            }
                            else {
                                CFRelease (factoryID);
                            }
                        }
                    }
                }
            }
        }
    }

    MyDebugMessage ("Factory", (SInt32) instance);
    return instance;
}


#pragma mark -











 
static HRESULT MyQueryInterface (
    void *this, 
    REFIID iID, 
    LPVOID *ppv
)

{

    

    static const PlugInIntfVTable sMyPDEVTable = 
    { 
        {
            MyPMRetain,
            MyPMRelease, 
            MyPMGetAPIVersion 
        },
        MyPrologue,
        MyInitialize,
        MySync,
        MyGetSummary,
        MyOpen,
        MyClose,
        MyTerminate
    };


    CFUUIDRef requestID = NULL;
    CFUUIDRef actualID  = NULL;
    HRESULT   result = E_UNEXPECTED;

    
    
    requestID = CFUUIDCreateFromUUIDBytes (kCFAllocatorDefault, iID);
    if (requestID != NULL)
    {
        
        actualID = CFUUIDCreateFromString (kCFAllocatorDefault, kDialogExtensionIntfIDStr);
        if (actualID != NULL)
        {
            if (CFEqual (requestID, actualID))
            {
                
        
                MyPDEInstance *instance = malloc (sizeof(MyPDEInstance));
        
                if (instance != NULL)
                {
                    instance->vtable = &sMyPDEVTable;
                    instance->refCount = 1;
                    *ppv = instance;
                    result = S_OK;
                }
            }
            else
            {
                if (CFEqual (requestID, IUnknownUUID))
                {
                    
                    MyIUnknownRetain (this);
                    *ppv = this;
                    result = S_OK;
                }
                else
                {
                    *ppv = NULL;
                    result = E_NOINTERFACE;
                }
            }
            CFRelease (actualID);
        }
        CFRelease (requestID);
    }

    MyDebugMessage("MyQueryInterface", result);
    return result;
}


















static ULONG MyIUnknownRetain (void* this)
{   
    MyIUnknownInstance* instance = (MyIUnknownInstance*) this;
    ULONG refCount = 1;
        
    if (instance != NULL) {
        refCount = ++instance->refCount;
    }

    MyDebugMessage("MyIUnknownRetain", refCount);
    return refCount;
}
















static ULONG MyIUnknownRelease (void* this)
{
    MyIUnknownInstance* instance = (MyIUnknownInstance*) this;
    ULONG refCount = 0;

    if (instance != NULL)
    {
        refCount = --instance->refCount;
        if (refCount == 0)
        {
            CFPlugInRemoveInstanceForFactory (instance->factoryID);
            CFRelease (instance->factoryID);
            free (instance);
        }
    }

    MyDebugMessage("MyIUnknownRelease", refCount);
    return refCount;      
}








static OSStatus MyPMRetain (PMPlugInHeaderInterface* this)
{
    MyPDEInstance* instance = (MyPDEInstance*) this;
    ULONG refCount = 1;
    OSStatus result = noErr;
    
    if (instance != NULL) {
        refCount = ++instance->refCount;
    }

    MyDebugMessage("MyPMRetain", refCount);
    return result;
}








static OSStatus MyPMRelease (
    PMPlugInHeaderInterface** this
)

{
    MyPDEInstance* instance = (MyPDEInstance*) *this;
    ULONG refCount = 0;
    OSStatus result = noErr;

    
    *this = NULL;

    if(instance != NULL)
    {
        
        refCount = --instance->refCount;
    
        if (refCount == 0) 
        {
            free (instance);
            MyFreeTitle();
            MyFreeBundle();
        }
    }

    MyDebugMessage("MyPMRelease", refCount);
    return result;
}








static OSStatus MyPMGetAPIVersion (
    PMPlugInHeaderInterface* this, 
    PMPlugInAPIVersion* versionPtr
)

{
    OSStatus result = noErr;

    
    versionPtr->buildVersionMajor = kPDEBuildVersionMajor;
    versionPtr->buildVersionMinor = kPDEBuildVersionMinor;
    versionPtr->baseVersionMajor = kPDEBaseVersionMajor;
    versionPtr->baseVersionMinor = kPDEBaseVersionMinor;
    
    MyDebugMessage("MyPMGetAPIVersion", result);
    return result;
}


#pragma mark -

















static OSStatus MyPrologue (
    PMPDEContext    *outContext,    
    OSType          *creator,       
    CFStringRef     *paneKind,      
    CFStringRef     *title,         
    UInt32          *maxH,          
    UInt32          *maxV           
)

{
    MyContext context = NULL;
    OSStatus result = kPMInvalidPDEContext;

    context = malloc (sizeof (MyContextBlock));

    if (context != NULL)
    {
        context->customContext = MyCreateCustomContext();
        context->initialized = FALSE;
        context->userPane = NULL;
        context->helpHandler = NULL;
        context->helpHandlerUPP = NULL;

        
        *outContext = (PMPDEContext) context;
        *creator    = kMozPDECreatorCode;
        *paneKind   = kMyPaneKindID;
        *title      = MyGetTitle();
        *maxH       = kMyMaxH;
        *maxV       = kMyMaxV;

        result = noErr;
    }

    MyDebugMessage("MyPrologue", result);
    return result;
}








static OSStatus MyInitialize (   
    PMPDEContext inContext,
    PMPDEFlags* flags,
    PMPDERef ref,
    ControlRef userPane,
    PMPrintSession session
)

{
    MyContext context = (MyContext) inContext;
    OSStatus result = noErr;

    *flags = kPMPDENoFlags;
    context->userPane = userPane;

    result = MySync (
        inContext, session, kSyncPaneFromTicket);

    MyDebugMessage("MyInitialize", result);
    return result;
}








static OSStatus MySync (
    PMPDEContext inContext,
    PMPrintSession session,
    Boolean syncDirection
)

{
    MyContext context = (MyContext) inContext;
    OSStatus result = noErr;

    if (syncDirection == kSyncPaneFromTicket)
    {
        result = MySyncPaneFromTicket (context->customContext, session);
    }
    else
    {
        result = MySyncTicketFromPane (context->customContext, session);
    }

    return result;
}












static OSStatus MyGetSummary (
    PMPDEContext inContext,
    CFArrayRef *titles,
    CFArrayRef *values
)

{
    MyContext context = (MyContext) inContext;
    CFMutableArrayRef titleArray = NULL;
    CFMutableArrayRef valueArray = NULL;

    
    OSStatus result = kPMInvalidPDEContext;

    
    

    titleArray = CFArrayCreateMutable (
        kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

    if (titleArray != NULL)
    {
        valueArray = CFArrayCreateMutable (
            kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);

        if (valueArray != NULL)
        {
            result = MyGetSummaryText (
                context->customContext, 
                titleArray, 
                valueArray
            );
        }
    }

    if (result != noErr)
    {
        if (titleArray != NULL)
        {
            CFRelease (titleArray);
            titleArray = NULL;
        }
        if (valueArray != NULL)
        {
            CFRelease (valueArray);
            valueArray = NULL;
        }
    }

    *titles = titleArray;
    *values = valueArray;

    MyDebugMessage("MyGetSummary", result);
    return result;
}








static OSStatus MyOpen (PMPDEContext inContext)
{
    MyContext context = (MyContext) inContext;
    OSStatus result = noErr;

    if (!context->initialized)
    {
        
    
        IBNibRef nib = NULL;
    
        result = CreateNibReferenceWithCFBundle (
            MyGetBundle(), 
            kMyNibFile,
            &nib
        );

        if (result == noErr)
        {
            WindowRef nibWindow = NULL;
 
            result = CreateWindowFromNib (
                nib, 
                kMyNibWindow, 
                &nibWindow
            );

            if (result == noErr)
            {
                result = MyEmbedCustomControls (
                    context->customContext, 
                    nibWindow, 
                    context->userPane
                );

                if (result == noErr)
                {
                    context->initialized = TRUE;
                }

                DisposeWindow (nibWindow);
            }

            DisposeNibReference (nib);
        }
    }

    if (context->initialized)
    {
        result = MyInstallHelpEventHandler (
            GetControlOwner (context->userPane), 
            &(context->helpHandler),
            &(context->helpHandlerUPP)
        );
    }
    
    MyDebugMessage("MyOpen", result);
    return result;
}








static OSStatus MyClose (PMPDEContext inContext)
{
    MyContext context = (MyContext) inContext;
    OSStatus result = noErr;

    result = MyRemoveHelpEventHandler (
        &(context->helpHandler),
        &(context->helpHandlerUPP)
    );

    MyDebugMessage("MyClose", result);
    return result;
}








static OSStatus MyTerminate (
    PMPDEContext inContext, 
    OSStatus inStatus
)

{
    MyContext context = (MyContext) inContext;
    OSStatus result = noErr;

    if (context != NULL)
    {
        result = MyRemoveHelpEventHandler (
            &(context->helpHandler),
            &(context->helpHandlerUPP)
        );

        if (context->customContext != NULL) {
            MyReleaseCustomContext (context->customContext);
        }

        free (context);
    }

    MyDebugMessage("MyTerminate", result);
    return result;
}



