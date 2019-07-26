




#ifndef npapi_h_
#define npapi_h_

#include "nptypes.h"

#if defined(_WIN32) && !defined(__SYMBIAN32__)
#include <windef.h>
#ifndef XP_WIN
#define XP_WIN 1
#endif
#endif

#if defined(__SYMBIAN32__)
#ifndef XP_SYMBIAN
#define XP_SYMBIAN 1
#undef XP_WIN
#endif
#endif

#if defined(__APPLE_CC__) && !defined(XP_UNIX)
#ifndef XP_MACOSX
#define XP_MACOSX 1
#endif
#endif

#if defined(XP_MACOSX) && defined(__LP64__)
#define NP_NO_QUICKDRAW
#define NP_NO_CARBON
#endif

#if defined(XP_MACOSX)
#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>
#ifndef NP_NO_CARBON
#include <Carbon/Carbon.h>
#endif
#endif

#if defined(XP_UNIX)
#include <stdio.h>
#if defined(MOZ_X11)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif
#endif

#if defined(XP_SYMBIAN)
#include <QEvent>
#include <QRegion>
#endif





#define NP_VERSION_MAJOR 0
#define NP_VERSION_MINOR 27




























#define NP_INFO_ProductVersion      1
#define NP_INFO_MIMEType            2
#define NP_INFO_FileOpenName        3
#define NP_INFO_FileExtents         4

#define NP_INFO_FileDescription     5
#define NP_INFO_ProductName         6

#define NP_INFO_CompanyName         7
#define NP_INFO_FileVersion         8
#define NP_INFO_InternalName        9
#define NP_INFO_LegalCopyright      10
#define NP_INFO_OriginalFilename    11

#ifndef RC_INVOKED





typedef unsigned char NPBool;
typedef int16_t       NPError;
typedef int16_t       NPReason;
typedef char*         NPMIMEType;





#if !defined(__LP64__)
#if defined(XP_MACOSX)
#pragma options align=mac68k
#endif
#endif 




typedef struct _NPP
{
  void* pdata;      
  void* ndata;      
} NPP_t;

typedef NPP_t*  NPP;

typedef struct _NPStream
{
  void*    pdata; 
  void*    ndata; 
  const    char* url;
  uint32_t end;
  uint32_t lastmodified;
  void*    notifyData;
  const    char* headers; 









} NPStream;

typedef struct _NPByteRange
{
  int32_t  offset; 
  uint32_t length;
  struct _NPByteRange* next;
} NPByteRange;

typedef struct _NPSavedData
{
  int32_t len;
  void*   buf;
} NPSavedData;

typedef struct _NPRect
{
  uint16_t top;
  uint16_t left;
  uint16_t bottom;
  uint16_t right;
} NPRect;

typedef struct _NPSize
{
  int32_t width;
  int32_t height;
} NPSize;

typedef enum {
  NPFocusNext = 0,
  NPFocusPrevious = 1
} NPFocusDirection;





typedef enum {
  
  NPImageFormatBGRA32     = 0x1,
  
  NPImageFormatBGRX32     = 0x2 
} NPImageFormat;

typedef struct _NPAsyncSurface
{
  uint32_t version;
  NPSize size;
  NPImageFormat format;
  union {
    struct {
      uint32_t stride;
      void *data;
    } bitmap;
#if defined(XP_WIN)
    HANDLE sharedHandle;
#endif
  };
} NPAsyncSurface;


#define kNPEventNotHandled 0
#define kNPEventHandled 1

#define kNPEventStartIME 2

#if defined(XP_UNIX)









enum {
  NP_SETWINDOW = 1,
  NP_PRINT
};

typedef struct
{
  int32_t type;
} NPAnyCallbackStruct;

typedef struct
{
  int32_t      type;
#if defined(MOZ_X11)
  Display*     display;
  Visual*      visual;
  Colormap     colormap;
  unsigned int depth;
#endif
} NPSetWindowCallbackStruct;

typedef struct
{
  int32_t type;
  FILE* fp;
} NPPrintCallbackStruct;

#endif 

typedef enum {
#if defined(XP_MACOSX)
#ifndef NP_NO_QUICKDRAW
  NPDrawingModelQuickDraw = 0,
#endif
  NPDrawingModelCoreGraphics = 1,
  NPDrawingModelOpenGL = 2,
  NPDrawingModelCoreAnimation = 3,
  NPDrawingModelInvalidatingCoreAnimation = 4,
#endif
#if defined(XP_WIN)
  NPDrawingModelSyncWin = 5,
#endif
#if defined(MOZ_X11)
  NPDrawingModelSyncX = 6,
#endif
  NPDrawingModelAsyncBitmapSurface = 7
#if defined(XP_WIN)
  , NPDrawingModelAsyncWindowsDXGISurface = 8
#endif
} NPDrawingModel;

#ifdef XP_MACOSX
typedef enum {
#ifndef NP_NO_CARBON
  NPEventModelCarbon = 0,
#endif
  NPEventModelCocoa = 1
} NPEventModel;
#endif














#define NP_ABI_GCC3_MASK  0x10000000




#if (defined(XP_UNIX) && defined(__GNUC__) && (__GNUC__ >= 3))
#define _NP_ABI_MIXIN_FOR_GCC3 NP_ABI_GCC3_MASK
#else
#define _NP_ABI_MIXIN_FOR_GCC3 0
#endif

#if defined(XP_MACOSX)
#define NP_ABI_MACHO_MASK 0x01000000
#define _NP_ABI_MIXIN_FOR_MACHO NP_ABI_MACHO_MASK
#else
#define _NP_ABI_MIXIN_FOR_MACHO 0
#endif

#define NP_ABI_MASK (_NP_ABI_MIXIN_FOR_GCC3 | _NP_ABI_MIXIN_FOR_MACHO)




typedef enum {
  NPPVpluginNameString = 1,
  NPPVpluginDescriptionString,
  NPPVpluginWindowBool,
  NPPVpluginTransparentBool,
  NPPVjavaClass,
  NPPVpluginWindowSize,
  NPPVpluginTimerInterval,
  NPPVpluginScriptableInstance = (10 | NP_ABI_MASK),
  NPPVpluginScriptableIID = 11,
  NPPVjavascriptPushCallerBool = 12,
  NPPVpluginKeepLibraryInMemory = 13,
  NPPVpluginNeedsXEmbed         = 14,

  

  NPPVpluginScriptableNPObject  = 15,

  




  NPPVformValue = 16,

  NPPVpluginUrlRequestsDisplayedBool = 17,

  


  NPPVpluginWantsAllNetworkStreams = 18,

  
  NPPVpluginNativeAccessibleAtkPlugId = 19,

  
  NPPVpluginCancelSrcStream = 20,

  NPPVsupportsAdvancedKeyHandling = 21,

  NPPVpluginUsesDOMForCursorBool = 22,

  
  NPPVpluginDrawingModel = 1000
#if defined(XP_MACOSX)
  
  , NPPVpluginEventModel = 1001
  
  , NPPVpluginCoreAnimationLayer = 1003
#endif

} NPPVariable;




typedef enum {
  NPNVxDisplay = 1,
  NPNVxtAppContext,
  NPNVnetscapeWindow,
  NPNVjavascriptEnabledBool,
  NPNVasdEnabledBool,
  NPNVisOfflineBool,

  NPNVserviceManager = (10 | NP_ABI_MASK),
  NPNVDOMElement     = (11 | NP_ABI_MASK),
  NPNVDOMWindow      = (12 | NP_ABI_MASK),
  NPNVToolkit        = (13 | NP_ABI_MASK),
  NPNVSupportsXEmbedBool = 14,

  
  NPNVWindowNPObject = 15,

  
  NPNVPluginElementNPObject = 16,

  NPNVSupportsWindowless = 17,

  NPNVprivateModeBool = 18,

  NPNVsupportsAdvancedKeyHandling = 21,

  NPNVdocumentOrigin = 22,

  NPNVpluginDrawingModel = 1000 
#if defined(XP_MACOSX)
  , NPNVcontentsScaleFactor = 1001
#ifndef NP_NO_QUICKDRAW
  , NPNVsupportsQuickDrawBool = 2000
#endif
  , NPNVsupportsCoreGraphicsBool = 2001
  , NPNVsupportsOpenGLBool = 2002
  , NPNVsupportsCoreAnimationBool = 2003
  , NPNVsupportsInvalidatingCoreAnimationBool = 2004
#endif
  , NPNVsupportsAsyncBitmapSurfaceBool = 2007
#if defined(XP_WIN)
  , NPNVsupportsAsyncWindowsDXGISurfaceBool = 2008
#endif
#if defined(XP_MACOSX)
#ifndef NP_NO_CARBON
  , NPNVsupportsCarbonBool = 3000 
#endif
  , NPNVsupportsCocoaBool = 3001 
  , NPNVsupportsUpdatedCocoaTextInputBool = 3002 

  , NPNVsupportsCompositingCoreAnimationPluginsBool = 74656 

#endif
} NPNVariable;

typedef enum {
  NPNURLVCookie = 501,
  NPNURLVProxy
} NPNURLVariable;




typedef enum {
  NPNVGtk12 = 1,
  NPNVGtk2
} NPNToolkitType;





typedef enum {
  NPWindowTypeWindow = 1,
  NPWindowTypeDrawable
} NPWindowType;

typedef struct _NPWindow
{
  void* window;  
                 
                 
  int32_t  x;      
  int32_t  y;      
  uint32_t width;  
  uint32_t height;
  NPRect   clipRect; 
#if (defined(XP_UNIX) || defined(XP_SYMBIAN)) && !defined(XP_MACOSX)
  void * ws_info; 
#endif 
  NPWindowType type; 
} NPWindow;

typedef struct _NPImageExpose
{
  char*    data;       
  int32_t  stride;     
  int32_t  depth;      
  int32_t  x;          
  int32_t  y;          
  uint32_t width;      
  uint32_t height;     
  NPSize   dataSize;   
  float    translateX; 
  float    translateY; 
  float    scaleX;     
  float    scaleY;     
} NPImageExpose;

typedef struct _NPFullPrint
{
  NPBool pluginPrinted;
  NPBool printOne;     

  void* platformPrint; 
} NPFullPrint;

typedef struct _NPEmbedPrint
{
  NPWindow window;
  void* platformPrint; 
} NPEmbedPrint;

typedef struct _NPPrint
{
  uint16_t mode;               
  union
  {
    NPFullPrint fullPrint;   
    NPEmbedPrint embedPrint; 
  } print;
} NPPrint;

#if defined(XP_MACOSX)
#ifndef NP_NO_CARBON
typedef EventRecord NPEvent;
#endif
#elif defined(XP_SYMBIAN)
typedef QEvent NPEvent;
#elif defined(XP_WIN)
typedef struct _NPEvent
{
  uint16_t event;
  uintptr_t wParam;
  uintptr_t lParam;
} NPEvent;
#elif defined(XP_UNIX) && defined(MOZ_X11)
typedef XEvent NPEvent;
#else
typedef void*  NPEvent;
#endif

#if defined(XP_MACOSX)
typedef void* NPRegion;
#ifndef NP_NO_QUICKDRAW
typedef RgnHandle NPQDRegion;
#endif
typedef CGPathRef NPCGRegion;
#elif defined(XP_WIN)
typedef HRGN NPRegion;
#elif defined(XP_UNIX) && defined(MOZ_X11)
typedef Region NPRegion;
#elif defined(XP_SYMBIAN)
typedef QRegion* NPRegion;
#else
typedef void *NPRegion;
#endif

typedef struct _NPNSString NPNSString;
typedef struct _NPNSWindow NPNSWindow;
typedef struct _NPNSMenu   NPNSMenu;

#if defined(XP_MACOSX)
typedef NPNSMenu NPMenu;
#else
typedef void *NPMenu;
#endif

typedef enum {
  NPCoordinateSpacePlugin = 1,
  NPCoordinateSpaceWindow,
  NPCoordinateSpaceFlippedWindow,
  NPCoordinateSpaceScreen,
  NPCoordinateSpaceFlippedScreen
} NPCoordinateSpace;

#if defined(XP_MACOSX)

#ifndef NP_NO_QUICKDRAW
typedef struct NP_Port
{
  CGrafPtr port;
  int32_t portx; 
  int32_t porty;
} NP_Port;
#endif 






typedef struct NP_CGContext
{
  CGContextRef context;
  void *window; 
} NP_CGContext;






typedef struct NP_GLContext
{
  CGLContextObj context;
#ifdef NP_NO_CARBON
  NPNSWindow *window;
#else
  void *window; 
#endif
} NP_GLContext;

typedef enum {
  NPCocoaEventDrawRect = 1,
  NPCocoaEventMouseDown,
  NPCocoaEventMouseUp,
  NPCocoaEventMouseMoved,
  NPCocoaEventMouseEntered,
  NPCocoaEventMouseExited,
  NPCocoaEventMouseDragged,
  NPCocoaEventKeyDown,
  NPCocoaEventKeyUp,
  NPCocoaEventFlagsChanged,
  NPCocoaEventFocusChanged,
  NPCocoaEventWindowFocusChanged,
  NPCocoaEventScrollWheel,
  NPCocoaEventTextInput
} NPCocoaEventType;

typedef struct _NPCocoaEvent {
  NPCocoaEventType type;
  uint32_t version;
  union {
    struct {
      uint32_t modifierFlags;
      double   pluginX;
      double   pluginY;
      int32_t  buttonNumber;
      int32_t  clickCount;
      double   deltaX;
      double   deltaY;
      double   deltaZ;
    } mouse;
    struct {
      uint32_t    modifierFlags;
      NPNSString *characters;
      NPNSString *charactersIgnoringModifiers;
      NPBool      isARepeat;
      uint16_t    keyCode;
    } key;
    struct {
      CGContextRef context;
      double x;
      double y;
      double width;
      double height;
    } draw;
    struct {
      NPBool hasFocus;
    } focus;
    struct {
      NPNSString *text;
    } text;
  } data;
} NPCocoaEvent;

#ifndef NP_NO_CARBON

enum NPEventType {
  NPEventType_GetFocusEvent = (osEvt + 16),
  NPEventType_LoseFocusEvent,
  NPEventType_AdjustCursorEvent,
  NPEventType_MenuCommandEvent,
  NPEventType_ClippingChangedEvent,
  NPEventType_ScrollingBeginsEvent = 1000,
  NPEventType_ScrollingEndsEvent
};
#endif 

#endif 




#define NP_EMBED 1
#define NP_FULL  2




#define NP_NORMAL     1
#define NP_SEEK       2
#define NP_ASFILE     3
#define NP_ASFILEONLY 4

#define NP_MAXREADY (((unsigned)(~0)<<1)>>1)




#define NP_CLEAR_ALL   0
#define NP_CLEAR_CACHE (1 << 0)

#if !defined(__LP64__)
#if defined(XP_MACOSX)
#pragma options align=reset
#endif
#endif 








#define NPERR_BASE                         0
#define NPERR_NO_ERROR                    (NPERR_BASE + 0)
#define NPERR_GENERIC_ERROR               (NPERR_BASE + 1)
#define NPERR_INVALID_INSTANCE_ERROR      (NPERR_BASE + 2)
#define NPERR_INVALID_FUNCTABLE_ERROR     (NPERR_BASE + 3)
#define NPERR_MODULE_LOAD_FAILED_ERROR    (NPERR_BASE + 4)
#define NPERR_OUT_OF_MEMORY_ERROR         (NPERR_BASE + 5)
#define NPERR_INVALID_PLUGIN_ERROR        (NPERR_BASE + 6)
#define NPERR_INVALID_PLUGIN_DIR_ERROR    (NPERR_BASE + 7)
#define NPERR_INCOMPATIBLE_VERSION_ERROR  (NPERR_BASE + 8)
#define NPERR_INVALID_PARAM               (NPERR_BASE + 9)
#define NPERR_INVALID_URL                 (NPERR_BASE + 10)
#define NPERR_FILE_NOT_FOUND              (NPERR_BASE + 11)
#define NPERR_NO_DATA                     (NPERR_BASE + 12)
#define NPERR_STREAM_NOT_SEEKABLE         (NPERR_BASE + 13)
#define NPERR_TIME_RANGE_NOT_SUPPORTED    (NPERR_BASE + 14)
#define NPERR_MALFORMED_SITE              (NPERR_BASE + 15)




#define NPRES_BASE          0
#define NPRES_DONE         (NPRES_BASE + 0)
#define NPRES_NETWORK_ERR  (NPRES_BASE + 1)
#define NPRES_USER_BREAK   (NPRES_BASE + 2)




#define NP_NOERR  NP_NOERR_is_obsolete_use_NPERR_NO_ERROR
#define NP_EINVAL NP_EINVAL_is_obsolete_use_NPERR_GENERIC_ERROR
#define NP_EABORT NP_EABORT_is_obsolete_use_NPRES_USER_BREAK




#define NPVERS_HAS_STREAMOUTPUT             8
#define NPVERS_HAS_NOTIFICATION             9
#define NPVERS_HAS_LIVECONNECT              9
#define NPVERS_68K_HAS_LIVECONNECT          11
#define NPVERS_HAS_WINDOWLESS               11
#define NPVERS_HAS_XPCONNECT_SCRIPTING      13
#define NPVERS_HAS_NPRUNTIME_SCRIPTING      14
#define NPVERS_HAS_FORM_VALUES              15
#define NPVERS_HAS_POPUPS_ENABLED_STATE     16
#define NPVERS_HAS_RESPONSE_HEADERS         17
#define NPVERS_HAS_NPOBJECT_ENUM            18
#define NPVERS_HAS_PLUGIN_THREAD_ASYNC_CALL 19
#define NPVERS_HAS_ALL_NETWORK_STREAMS      20
#define NPVERS_HAS_URL_AND_AUTH_INFO        21
#define NPVERS_HAS_PRIVATE_MODE             22
#define NPVERS_MACOSX_HAS_COCOA_EVENTS      23
#define NPVERS_HAS_ADVANCED_KEY_HANDLING    25
#define NPVERS_HAS_URL_REDIRECT_HANDLING    26
#define NPVERS_HAS_CLEAR_SITE_DATA          27





#ifdef __cplusplus
extern "C" {
#endif



#if defined(XP_UNIX)
const char* NPP_GetMIMEDescription(void);
#endif

NPError NPP_New(NPMIMEType pluginType, NPP instance,
                uint16_t mode, int16_t argc, char* argn[],
                char* argv[], NPSavedData* saved);
NPError NPP_Destroy(NPP instance, NPSavedData** save);
NPError NPP_SetWindow(NPP instance, NPWindow* window);
NPError NPP_NewStream(NPP instance, NPMIMEType type,
                      NPStream* stream, NPBool seekable,
                      uint16_t* stype);
NPError NPP_DestroyStream(NPP instance, NPStream* stream,
                          NPReason reason);
int32_t NPP_WriteReady(NPP instance, NPStream* stream);
int32_t NPP_Write(NPP instance, NPStream* stream, int32_t offset,
                  int32_t len, void* buffer);
void    NPP_StreamAsFile(NPP instance, NPStream* stream,
                         const char* fname);
void    NPP_Print(NPP instance, NPPrint* platformPrint);
int16_t NPP_HandleEvent(NPP instance, void* event);
void    NPP_URLNotify(NPP instance, const char* url,
                      NPReason reason, void* notifyData);
NPError NPP_GetValue(NPP instance, NPPVariable variable, void *value);
NPError NPP_SetValue(NPP instance, NPNVariable variable, void *value);
NPBool  NPP_GotFocus(NPP instance, NPFocusDirection direction);
void    NPP_LostFocus(NPP instance);
void    NPP_URLRedirectNotify(NPP instance, const char* url, int32_t status, void* notifyData);
NPError NPP_ClearSiteData(const char* site, uint64_t flags, uint64_t maxAge);
char**  NPP_GetSitesWithData(void);
void    NPP_DidComposite(NPP instance);


void        NPN_Version(int* plugin_major, int* plugin_minor,
                        int* netscape_major, int* netscape_minor);
NPError     NPN_GetURLNotify(NPP instance, const char* url,
                             const char* target, void* notifyData);
NPError     NPN_GetURL(NPP instance, const char* url,
                       const char* target);
NPError     NPN_PostURLNotify(NPP instance, const char* url,
                              const char* target, uint32_t len,
                              const char* buf, NPBool file,
                              void* notifyData);
NPError     NPN_PostURL(NPP instance, const char* url,
                        const char* target, uint32_t len,
                        const char* buf, NPBool file);
NPError     NPN_RequestRead(NPStream* stream, NPByteRange* rangeList);
NPError     NPN_NewStream(NPP instance, NPMIMEType type,
                          const char* target, NPStream** stream);
int32_t     NPN_Write(NPP instance, NPStream* stream, int32_t len,
                      void* buffer);
NPError     NPN_DestroyStream(NPP instance, NPStream* stream,
                              NPReason reason);
void        NPN_Status(NPP instance, const char* message);
const char* NPN_UserAgent(NPP instance);
void*       NPN_MemAlloc(uint32_t size);
void        NPN_MemFree(void* ptr);
uint32_t    NPN_MemFlush(uint32_t size);
void        NPN_ReloadPlugins(NPBool reloadPages);
NPError     NPN_GetValue(NPP instance, NPNVariable variable,
                         void *value);
NPError     NPN_SetValue(NPP instance, NPPVariable variable,
                         void *value);
void        NPN_InvalidateRect(NPP instance, NPRect *invalidRect);
void        NPN_InvalidateRegion(NPP instance,
                                 NPRegion invalidRegion);
void        NPN_ForceRedraw(NPP instance);
void        NPN_PushPopupsEnabledState(NPP instance, NPBool enabled);
void        NPN_PopPopupsEnabledState(NPP instance);
void        NPN_PluginThreadAsyncCall(NPP instance,
                                      void (*func) (void *),
                                      void *userData);
NPError     NPN_GetValueForURL(NPP instance, NPNURLVariable variable,
                               const char *url, char **value,
                               uint32_t *len);
NPError     NPN_SetValueForURL(NPP instance, NPNURLVariable variable,
                               const char *url, const char *value,
                               uint32_t len);
NPError     NPN_GetAuthenticationInfo(NPP instance,
                                      const char *protocol,
                                      const char *host, int32_t port,
                                      const char *scheme,
                                      const char *realm,
                                      char **username, uint32_t *ulen,
                                      char **password,
                                      uint32_t *plen);
uint32_t    NPN_ScheduleTimer(NPP instance, uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID));
void        NPN_UnscheduleTimer(NPP instance, uint32_t timerID);
NPError     NPN_PopUpContextMenu(NPP instance, NPMenu* menu);
NPBool      NPN_ConvertPoint(NPP instance, double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double *destX, double *destY, NPCoordinateSpace destSpace);
NPBool      NPN_HandleEvent(NPP instance, void *event, NPBool handled);
NPBool      NPN_UnfocusInstance(NPP instance, NPFocusDirection direction);
void        NPN_URLRedirectResponse(NPP instance, void* notifyData, NPBool allow);
NPError     NPN_InitAsyncSurface(NPP instance, NPSize *size,
                                 NPImageFormat format, void *initData,
                                 NPAsyncSurface *surface);
NPError     NPN_FinalizeAsyncSurface(NPP instance, NPAsyncSurface *surface);
void        NPN_SetCurrentAsyncSurface(NPP instance, NPAsyncSurface *surface, NPRect *changed);

#ifdef __cplusplus
}  
#endif

#endif

#endif
