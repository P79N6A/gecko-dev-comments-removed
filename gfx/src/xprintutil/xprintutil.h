
#ifndef XPRINTUTIL_H
#define XPRINTUTIL_H 1





































#ifndef FUNCPROTO 
#define FUNCPROTO 15
#endif 

#include <X11/Xlibint.h>
#include <X11/extensions/Print.h>
#include <X11/Intrinsic.h>


#ifdef USE_MOZILLA_TYPES
#include <prtypes.h>
#include <prmem.h>
#include <prthread.h>
#define XPU_USE_NSPR 1





#endif 

#ifdef DEBUG

#define XPU_TRACE(EX)  (puts(#EX),EX)

#define XPU_TRACE_CHILD(EX)  (puts("child: " #EX),EX)

#define XPU_DEBUG_ONLY(EX)  (EX)
#else
#define XPU_TRACE(EX) (EX)
#define XPU_TRACE_CHILD(EX) (EX)
#define XPU_DEBUG_ONLY(EX)
#endif 


#define XPU_NULLXSTR(s) (((s)!=NULL)?(s):("<NULL>"))






typedef struct {
  const char *tray_name;
  const char *medium_name;
  int         mbool;
  float       ma1; 
  float       ma2; 
  float       ma3; 
  float       ma4;
} XpuMediumSourceSizeRec, *XpuMediumSourceSizeList;






typedef struct {
  const char *name;
  long        x_dpi;
  long        y_dpi;
} XpuResolutionRec, *XpuResolutionList;






typedef struct {
  const char *orientation;
} XpuOrientationRec, *XpuOrientationList;





typedef struct {
  const char *plex;
} XpuPlexRec, *XpuPlexList;




typedef struct
{
  const char  *name;
  XVisualInfo  visualinfo;
} XpuColorspaceRec, *XpuColorspaceList;




typedef long XpuSupportedFlags;

#define XPUATTRIBUTESUPPORTED_JOB_NAME                     (1L<<0)
#define XPUATTRIBUTESUPPORTED_JOB_OWNER                    (1L<<1)
#define XPUATTRIBUTESUPPORTED_NOTIFICATION_PROFILE         (1L<<2)

#define XPUATTRIBUTESUPPORTED_COPY_COUNT                   (1L<<3)
#define XPUATTRIBUTESUPPORTED_DOCUMENT_FORMAT              (1L<<4)
#define XPUATTRIBUTESUPPORTED_CONTENT_ORIENTATION          (1L<<5)
#define XPUATTRIBUTESUPPORTED_DEFAULT_PRINTER_RESOLUTION   (1L<<6)
#define XPUATTRIBUTESUPPORTED_DEFAULT_INPUT_TRAY           (1L<<7)
#define XPUATTRIBUTESUPPORTED_DEFAULT_MEDIUM               (1L<<8)
#define XPUATTRIBUTESUPPORTED_PLEX                         (1L<<9)
#define XPUATTRIBUTESUPPORTED_LISTFONTS_MODES              (1L<<10)


_XFUNCPROTOBEGIN

int XpuCheckExtension( Display *pdpy );


Bool XpuXprintServersAvailable( void );
int XpuGetPrinter( const char *printername, Display **pdpyptr, XPContext *pcontextptr );
void XpuClosePrinterDisplay(Display *pdpy, XPContext pcontext);


void XpuSetOneAttribute( Display *pdpy, XPContext pcontext, 
                         XPAttributes type, const char *attribute_name, const char *value, XPAttrReplacement replacement_rule );
void XpuSetOneLongAttribute( Display *pdpy, XPContext pcontext, 
                         XPAttributes type, const char *attribute_name, long value, XPAttrReplacement replacement_rule );
int XpuCheckSupported( Display *pdpy, XPContext pcontext, XPAttributes type, const char *attribute_name, const char *query );
int XpuSetJobTitle( Display *pdpy, XPContext pcontext, const char *title );
int XpuGetOneLongAttribute( Display *pdpy, XPContext pcontext, XPAttributes type, const char *attribute_name, long *result );
#ifdef DEBUG
void dumpXpAttributes( Display *pdpy, XPContext pcontext );
#endif 
void XpuWaitForPrintNotify( Display *pdpy, int xp_event_base, int detail );


XPPrinterList XpuGetPrinterList( const char *printer, int *res_list_count );
void XpuFreePrinterList( XPPrinterList list );


int XpuSetDocumentCopies( Display *pdpy, XPContext pcontext, long num_copies );


XpuMediumSourceSizeList XpuGetMediumSourceSizeList( Display *pdpy, XPContext pcontext, int *numEntriesPtr );
void XpuFreeMediumSourceSizeList( XpuMediumSourceSizeList list );
int XpuSetDocMediumSourceSize( Display *pdpy, XPContext pcontext, XpuMediumSourceSizeRec *medium_spec );
int XpuSetPageMediumSourceSize( Display *pdpy, XPContext pcontext, XpuMediumSourceSizeRec *medium_spec );
XpuMediumSourceSizeRec *
XpuFindMediumSourceSizeBySize( XpuMediumSourceSizeList mlist, int mlist_count,
                               float page_width_mm, float page_height_mm, float tolerance );
XpuMediumSourceSizeRec *
XpuFindMediumSourceSizeByBounds( XpuMediumSourceSizeList mlist, int mlist_count, 
                                 float m1, float m2, float m3, float m4, float tolerance );
XpuMediumSourceSizeRec *
XpuFindMediumSourceSizeByName( XpuMediumSourceSizeList mlist, int mlist_count, 
                               const char *tray_name, const char *medium_name );


XpuResolutionList XpuGetResolutionList( Display *pdpy, XPContext pcontext, int *numEntriesPtr );
void XpuFreeResolutionList( XpuResolutionList list );
Bool XpuGetResolution( Display *pdpy, XPContext pcontext, long *x_dpi, long *y_dpi );
Bool XpuSetPageResolution( Display *pdpy, XPContext pcontext, XpuResolutionRec * );
Bool XpuSetDocResolution( Display *pdpy, XPContext pcontext, XpuResolutionRec * );
XpuResolutionRec *XpuFindResolutionByName( XpuResolutionList list, int list_count, const char *resolution_name);


XpuOrientationList XpuGetOrientationList( Display *pdpy, XPContext pcontext, int *numEntriesPtr );
void XpuFreeOrientationList( XpuOrientationList list );
XpuOrientationRec *
XpuFindOrientationByName( XpuOrientationList list, int list_count, const char *orientation );
int XpuSetDocOrientation( Display *pdpy, XPContext pcontext, XpuOrientationRec *rec );
int XpuSetPageOrientation( Display *pdpy, XPContext pcontext, XpuOrientationRec *rec );


XpuPlexList XpuGetPlexList( Display *pdpy, XPContext pcontext, int *numEntriesPtr );
void XpuFreePlexList( XpuPlexList list );
XpuPlexRec *XpuFindPlexByName( XpuPlexList list, int list_count, const char *plex );
int XpuSetDocPlex( Display *pdpy, XPContext pcontext, XpuPlexRec *rec );
int XpuSetPagePlex( Display *pdpy, XPContext pcontext, XpuPlexRec *rec );


Bool XpuGetEnableFontDownload( Display *pdpy, XPContext pcontext );
int XpuSetEnableFontDownload( Display *pdpy, XPContext pcontext, Bool enableFontDownload );


XpuColorspaceList XpuGetColorspaceList( Display *pdpy, XPContext pcontext, int *numEntriesPtr );
void XpuFreeColorspaceList( XpuColorspaceList list );
XpuColorspaceRec *XpuFindColorspaceByName( XpuColorspaceList list, int list_count, const char *colorspace );


void XpuStartJobToSpooler(Display *pdpy);
void *XpuStartJobToFile( Display *pdpy, XPContext pcontext, const char *filename );
XPGetDocStatus XpuWaitForPrintFileChild( void *handle );


XpuSupportedFlags XpuGetSupportedJobAttributes(Display *pdpy, XPContext pcontext);
XpuSupportedFlags XpuGetSupportedDocAttributes(Display *pdpy, XPContext pcontext);
XpuSupportedFlags XpuGetSupportedPageAttributes(Display *pdpy, XPContext pcontext);


char *XpuResourceEncode( const char *str );
char *XpuResourceDecode( const char *str );
void XpuResourceFreeString( char *s );


const char *XpuXmbToCompoundText(Display *dpy, const char *xmbtext);
void XpuFreeCompundTextString( const char *s );
const char *XpuCompoundTextToXmb(Display *dpy, const char *ct);
void XpuFreeXmbString( const char *s );


_XFUNCPROTOEND

#define XpuGetJobAttributes( pdpy, pcontext )     XpGetAttributes( (pdpy), (pcontext), XPJobAttr )
#define XpuGetDocAttributes( pdpy, pcontext )     XpGetAttributes( (pdpy), (pcontext), XPDocAttr )
#define XpuGetPageAttributes( pdpy, pcontext )    XpGetAttributes( (pdpy), (pcontext), XPPageAttr )
#define XpuGetPrinterAttributes( pdpy, pcontext ) XpGetAttributes( (pdpy), (pcontext), XPPrinterAttr )
#define XpuGetServerAttributes( pdpy, pcontext )  XpGetAttributes( (pdpy), (pcontext), XPServerAttr )

#endif 

