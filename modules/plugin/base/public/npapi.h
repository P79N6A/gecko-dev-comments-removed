




































#ifndef npapi_h_
#define npapi_h_

#ifdef __OS2__
#pragma pack(1)
#endif

#include "nptypes.h"

#if defined (__OS2__) || defined (OS2)
# ifndef XP_OS2
#  define XP_OS2 1
# endif 
#endif 

#ifdef _WINDOWS
# include <windef.h>
# ifndef XP_WIN
#  define XP_WIN 1
# endif 
#endif 

#ifdef XP_MACOSX
#ifdef __LP64__
#define NP_NO_QUICKDRAW
#define NP_NO_CARBON
#include <ApplicationServices/ApplicationServices.h>
#else
#include <Carbon/Carbon.h>
#endif
#endif

#if defined(XP_UNIX) 
# include <stdio.h>
# if defined(MOZ_X11)
#  include <X11/Xlib.h>
#  include <X11/Xutil.h>
# endif
#endif





#define NP_VERSION_MAJOR 0
#define NP_VERSION_MINOR 23




























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
#if defined(XP_MAC) || defined(XP_MACOSX)
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

#ifdef XP_UNIX









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
#ifdef MOZ_X11
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

#ifdef XP_MACOSX
typedef enum {
#ifndef NP_NO_QUICKDRAW
  NPDrawingModelQuickDraw = 0,
#endif
  NPDrawingModelCoreGraphics = 1
} NPDrawingModel;

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

#ifdef XP_MACOSX
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
  
  


  NPPVpluginWantsAllNetworkStreams = 18

#ifdef XP_MACOSX
  
  , NPPVpluginDrawingModel = 1000
  
  , NPPVpluginEventModel = 1001
#endif

#ifdef MOZ_PLATFORM_HILDON
  , NPPVpluginWindowlessLocalBool = 2002
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

  NPNVprivateModeBool = 18

#ifdef XP_MACOSX
  
  , NPNVpluginDrawingModel = 1000
#ifndef NP_NO_QUICKDRAW
  , NPNVsupportsQuickDrawBool = 2000
#endif
  , NPNVsupportsCoreGraphicsBool = 2001
#ifndef NP_NO_CARBON
  , NPNVsupportsCarbonBool = 3000 
#endif
  , NPNVsupportsCocoaBool = 3001 
#endif
#ifdef MOZ_PLATFORM_HILDON
  , NPNVSupportsWindowlessLocal = 2002
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
#if defined(XP_UNIX) && !defined(XP_MACOSX)
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

#ifdef XP_MACOSX
#ifndef NP_NO_CARBON
typedef EventRecord NPEvent;
#endif
#elif defined(XP_WIN)
typedef struct _NPEvent
{
  uint16_t event;
  uint32_t wParam;
  uint32_t lParam;
} NPEvent;
#elif defined(XP_OS2)
typedef struct _NPEvent
{
  uint32_t event;
  uint32_t wParam;
  uint32_t lParam;
} NPEvent;
#elif defined (XP_UNIX) && defined(MOZ_X11)
typedef XEvent NPEvent;
#else
typedef void*  NPEvent;
#endif

#ifdef XP_MACOSX
typedef void* NPRegion;
#ifndef NP_NO_QUICKDRAW
typedef RgnHandle NPQDRegion;
#endif
typedef CGPathRef NPCGRegion;
#elif defined(XP_WIN)
typedef HRGN NPRegion;
#elif defined(XP_UNIX) && defined(MOZ_X11)
typedef Region NPRegion;
#else
typedef void *NPRegion;
#endif

typedef struct _NPNSString NPNSString;
typedef struct _NPNSWindow NPNSWindow;
typedef struct _NPNSMenu   NPNSMenu;

#ifdef XP_MACOSX
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

#ifdef XP_MACOSX

typedef struct NP_Port
{
  CGrafPtr port;
  int32_t portx; 
  int32_t porty;
} NP_Port;

typedef struct NP_CGContext
{
  CGContextRef context;
#ifdef NP_NO_CARBON
  NPNSWindow *window;
#else
  void *window; 
#endif
} NP_CGContext;

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
#ifdef OBSOLETE
#define getFocusEvent     (osEvt + 16)
#define loseFocusEvent    (osEvt + 17)
#define adjustCursorEvent (osEvt + 18)
#endif 
#endif 

#endif 




#define NP_EMBED 1
#define NP_FULL  2




#define NP_NORMAL     1
#define NP_SEEK       2
#define NP_ASFILE     3
#define NP_ASFILEONLY 4

#define NP_MAXREADY (((unsigned)(~0)<<1)>>1)

#if !defined(__LP64__)
#if defined(XP_MAC) || defined(XP_MACOSX)
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





#if defined(__OS2__)
#define NP_LOADDS _System
#else
#define NP_LOADDS
#endif

#ifdef __cplusplus
extern "C" {
#endif



#ifdef XP_UNIX
char* NPP_GetMIMEDescription();
#endif

NPError NP_LOADDS NPP_Initialize();
void    NP_LOADDS NPP_Shutdown();
NPError NP_LOADDS NPP_New(NPMIMEType pluginType, NPP instance,
                          uint16_t mode, int16_t argc, char* argn[],
                          char* argv[], NPSavedData* saved);
NPError NP_LOADDS NPP_Destroy(NPP instance, NPSavedData** save);
NPError NP_LOADDS NPP_SetWindow(NPP instance, NPWindow* window);
NPError NP_LOADDS NPP_NewStream(NPP instance, NPMIMEType type,
                                NPStream* stream, NPBool seekable,
                                uint16_t* stype);
NPError NP_LOADDS NPP_DestroyStream(NPP instance, NPStream* stream,
                                    NPReason reason);
int32_t NP_LOADDS NPP_WriteReady(NPP instance, NPStream* stream);
int32_t NP_LOADDS NPP_Write(NPP instance, NPStream* stream, int32_t offset,
                            int32_t len, void* buffer);
void    NP_LOADDS NPP_StreamAsFile(NPP instance, NPStream* stream,
                                   const char* fname);
void    NP_LOADDS NPP_Print(NPP instance, NPPrint* platformPrint);
int16_t NP_LOADDS NPP_HandleEvent(NPP instance, void* event);
void    NP_LOADDS NPP_URLNotify(NPP instance, const char* url,
                                NPReason reason, void* notifyData);
NPError NP_LOADDS NPP_GetValue(NPP instance, NPPVariable variable, void *value);
NPError NP_LOADDS NPP_SetValue(NPP instance, NPNVariable variable, void *value);


void        NP_LOADDS NPN_Version(int* plugin_major, int* plugin_minor,
                                  int* netscape_major, int* netscape_minor);
NPError     NP_LOADDS NPN_GetURLNotify(NPP instance, const char* url,
                                       const char* target, void* notifyData);
NPError     NP_LOADDS NPN_GetURL(NPP instance, const char* url,
                                 const char* target);
NPError     NP_LOADDS NPN_PostURLNotify(NPP instance, const char* url,
                                        const char* target, uint32_t len,
                                        const char* buf, NPBool file,
                                        void* notifyData);
NPError     NP_LOADDS NPN_PostURL(NPP instance, const char* url,
                                  const char* target, uint32_t len,
                                  const char* buf, NPBool file);
NPError     NP_LOADDS NPN_RequestRead(NPStream* stream, NPByteRange* rangeList);
NPError     NP_LOADDS NPN_NewStream(NPP instance, NPMIMEType type,
                                const char* target, NPStream** stream);
int32_t     NP_LOADDS NPN_Write(NPP instance, NPStream* stream, int32_t len,
                                void* buffer);
NPError     NP_LOADDS NPN_DestroyStream(NPP instance, NPStream* stream,
                                        NPReason reason);
void        NP_LOADDS NPN_Status(NPP instance, const char* message);
const char* NP_LOADDS NPN_UserAgent(NPP instance);
void*       NP_LOADDS NPN_MemAlloc(uint32_t size);
void        NP_LOADDS NPN_MemFree(void* ptr);
uint32_t    NP_LOADDS NPN_MemFlush(uint32_t size);
void        NP_LOADDS NPN_ReloadPlugins(NPBool reloadPages);
NPError     NP_LOADDS NPN_GetValue(NPP instance, NPNVariable variable,
                                   void *value);
NPError     NP_LOADDS NPN_SetValue(NPP instance, NPPVariable variable,
                                   void *value);
void        NP_LOADDS NPN_InvalidateRect(NPP instance, NPRect *invalidRect);
void        NP_LOADDS NPN_InvalidateRegion(NPP instance,
                                           NPRegion invalidRegion);
void        NP_LOADDS NPN_ForceRedraw(NPP instance);
void        NP_LOADDS NPN_PushPopupsEnabledState(NPP instance, NPBool enabled);
void        NP_LOADDS NPN_PopPopupsEnabledState(NPP instance);
void        NP_LOADDS NPN_PluginThreadAsyncCall(NPP instance,
                                                void (*func) (void *),
                                                void *userData);
NPError     NP_LOADDS NPN_GetValueForURL(NPP instance, NPNURLVariable variable,
                                         const char *url, char **value,
                                         uint32_t *len);
NPError     NP_LOADDS NPN_SetValueForURL(NPP instance, NPNURLVariable variable,
                                         const char *url, const char *value,
                                         uint32_t len);
NPError     NP_LOADDS NPN_GetAuthenticationInfo(NPP instance,
                                                const char *protocol,
                                                const char *host, int32_t port,
                                                const char *scheme,
                                                const char *realm,
                                                char **username, uint32_t *ulen,
                                                char **password,
                                                uint32_t *plen);
uint32_t    NP_LOADDS NPN_ScheduleTimer(NPP instance, uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID));
void        NP_LOADDS NPN_UnscheduleTimer(NPP instance, uint32_t timerID);
NPError     NP_LOADDS NPN_PopUpContextMenu(NPP instance, NPMenu* menu);
NPBool      NP_LOADDS NPN_ConvertPoint(NPP instance, double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double *destX, double *destY, NPCoordinateSpace destSpace);

#ifdef __cplusplus
}  
#endif

#endif
#ifdef __OS2__
#pragma pack()
#endif

#endif
