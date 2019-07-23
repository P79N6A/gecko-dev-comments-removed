




































#ifndef nsplugindefs_h___
#define nsplugindefs_h___

#if defined(XP_OS2) || defined(__OS2__)
#define INCL_BASE
#define INCL_PM
#include <os2.h>
#pragma pack(1)
#endif

#ifndef prtypes_h___
#include "prtypes.h"
#endif

#ifdef XP_MACOSX
#   include <Quickdraw.h>
#   include <Events.h>
#   include <MacWindows.h>
#endif

#if defined(XP_UNIX) && defined(MOZ_X11)
#   include <X11/Xlib.h>
#   include <X11/Xutil.h>
#endif

#if defined(XP_WIN)
#   include <windef.h>
#endif
































#define NS_INFO_ProductVersion      1
#define NS_INFO_MIMEType            2
#define NS_INFO_FileOpenName        3
#define NS_INFO_FileExtents         4


#define NS_INFO_FileDescription     5
#define NS_INFO_ProductName         6


#define NS_INFO_CompanyName         7
#define NS_INFO_FileVersion         8
#define NS_INFO_InternalName        9
#define NS_INFO_LegalCopyright      10
#define NS_INFO_OriginalFilename    11

#ifndef RC_INVOKED




typedef const char*     nsMIMEType;

struct nsByteRange {
    PRInt32             offset; 	
    PRUint32            length;
    struct nsByteRange* next;
};

struct nsPluginRect {
    PRUint16            top;
    PRUint16            left;
    PRUint16            bottom;
    PRUint16            right;
};




#ifdef XP_UNIX

#include <stdio.h>






enum nsPluginCallbackType {
    nsPluginCallbackType_SetWindow = 1,
    nsPluginCallbackType_Print
};

struct nsPluginAnyCallbackStruct {
    PRInt32     type;
};

#ifdef MOZ_X11
struct nsPluginSetWindowCallbackStruct {
    PRInt32     type;
    Display*    display;
    Visual*     visual;
    Colormap    colormap;
    PRUint32    depth;
};
#else
struct nsPluginSetWindowCallbackStruct {
    PRInt32     type;
};
#endif


struct nsPluginPrintCallbackStruct {
    PRInt32     type;
    FILE*       fp;
};

#endif 




enum nsPluginVariable {
    nsPluginVariable_NameString                      = 1,
    nsPluginVariable_DescriptionString               = 2
};

enum nsPluginManagerVariable {
    nsPluginManagerVariable_XDisplay                 = 1,
    nsPluginManagerVariable_XtAppContext             = 2,
    nsPluginManagerVariable_SupportsXEmbed            = 14
};

enum nsPluginInstancePeerVariable {
    nsPluginInstancePeerVariable_NetscapeWindow      = 3


};

enum nsPluginInstanceVariable {
    nsPluginInstanceVariable_WindowlessBool          = 3,
    nsPluginInstanceVariable_TransparentBool         = 4,
    nsPluginInstanceVariable_DoCacheBool             = 5,
    nsPluginInstanceVariable_CallSetWindowAfterDestroyBool = 6,
    nsPluginInstanceVariable_ScriptableInstance      = 10,
    nsPluginInstanceVariable_ScriptableIID           = 11,
    nsPluginInstanceVariable_NeedsXEmbed             = 14
};



enum nsPluginMode {
    nsPluginMode_Embedded = 1,
    nsPluginMode_Full
};


enum nsPluginStreamType {
    nsPluginStreamType_Normal = 1,
    nsPluginStreamType_Seek,
    nsPluginStreamType_AsFile,
    nsPluginStreamType_AsFileOnly
};





enum nsPluginWindowType {
    nsPluginWindowType_Window = 1,
    nsPluginWindowType_Drawable
};

#ifdef XP_MACOSX

struct nsPluginPort {
    CGrafPtr     port;   
    PRInt32     portx;  
    PRInt32     porty;
};
typedef RgnHandle       nsPluginRegion;
typedef WindowRef       nsPluginPlatformWindowRef;

#elif defined(XP_WIN) || defined(XP_OS2)

struct nsPluginPort;
typedef HRGN            nsPluginRegion;
typedef HWND            nsPluginPlatformWindowRef;

#elif defined(XP_UNIX) && defined(MOZ_X11)

struct nsPluginPort;
typedef Region          nsPluginRegion;
typedef Drawable        nsPluginPlatformWindowRef;

#else

struct nsPluginPort;
typedef void*           nsPluginRegion;
typedef void*           nsPluginPlatformWindowRef;

#endif

struct nsPluginWindow {
    nsPluginPort* window;       
                                
                                
    PRInt32       x;            
    PRInt32       y;            
    PRUint32      width;        
    PRUint32      height;
    nsPluginRect  clipRect;     
                                
#if defined(XP_UNIX) && !defined(XP_MACOSX)
    void*         ws_info;      
#endif 
    nsPluginWindowType type;    
};

struct nsPluginFullPrint {
    PRBool      pluginPrinted;	
                                
    PRBool      printOne;       
                                
    void*       platformPrint;  
};

struct nsPluginEmbedPrint {
    nsPluginWindow    window;
    void*             platformPrint;	
};

struct nsPluginPrint {
    PRUint16                  mode;         
    union
    {
        nsPluginFullPrint     fullPrint;	
        nsPluginEmbedPrint    embedPrint;	
    } print;
};

struct nsPluginEvent {

#ifdef XP_MACOSX
    EventRecord*                event;
    nsPluginPlatformWindowRef   window;

#elif defined(XP_OS2)
    uint32      event;
    uint32      wParam;
    uint32      lParam;

#elif defined(XP_WIN)
    uint16      event;
    uint32      wParam;
    uint32      lParam;

#elif defined(XP_UNIX) && defined(MOZ_X11)
    XEvent      event;
#else
    void        *event;
#endif
};





enum nsPluginEventType {
#ifdef XP_MACOSX
    nsPluginEventType_GetFocusEvent = (osEvt + 16),
    nsPluginEventType_LoseFocusEvent,
    nsPluginEventType_AdjustCursorEvent,
    nsPluginEventType_MenuCommandEvent,
    nsPluginEventType_ClippingChangedEvent,
    nsPluginEventType_ScrollingBeginsEvent,
    nsPluginEventType_ScrollingEndsEvent,
#endif 
    nsPluginEventType_Idle                 = 0
};



enum nsPluginReason {
    nsPluginReason_Base = 0,
    nsPluginReason_Done = 0,
    nsPluginReason_NetworkErr,
    nsPluginReason_UserBreak,
    nsPluginReason_NoReason
};









#define nsMajorVersion(v)       (((PRInt32)(v) >> 16) & 0xffff)
#define nsMinorVersion(v)       ((PRInt32)(v) & 0xffff)

#define nsVersionOK(suppliedV, requiredV)                   \
    (nsMajorVersion(suppliedV) == nsMajorVersion(requiredV) \
     && nsMinorVersion(suppliedV) >= nsMinorVersion(requiredV))

#define NP_POPUP_API_VERSION 16






class nsIPlugin;                        
class nsIEventHandler;                  
class nsIPluginInstance;                


class nsIPluginManager;                 
class nsIFileUtilities;                 
class nsIPluginInstancePeer;            
class nsIWindowlessPluginInstancePeer;  
class nsIPluginTagInfo;                 


#endif 
#ifdef __OS2__
#pragma pack()
#endif

#endif 
