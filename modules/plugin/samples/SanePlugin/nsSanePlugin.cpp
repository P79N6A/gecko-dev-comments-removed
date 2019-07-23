








































#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <gtk/gtk.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "nsIIOService.h"
#include "nsSanePlugin.h"
#include "plstr.h"
#include "prmem.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"

extern "C"
{
#include "jpeglib.h"
#include "sane/sane.h"
#include "sane/sanei.h"
#include "sane/saneopts.h"
}

#define MAX_OPT_SIZE (1024 * 2)

struct my_JPEG_error_mgr
{
    struct jpeg_error_mgr pub;
    sigjmp_buf setjmp_buffer;
};
typedef struct my_JPEG_error_mgr *emptr;

static void PR_CALLBACK ThreadedDestroyEvent(PLEvent* aEvent);
static void* PR_CALLBACK ThreadedHandleEvent(PLEvent* aEvent);



static void store_filename(GtkFileSelection *selector, gpointer user_data);
static nsresult optioncat( char ** dest, const char * src, 
                           const char * ending );
static void my_jpeg_error_exit (j_common_ptr cinfo);
static unsigned char* jpeg_file_to_rgb (FILE * f, int *w, int *h);
static gboolean draw (GtkWidget *widget, GdkEventExpose *event, gpointer data);
static unsigned char * crop_image(unsigned char *rgb_data, int *rgb_width, 
                                  int *rgb_height,	gint x, gint y, 
                                  gint w, gint h);
static unsigned char * scale_image(unsigned char *rgb_data, int rgb_width, 
                                   int rgb_height, int w, int h);

static PRInt32 gPluginObjectCount = 0;

static NS_DEFINE_CID(kCPluginManagerCID,        NS_PLUGINMANAGER_CID         );
static NS_DEFINE_CID(kEventQueueService,        NS_EVENTQUEUESERVICE_CID     );





nsSanePluginInstance::nsSanePluginInstance( void )
    : fPeer( NULL ), fWindow( NULL ), fMode( nsPluginMode_Embedded )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::nsSanePluginInstance()\n");
#endif

    PR_AtomicIncrement(&gPluginObjectCount);

    
    mCompQuality = 10;
    mCompMethod = JDCT_FASTEST;

    
    mSaneDevice = nsnull;
    mSaneOpen = SANE_FALSE;
    mRGBData = nsnull;
    mBottomRightXChange = mBottomRightYChange = 
        mTopLeftXChange = mTopLeftYChange = 0;

    
    mOnScanCompleteScript = nsnull;
    mOnInitCompleteScript = nsnull;
    mScanThread = nsnull;
    mUIThread = PR_GetCurrentThread();

    mState = 0; 
    mSuccess = 1; 
    mFileSelection = nsnull;

    
    fPlatform.widget = nsnull;
    mEvent_box = nsnull;
    mDrawing_area = nsnull;

    if (NS_FAILED(CallCreateInstance(kCPluginManagerCID, &mPluginManager))) {
        NS_ERROR("Error trying to create plugin manager!");
        return;
    }
}

nsSanePluginInstance::~nsSanePluginInstance(void)
{
#ifdef DEBUG
    printf("nsSanePluginInstance::~nsSanePluginInstance(%i)\n", 
           gPluginObjectCount );
#endif 

    PR_AtomicDecrement(&gPluginObjectCount);
    PlatformDestroy(); 

    if (mPluginManager) {
        mPluginManager->Release();
        mPluginManager = nsnull;
    }

    sane_exit();

    
    if (fPlatform.widget)
        gtk_widget_destroy(fPlatform.widget);
    if (mEvent_box)
        gtk_widget_destroy(mEvent_box);
    if (mDrawing_area)
        gtk_widget_destroy(mDrawing_area);

    NS_IF_RELEASE(fPeer); 
    PR_FREEIF(mSaneDevice);
    PR_FREEIF(mOnScanCompleteScript);
    PR_FREEIF(mOnInitCompleteScript);

}



NS_IMPL_ISUPPORTS2(nsSanePluginInstance, 
                   nsIPluginInstance, 
                   nsISanePluginInstance)

NS_METHOD
nsSanePluginInstance::Initialize( nsIPluginInstancePeer* peer )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::Initialize()\n");
#endif

    NS_ASSERTION(peer != NULL, "null peer");

    nsIPluginTagInfo * taginfo;
    const char * const * names = nsnull;
    const char * const * values = nsnull;
    PRUint16 count = 0;
    nsresult result;

    gdk_rgb_init();

    fPeer = peer;
    peer->AddRef();
    result = peer->GetMode( &fMode );

    if ( NS_FAILED( result ) )
        return result;

    result = peer->QueryInterface( NS_GET_IID(nsIPluginTagInfo),
                                   ( void ** )&taginfo );

    if ( NS_SUCCEEDED( result ) )
    {
        taginfo->GetAttributes( count, names, values );
        NS_IF_RELEASE( taginfo );
    }

    
    mLineWidth = 5;
    mLineStyle = GDK_LINE_ON_OFF_DASH;
    mCapStyle = GDK_CAP_BUTT;
    mJoinStyle = GDK_JOIN_MITER;

    
    for (int i=0; i < count; i++) {

        if (PL_strcasecmp(names[i], "line_width") == 0) {
            
            
            sscanf(values[i], "%i", &mLineWidth);

        } else if (PL_strcasecmp(names[i], "line_style") == 0) {
            
            
            if (PL_strcasecmp(values[i], "dash") == 0) 
                mLineStyle = GDK_LINE_ON_OFF_DASH;
            else if (PL_strcasecmp(values[i], "solid") == 0)
                mLineStyle = GDK_LINE_SOLID;
            else if (PL_strcasecmp(values[i], "double_dash") == 0)
                mLineStyle = GDK_LINE_DOUBLE_DASH;

        } else if (PL_strcasecmp(names[i], "cap_style") == 0) {
            
            
            if (PL_strcasecmp(values[i], "round") == 0) 
                mCapStyle = GDK_CAP_ROUND;
            else if (PL_strcasecmp(values[i], "projecting") == 0)
                mCapStyle = GDK_CAP_PROJECTING;
            else if (PL_strcasecmp(values[i], "butt") == 0)
                mCapStyle = GDK_CAP_BUTT;
            else if (PL_strcasecmp(values[i], "not_last") == 0)
                mCapStyle = GDK_CAP_NOT_LAST;

        } else if (PL_strcasecmp(names[i], "join_style") == 0) {
            
            
            if (PL_strcasecmp(values[i], "miter") == 0) 
                mJoinStyle = GDK_JOIN_MITER;
            else if (PL_strcasecmp(values[i], "round") == 0)
                mJoinStyle = GDK_JOIN_ROUND;
            else if (PL_strcasecmp(values[i], "bevel") == 0)
                mJoinStyle = GDK_JOIN_BEVEL;

        } else if (PL_strcasecmp(names[i], "device") == 0) {
            
            
            PR_FREEIF(mSaneDevice);
            mSaneDevice = PL_strdup(values[i]);
        } else if (PL_strcasecmp(names[i], "onScanComplete") == 0) {
            
            
            PR_FREEIF(mOnScanCompleteScript);
            mOnScanCompleteScript = PL_strdup(values[i]);
        } else if(PL_strcasecmp(names[i], "onInitComplete") == 0) {
            
            
            PR_FREEIF(mOnInitCompleteScript);
            mOnInitCompleteScript = PL_strdup(values[i]);
        }
	}
    
    
    
    sprintf( mImageFilename, "/tmp/nsSanePlugin.%p.jpg", (void *)this );

    PlatformNew(); 	
    return NS_OK;

}

NS_METHOD
nsSanePluginInstance::SaveImage()
{
#ifdef DEBUG
    printf("nsSanePluginInstance::SaveImage()\n");
#endif
    
    if (!mImageFilename) {
        NS_ERROR("Attempt to save image before initial scan!");
        return NS_ERROR_FAILURE;
    }

    mFileSelection = gtk_file_selection_new("Choose file to save image to.");
    
    
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(mFileSelection),
                                    "saved.jpg");

    
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(mFileSelection)->ok_button),
                       "clicked", GTK_SIGNAL_FUNC (store_filename), 
                       (gpointer) this);

    
    
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(mFileSelection)->ok_button),
                              "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                              GTK_OBJECT(mFileSelection));
    gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(mFileSelection)->cancel_button),
                              "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                              GTK_OBJECT(mFileSelection));

    
    gtk_widget_show(mFileSelection);
    
    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::DoScanCompleteCallback()
{
#ifdef DEBUG
    printf("nsSanePluginInstance::DoScanCompleteCallback()\n");
#endif

    char *url;

    if (mOnScanCompleteScript && mPluginManager) {
        url = (char *)PR_Malloc(PL_strlen(mOnScanCompleteScript) + 13);
        if (!url)
            return NS_ERROR_OUT_OF_MEMORY;

        sprintf(url, "javascript:%s", mOnScanCompleteScript);   
        mPluginManager->GetURL((nsIPluginInstance *)this, url, "_self");

        PR_FREEIF(url);
    }
    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::DoInitCompleteCallback()
{
#ifdef DEBUG
    printf("nsSanePluginInstance::DoInitCompleteCallback()\n");
#endif

    char *url;

    if (mOnInitCompleteScript && mPluginManager) {
        url = (char *)PR_Malloc(PL_strlen(mOnInitCompleteScript) + 13);
        if (!url)
            return NS_ERROR_OUT_OF_MEMORY;

        sprintf(url, "javascript:%s", mOnInitCompleteScript);   
        mPluginManager->GetURL((nsIPluginInstance *)this, url, "_self");

        PR_FREEIF(url);
    }

    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::GetPeer( nsIPluginInstancePeer* *result )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::GetPeer()\n");
#endif

    fPeer->AddRef();
    *result = fPeer;
    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::Start( void )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::Start()\n");
#endif

    PR_FREEIF(mRGBData);
    mRGBData = nsnull;

    sane_init(0,0);
    mSaneOpen = SANE_FALSE;
    nsresult rv = OpenSaneDeviceIF();
    if (rv != NS_OK) {
        NS_ERROR("Failed to open device in Start()\n");
        return rv;
    }

    PlatformNew();

    
    DoInitCompleteCallback();
    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::Stop( void )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::Stop()\n");
#endif

    
    
    
    
    if (mScanThread)
        PR_JoinThread(mScanThread);

    PR_FREEIF(mRGBData);

    
    if (mSaneOpen) {
        sane_close(mSaneHandle);
        sane_exit();
        mSaneOpen = SANE_FALSE;
    }

    if (remove ( mImageFilename ) == -1)
        perror(mImageFilename);

    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::Destroy( void )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::Destroy()\n");
#endif

    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::SetWindow(nsPluginWindow* window)
{
#ifdef DEBUG
    printf("nsSanePluginInstance::SetWindow()\n");
#endif

    nsresult result;

    
    
    result = PlatformSetWindow( window );
    fWindow = window;

    return result;
}

NS_METHOD
nsSanePluginInstance::NewStream( nsIPluginStreamListener** listener )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::NewStream()\n");
#endif

    if ( listener != NULL )
        *listener = new nsSanePluginStreamListener( this );
    
    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::Print( nsPluginPrint* printInfo )
{
#ifdef DEBUG
    printf("nsSanePluginInstance::Print()\n");
#endif

    if ( printInfo == NULL )
        return NS_ERROR_FAILURE;

    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsSanePluginInstance::HandleEvent(nsPluginEvent* event, PRBool* handled)
{
#ifdef DEBUG
    printf("nsSanePluginInstance::HandleEvent()\n");
#endif

    
    
    *handled = (PRBool)PlatformHandleEvent(event);
    return NS_OK;
}

NS_METHOD
nsSanePluginInstance::GetValue(nsPluginInstanceVariable variable, void *value)
{
#ifdef DEBUG
    printf("nsSanePluginInstance::GetValue()\n");
#endif

    
    return NS_ERROR_NOT_IMPLEMENTED;
}

void
nsSanePluginInstance::PlatformNew( void )
{
#ifdef DEBUG
    printf("nsSaneInstance::PlatformNew\n");
#endif

    fPlatform.window = 0;
}

nsresult
nsSanePluginInstance::PlatformDestroy( void )
{
#ifdef DEBUG
    printf("nsSaneInstance::PlatformDestroy()\n");
#endif

	return NS_OK;
}

nsresult
nsSanePluginInstance::PlatformSetWindow( nsPluginWindow* window )
{
#ifdef DEBUG
    printf("nsSaneInstance::PlatformSetWindow()\n");
#endif


    if ( window == NULL || window->window == NULL )
        return NS_ERROR_NULL_POINTER;
    
    if ( fPlatform.superwin == (GdkSuperWin *)window->window )
        return NS_OK;

    
    fPlatform.superwin = (GdkSuperWin *)window->window;
    fPlatform.height = window->height;
    fPlatform.width = window->width;

    
    if (fPlatform.widget)
        gtk_widget_destroy(fPlatform.widget);
    if (mEvent_box)
        gtk_widget_destroy(mEvent_box);
    if (mDrawing_area)
        gtk_widget_destroy(mDrawing_area);

    fPlatform.widget = gtk_mozbox_new(fPlatform.superwin->bin_window);

    
    mZoom_box.x = 0;
    mZoom_box.y = 0;
    mZoom_box.width = fPlatform.width;
    mZoom_box.height = fPlatform.height;

    
    mEvent_box = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(fPlatform.widget), mEvent_box);
    gtk_widget_show(mEvent_box);

    
    mDrawing_area = ( GtkWidget * )gtk_drawing_area_new();
    gtk_drawing_area_size( GTK_DRAWING_AREA( mDrawing_area ),
                           window->width, window->height);
    gtk_container_add(GTK_CONTAINER(mEvent_box), mDrawing_area);
    gtk_widget_show(mDrawing_area);

    
    GdkGC * da_gc = mDrawing_area->style->fg_gc[mDrawing_area->state];
    gdk_gc_set_line_attributes ( da_gc, mLineWidth, mLineStyle, 
                                 mCapStyle, mJoinStyle);

    
    gtk_signal_connect (GTK_OBJECT(mDrawing_area), "expose_event",
                        GTK_SIGNAL_FUNC(draw), this);

    
    gtk_widget_show( fPlatform.widget );

    return NS_OK;
}

int16
nsSanePluginInstance::PlatformHandleEvent(nsPluginEvent* event)
{
#ifdef DEBUG
    printf("nsSaneInstance::PlatformHandleEvent()\n");
#endif

    
    return 0;
}


NS_IMETHODIMP
nsSanePluginInstance::PaintImage(void)
{
#ifdef DEBUG
    printf("nsSaneInstance::PaintImage(%p)\n", this);
#endif

    if (mDrawing_area && fPlatform.widget) {
        if (mImageFilename) {

            if (!mRGBData) {

                if (mState)
                    
                    
                    return NS_OK;
                
                FILE * f = fopen(mImageFilename, "rb");
                if (!f) 
                    
                    return NS_OK;

                mRGBData = jpeg_file_to_rgb(f, &mRGBWidth, &mRGBHeight);
                if (!mRGBData) {
                    NS_ERROR("Error converting jpeg to rgb data!");
                    return NS_ERROR_FAILURE;
                }

                fclose(f);

                
                unsigned char * tmp = scale_image( mRGBData, 
                                                   mRGBWidth, mRGBHeight,
                                                   fPlatform.width,
                                                   fPlatform.height);
                if (!tmp) {
                    NS_ERROR("Error trying to scale image!");
                    return NS_ERROR_FAILURE;
                }

                
                PR_FREEIF(mRGBData);
                mRGBData = tmp;
            }

            
            gdk_draw_rgb_image (mDrawing_area->window, 
                                mDrawing_area->style->fg_gc[GTK_STATE_NORMAL],
                                0, 0, fPlatform.width, fPlatform.height,
                                GDK_RGB_DITHER_MAX, mRGBData, 
                                fPlatform.width * 3);

            gdk_draw_rectangle (mDrawing_area->window,
                                mDrawing_area->style->fg_gc[mDrawing_area->state],
                                FALSE, mZoom_box.x, mZoom_box.y, 
                                mZoom_box.width, mZoom_box.height);        
            
            gdk_flush();
        }
    }

    return NS_OK;
}

char *
nsSanePluginInstance::GetImageFilename()
{
    if (!mImageFilename)
        return (char *)NULL;
    
    
    return PL_strdup(mImageFilename);
}

GtkWidget *
nsSanePluginInstance::GetFileSelection()
{
    if (!mFileSelection)
        return (GtkWidget *)NULL;

    return mFileSelection;
}

PRBool
nsSanePluginInstance::IsUIThread()
{
    if (!mUIThread)
        return PR_FALSE;

    return (mUIThread == PR_GetCurrentThread());
}







NS_IMETHODIMP
nsSanePluginInstance::ZoomImage( PRUint16 x, PRUint16 y, 
                                 PRUint16 width, PRUint16 height)
{
#ifdef DEBUG
    printf("nsSaneInstance::ZoomImage()\n");
#endif

    int im_width, im_height, c_width, c_height, c_x, c_y;

    if (!mRGBData)
        return NS_ERROR_FAILURE;

    
    im_width = mRGBWidth;
    im_height = mRGBHeight;

    if ( x >= (PRUint16)fPlatform.width ||
         y >= (PRUint16)fPlatform.height )
        return NS_ERROR_FAILURE;

    if ( width + mZoom_box.x >= (PRUint16)fPlatform.width )
        mZoom_box.width = fPlatform.width - mZoom_box.x;

    if ( height + mZoom_box.y >= (PRUint16) fPlatform.height )
        mZoom_box.height = fPlatform.height - mZoom_box.y;

    mZoom_box.x = x;
    mZoom_box.y = y;

    
    c_width = (int)(((double)im_width/(double)fPlatform.width) * 
                    (double)mZoom_box.width);
    c_height =(int)(((double)im_height/(double)fPlatform.height) * 
                    (double)mZoom_box.height);
    c_x = (int)(((double)im_width/(double)fPlatform.width) * 
                (double)mZoom_box.x);
    c_y = (int)(((double)im_height/(double)fPlatform.height) * 
                (double)mZoom_box.y);

    unsigned char * tmp = crop_image(mRGBData, &mRGBWidth, &mRGBHeight,
                                     c_x, c_y, c_width, c_height);
    if (!tmp)
        return NS_ERROR_FAILURE;
    PR_FREEIF(mRGBData);
    mRGBData = tmp;

    mRGBWidth = c_width;
    mRGBHeight = c_height;

    mTopLeftXChange = (float)x/(float)fPlatform.width;
    mTopLeftYChange = (float)y/(float)fPlatform.height;
    mBottomRightXChange = (float)(fPlatform.width - (PRInt32)(x + mZoom_box.width))/(float)fPlatform.width;
    mBottomRightYChange = (float)(fPlatform.height - (PRInt32)(y + mZoom_box.height))/(float)fPlatform.height;

    mZoom_box.x = 0;
    mZoom_box.y = 0;
    mZoom_box.width = (PRUint16)fPlatform.width;
    mZoom_box.height = (PRUint16)fPlatform.height;

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::ZoomImageWithAttributes( PRUint16 x, PRUint16 y, 
                                               PRUint16 width, PRUint16 height,
                                               PRInt32 req_line_width,
                                               const char * req_line_style,
                                               const char * req_cap_style,
                                               const char * req_join_style )
{
#ifdef DEBUG
    printf("nsSaneInstance::ZoomImageWithAttributes()\n");
#endif

    if (!mRGBData)
        return NS_ERROR_FAILURE;

    ZoomImage(x, y, width, height);

    



    
    mLineWidth = req_line_width;

    
    if (PL_strcasecmp(req_line_style, "dash") == 0) 
        mLineStyle = GDK_LINE_ON_OFF_DASH;
    else if (PL_strcasecmp(req_line_style, "solid") == 0)
        mLineStyle = GDK_LINE_SOLID;
    else if (PL_strcasecmp(req_line_style, "double_dash") == 0)
        mLineStyle = GDK_LINE_DOUBLE_DASH;

    
    if (PL_strcasecmp(req_cap_style, "round") == 0) 
        mCapStyle = GDK_CAP_ROUND;
    else if (PL_strcasecmp(req_cap_style, "projecting") == 0)
        mCapStyle = GDK_CAP_PROJECTING;
    else if (PL_strcasecmp(req_cap_style, "butt") == 0)
        mCapStyle = GDK_CAP_BUTT;
    else if (PL_strcasecmp(req_cap_style, "not_last") == 0)
        mCapStyle = GDK_CAP_NOT_LAST;

    
    if (PL_strcasecmp(req_join_style, "miter") == 0) 
        mJoinStyle = GDK_JOIN_MITER;
    else if (PL_strcasecmp(req_join_style, "round") == 0)
        mJoinStyle = GDK_JOIN_ROUND;
    else if (PL_strcasecmp(req_join_style, "bevel") == 0)
        mJoinStyle = GDK_JOIN_BEVEL;

    
    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::Crop(PRUint16 x, PRUint16 y, 
                           PRUint16 width, PRUint16 height)
{
#ifdef DEBUG
    printf("nsSaneInstance::Crop()\n");
#endif

    if ( (x <= (PRUint16)fPlatform.width - width ) &&
         (y <= (PRUint16)fPlatform.height - height) ) {
        
        mZoom_box.x = x; mZoom_box.y = y;
        mZoom_box.width = width;
        mZoom_box.height = height;

    } else {
        return NS_ERROR_FAILURE;
    }

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::Restore(void)
{
#ifdef DEBUG
    printf("nsSaneInstance::Restore()\n");
#endif

    PR_FREEIF(mRGBData);
    mRGBData = nsnull;

    
    mZoom_box.x = mZoom_box.y = 0;
    mZoom_box.width = (PRUint16) fPlatform.width;
    mZoom_box.height = (PRUint16) fPlatform.height;

    PaintImage();

    return NS_OK;
}


NS_IMETHODIMP
nsSanePluginInstance::GetZoomX(PRUint16 * x) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomX()\n");
#endif

    NS_ASSERTION(x != NULL, "null x pointer");
    if (!x)
        return NS_ERROR_NULL_POINTER;

    *x = mZoom_box.x;
    
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomX(PRUint16 x) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomX()\n");
#endif

    if (x > (PRInt32) fPlatform.width)
        mZoom_box.x = (PRInt32) fPlatform.width;
    else
        mZoom_box.x = x;

    return PaintImage();
}


NS_IMETHODIMP
nsSanePluginInstance::GetZoomY(PRUint16 * y) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomY()\n");
#endif

    NS_ASSERTION(y != NULL, "null y pointer");
    if (!y)
        return NS_ERROR_NULL_POINTER;

    *y = mZoom_box.y;
    
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomY(PRUint16 y) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomY()\n");
#endif

    if (y > (PRInt32) fPlatform.height)
        mZoom_box.y = (PRInt32)fPlatform.height;
    else
        mZoom_box.y = y;
    
    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomWidth(PRUint16 * width) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomWidth()\n");
#endif

    NS_ASSERTION(width != NULL, "null width pointer");
    if (!width)
        return NS_ERROR_NULL_POINTER;

    *width = mZoom_box.width;
    
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomWidth(PRUint16 width) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomWidth()\n");
#endif

    if (width + mZoom_box.x > (PRUint16)fPlatform.width)
        mZoom_box.width = (PRInt32) fPlatform.width - mZoom_box.x;
    else
        mZoom_box.width = width;

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomHeight(PRUint16 * height) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomHeight()\n");
#endif

    NS_ASSERTION(height != NULL, "null height pointer");
    if (!height)
        return NS_ERROR_NULL_POINTER;

    *height = mZoom_box.height;
    
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomHeight(PRUint16 height) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomHeight()\n");
#endif

    if (height + mZoom_box.y > (PRUint16) fPlatform.height)
        mZoom_box.height = fPlatform.height - mZoom_box.y;
    else
        mZoom_box.height = height;

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomLineWidth(PRInt32 * aZoomLineWidth) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomLineWidth()\n");
#endif

    NS_ASSERTION(aZoomLineWidth != NULL, "null width pointer");   
    if (!aZoomLineWidth)
        return NS_ERROR_NULL_POINTER;

    *aZoomLineWidth = mLineWidth;

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomLineWidth(PRInt32 aZoomLineWidth) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomLineWidth()\n");
#endif

    if (aZoomLineWidth <= 0)
        return NS_ERROR_INVALID_ARG;

    mLineWidth = aZoomLineWidth;

    return PaintImage();
}

#define MAX_LINE_ATTRIBUTE_LENGTH 20

NS_IMETHODIMP
nsSanePluginInstance::GetZoomLineStyle(char ** aZoomLineStyle) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomLineStyle()\n");
#endif

    NS_ASSERTION(*aZoomLineStyle != NULL, "null width pointer");   
    if (!*aZoomLineStyle)
        return NS_ERROR_NULL_POINTER;

    *aZoomLineStyle = (char*) nsMemory::Alloc(MAX_LINE_ATTRIBUTE_LENGTH);
    if (! *aZoomLineStyle)
        return NS_ERROR_OUT_OF_MEMORY;
    
    if (mLineStyle == GDK_LINE_ON_OFF_DASH)
        PL_strcpy(*aZoomLineStyle, "dash");
    else if (mLineStyle == GDK_LINE_SOLID)
        PL_strcpy(*aZoomLineStyle, "solid");
    else if (mLineStyle == GDK_LINE_DOUBLE_DASH)
        PL_strcpy(*aZoomLineStyle, "double_dash");
    else
        PL_strcpy(*aZoomLineStyle, "unknown");

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomLineStyle(const char * aZoomLineStyle) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomLineStyle()\n");
#endif

    NS_ASSERTION(aZoomLineStyle != NULL, "null width pointer");   
    if (!aZoomLineStyle)
        return NS_ERROR_NULL_POINTER;

    
    if (PL_strcasecmp(aZoomLineStyle, "dash") == 0) 
        mLineStyle = GDK_LINE_ON_OFF_DASH;
    else if (PL_strcasecmp(aZoomLineStyle, "solid") == 0)
        mLineStyle = GDK_LINE_SOLID;
    else if (PL_strcasecmp(aZoomLineStyle, "double_dash") == 0)
        mLineStyle = GDK_LINE_DOUBLE_DASH;

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomCapStyle(char ** aZoomCapStyle) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomCapStyle()\n");
#endif

    NS_ASSERTION(*aZoomCapStyle != NULL, "null width pointer");   
    if (!*aZoomCapStyle)
        return NS_ERROR_NULL_POINTER;

    *aZoomCapStyle = (char*) nsMemory::Alloc(MAX_LINE_ATTRIBUTE_LENGTH);
    if (! *aZoomCapStyle)
        return NS_ERROR_OUT_OF_MEMORY;
    
    if (mCapStyle == GDK_CAP_ROUND)
        PL_strcpy(*aZoomCapStyle, "round");
    else if (mCapStyle == GDK_CAP_PROJECTING)
        PL_strcpy(*aZoomCapStyle, "projecting");
    else if (mCapStyle == GDK_CAP_BUTT)
        PL_strcpy(*aZoomCapStyle, "butt");
    else if (mCapStyle == GDK_CAP_NOT_LAST)
        PL_strcpy(*aZoomCapStyle, "not_last");
    else
        PL_strcpy(*aZoomCapStyle, "unknown");


    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomCapStyle(const char * aZoomCapStyle) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomCapStyle()\n");
#endif

    NS_ASSERTION(aZoomCapStyle != NULL, "null width pointer");   
    if (!aZoomCapStyle)
        return NS_ERROR_NULL_POINTER;

    
    if (PL_strcasecmp(aZoomCapStyle, "round") == 0) 
        mCapStyle = GDK_CAP_ROUND;
    else if (PL_strcasecmp(aZoomCapStyle, "projecting") == 0)
        mCapStyle = GDK_CAP_PROJECTING;
    else if (PL_strcasecmp(aZoomCapStyle, "butt") == 0)
        mCapStyle = GDK_CAP_BUTT;
    else if (PL_strcasecmp(aZoomCapStyle, "not_last") == 0)
        mCapStyle = GDK_CAP_NOT_LAST;

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomJoinStyle(char ** aZoomJoinStyle) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomJoinStyle()\n");
#endif

    NS_ASSERTION(*aZoomJoinStyle != NULL, "null width pointer");   
    if (!*aZoomJoinStyle)
        return NS_ERROR_NULL_POINTER;

    *aZoomJoinStyle = (char*) nsMemory::Alloc(MAX_LINE_ATTRIBUTE_LENGTH);
    if (! *aZoomJoinStyle)
        return NS_ERROR_OUT_OF_MEMORY;
    
    if (mJoinStyle == GDK_JOIN_MITER)
        PL_strcpy(*aZoomJoinStyle, "miter");
    else if (mJoinStyle == GDK_JOIN_ROUND)
        PL_strcpy(*aZoomJoinStyle, "round");
    else if (mJoinStyle == GDK_JOIN_BEVEL)
        PL_strcpy(*aZoomJoinStyle, "bevel");
    else
        PL_strcpy(*aZoomJoinStyle, "unknown");

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomJoinStyle(const char * aZoomJoinStyle) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomJoinStyle()\n");
#endif

    NS_ASSERTION(aZoomJoinStyle != NULL, "null width pointer");   
    if (!aZoomJoinStyle)
        return NS_ERROR_NULL_POINTER;

    
    if (PL_strcasecmp(aZoomJoinStyle, "miter") == 0) 
        mJoinStyle = GDK_JOIN_MITER;
    else if (PL_strcasecmp(aZoomJoinStyle, "round") == 0)
        mJoinStyle = GDK_JOIN_ROUND;
    else if (PL_strcasecmp(aZoomJoinStyle, "bevel") == 0)
        mJoinStyle = GDK_JOIN_BEVEL;

    return PaintImage();
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomBR_XChange(float * aZoomBR_XChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomBR_XChange()\n");
#endif

    *aZoomBR_XChange = mBottomRightXChange;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomBR_XChange(float aZoomBR_XChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomBR_XChange()\n");
#endif

    
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomBR_YChange(float * aZoomBR_YChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomBR_YChange()\n");
#endif

    *aZoomBR_YChange = mBottomRightYChange;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomBR_YChange(float aZoomBR_YChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomBR_YChange()\n");
#endif

    
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomTL_XChange(float * aZoomTL_XChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomTL_XChange()\n");
#endif

    *aZoomTL_XChange = mTopLeftXChange;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomTL_XChange(float aZoomTL_XChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomTL_XChange()\n");
#endif

    
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetZoomTL_YChange(float * aZoomTL_YChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::GetZoomTL_YChange()\n");
#endif

    *aZoomTL_YChange = mTopLeftYChange;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetZoomTL_YChange(float aZoomTL_YChange) 
{
#ifdef DEBUG
    printf("nsSaneInstance::SetZoomTL_YChange()\n");
#endif

    
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetQuality(PRInt32 * aQuality)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetQuality()\n");
#endif

    NS_ASSERTION(aQuality != NULL, "null compression quality pointer");   
    if (!aQuality)
        return NS_ERROR_NULL_POINTER;    
    
    *aQuality = mCompQuality;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetQuality(PRInt32 aQuality)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetQuality()\n");
#endif

    NS_ASSERTION(aQuality > -1 && aQuality < 101, 
                 "Invalid compression quality");
    if (aQuality < 0 || aQuality > 100)
        return NS_ERROR_INVALID_ARG;

    mCompQuality = aQuality;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::GetMethod(char ** aMethod)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetMethod()\n");
#endif

    NS_ASSERTION(aMethod != NULL, "Null method pointer");
    if (!aMethod)
        return NS_ERROR_NULL_POINTER;

    *aMethod = (char*) nsMemory::Alloc(8);
    if (!*aMethod)
        return NS_ERROR_OUT_OF_MEMORY;

    switch (mCompMethod) {
    case JDCT_ISLOW:
        PL_strcpy(*aMethod, "SLOW");
        break;
    case JDCT_IFAST:
        PL_strcpy(*aMethod, "FAST");
        break;
    case JDCT_FLOAT:
        PL_strcpy(*aMethod, "FLOAT");
        break;
    default:
        PL_strcpy(*aMethod, "DEFAULT");
        
    }

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetMethod(const char * aMethod)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetMethod()\n");
#endif

    NS_ASSERTION(aMethod != NULL, "Null method pointer");
    if (!aMethod)
        return NS_ERROR_NULL_POINTER;

    if (PL_strcasecmp(aMethod, "SLOW") == 0)
        mCompMethod = JDCT_ISLOW;
    else if (PL_strcasecmp(aMethod, "FAST") == 0)
        mCompMethod = JDCT_IFAST;
    else if (PL_strcasecmp(aMethod, "FLOAT") == 0)
        mCompMethod = JDCT_FLOAT;
    else
        mCompMethod = JDCT_DEFAULT;

    return NS_OK;
}







NS_IMETHODIMP
nsSanePluginInstance::GetDeviceOptions(char ** aDeviceOptions)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetDeviceOptions()\n");
#endif

    const SANE_Option_Descriptor * optdesc;
    SANE_Int string_size;
    char *range_string, *word_string;
    nsresult rv;

    NS_ASSERTION(aDeviceOptions != NULL, "null options pointer");   
    if (!aDeviceOptions)
        return NS_ERROR_NULL_POINTER;

    *aDeviceOptions = nsnull;

    rv = OpenSaneDeviceIF();
    if (rv != NS_OK)
        return rv;

    
    optdesc = sane_get_option_descriptor(mSaneHandle, 0);
    if (!optdesc || optdesc->size == 0) {
        NS_ERROR("Error trying to get number of options for device!");
        return NS_ERROR_FAILURE;
    }

    if (optdesc->size == 1) { 
        NS_ERROR("Hmm.. That's odd! No options for this device?");
        return NS_OK; 
    }

    
    string_size = ((optdesc->size - 1) * MAX_OPT_SIZE) + 1;
    *aDeviceOptions = (char*) nsMemory::Alloc(string_size);
    NS_ENSURE_TRUE(*aDeviceOptions, NS_ERROR_OUT_OF_MEMORY);

    
    for (int n=0; n<string_size; n++)
        (*aDeviceOptions)[n] = '\0';

    
    int i = 1;
    while((optdesc = sane_get_option_descriptor(mSaneHandle, i)) != NULL) {
        i++;

        
        if (optdesc->type == SANE_TYPE_GROUP) 
            continue;
        
        





        
        if (optdesc->name) 
            optioncat(aDeviceOptions, optdesc->name, ", ");
        else 
            strcat(*aDeviceOptions, "N/A, ");

        
        
        if (optdesc->title)
            optioncat(aDeviceOptions, optdesc->title, ", ");
        else 
            PL_strcat(*aDeviceOptions, "N/A, ");

        
        
        if (optdesc->desc) 
            optioncat(aDeviceOptions, optdesc->desc, ", ");
        else
            PL_strcat(*aDeviceOptions, "N/A, ");

        
        
        switch (optdesc->type) {

        case SANE_TYPE_BOOL:
            PL_strcat(*aDeviceOptions, "BOOL, ");
            break;

        case SANE_TYPE_INT:
            PL_strcat(*aDeviceOptions, "INT, ");
            break;

        case SANE_TYPE_FIXED:
            PL_strcat(*aDeviceOptions, "FIXED, ");
            break;

        case SANE_TYPE_STRING:
            PL_strcat(*aDeviceOptions, "STRING, ");
            break;

        case SANE_TYPE_BUTTON:
            PL_strcat(*aDeviceOptions, "BUTTON, ");
            break;

        case SANE_TYPE_GROUP:
            PL_strcat(*aDeviceOptions, "GROUP, ");
            break;

        default:
            PL_strcat(*aDeviceOptions, "UNKNOWN, ");
        }

        
        
        switch (optdesc->unit) {
        case SANE_UNIT_NONE:
            PL_strcat(*aDeviceOptions, "NONE, ");
            break;

        case SANE_UNIT_PIXEL:
            PL_strcat(*aDeviceOptions, "PIXEL, ");
            break;

        case SANE_UNIT_BIT:
            PL_strcat(*aDeviceOptions, "BIT, ");
            break;

        case SANE_UNIT_MM:
            PL_strcat(*aDeviceOptions, "MM, ");
            break;

        case SANE_UNIT_DPI:
            PL_strcat(*aDeviceOptions, "PERCENT, ");
            break;

        case SANE_UNIT_MICROSECOND:
            PL_strcat(*aDeviceOptions, "MICROSECOND, ");
            break;

        default:
            PL_strcat(*aDeviceOptions, "UNKNOWN, ");
        }

        
        
        char * size_option = (char *)PR_MALLOC(64);
        if (!size_option)
            return NS_ERROR_OUT_OF_MEMORY;
        sprintf(size_option, "%i, ", optdesc->size);
        PL_strcat(*aDeviceOptions, size_option);
        PR_FREEIF(size_option);

        
        
        if (SANE_OPTION_IS_ACTIVE(optdesc->cap))
            PL_strcat(*aDeviceOptions, "ACTIVE, ");
        else 
            PL_strcat(*aDeviceOptions, "UNACTIVE, ");

        
        
        if (SANE_OPTION_IS_SETTABLE(optdesc->cap))
            PL_strcat(*aDeviceOptions, "SETTABLE, ");
        else 
            PL_strcat(*aDeviceOptions, "UNSETTABLE, ");

        
        
        switch (optdesc->constraint_type) {
        case SANE_CONSTRAINT_RANGE:
            range_string = (char *)PR_MALLOC(128);
            if (!range_string)
                return NS_ERROR_OUT_OF_MEMORY;
            sprintf(range_string, "RANGE, %i, %i, %i; ",
                    optdesc->constraint.range->min,
                    optdesc->constraint.range->max,
                    optdesc->constraint.range->quant);
            PL_strcat(*aDeviceOptions, range_string);
            PR_FREEIF(range_string);
            break;

        case SANE_CONSTRAINT_WORD_LIST:
            word_string = (char *) PR_MALLOC(64);
            if (!word_string)
                return NS_ERROR_OUT_OF_MEMORY;
            
            PL_strcat(*aDeviceOptions, "WORD_LIST");
            for (int word_index=1; 
                 word_index <= optdesc->constraint.word_list[0]; 
                 word_index++) {
                sprintf(word_string, ", %i", optdesc->constraint.word_list[i]);
                PL_strcat(*aDeviceOptions, word_string);
            }
            PL_strcat(*aDeviceOptions, ";");
            PR_FREEIF(word_string);
            break;

        case SANE_CONSTRAINT_STRING_LIST:
            PL_strcat(*aDeviceOptions, "STRING_LIST");
            for (int string_index=0; 
                 optdesc->constraint.string_list[string_index];
                 string_index++) {
                PL_strcat(*aDeviceOptions, ", ");
                PL_strcat(*aDeviceOptions, 
                          optdesc->constraint.string_list[string_index]);
            }
            PL_strcat(*aDeviceOptions, ";");
            break;

        default:
            
            PL_strcat(*aDeviceOptions, "NONE;");
        }        
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetDeviceOptions(const char * aDeviceOptions)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetDeviceOptions()\n");
#endif

    NS_ERROR("DeviceOptions is read-only!\n");
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetActiveDevice(char ** aActiveDevice)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetActiveDevice()\n");
#endif

    NS_ASSERTION(aActiveDevice != NULL, "null device pointer");   
    if (!aActiveDevice)
        return NS_ERROR_NULL_POINTER;

    *aActiveDevice = nsnull;
    if (!mSaneDevice)
        return NS_OK;
    
    *aActiveDevice = (char*) nsMemory::Alloc(PL_strlen(mSaneDevice) + 1);
    NS_ENSURE_TRUE(*aActiveDevice, NS_ERROR_OUT_OF_MEMORY);
    PL_strcpy(*aActiveDevice, mSaneDevice);
    
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetActiveDevice(const char * aActiveDevice)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetActiveDevice()\n");
#endif

    NS_ASSERTION(aActiveDevice, "null active device pointer");
    if (!aActiveDevice)
        return NS_ERROR_NULL_POINTER;

    PR_FREEIF (mSaneDevice);
    mSaneDevice = PL_strdup(aActiveDevice);

    if (mSaneOpen) {
        sane_close(mSaneHandle);
        sane_exit();
        sane_init(0,0);
        if (sane_open(mSaneDevice, &mSaneHandle) != SANE_STATUS_GOOD) {
            mSaneOpen = SANE_FALSE;
            NS_ERROR("Error trying to reopen sane device!\n");
            return NS_ERROR_FAILURE;
        }
            
    } else {
        if (sane_open(mSaneDevice, &mSaneHandle) != SANE_STATUS_GOOD) {
            NS_ERROR("Error trying to open closed sane device!\n");
            return NS_ERROR_FAILURE;
        }
        mSaneOpen = SANE_TRUE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::OpenSaneDeviceIF( void )
{
#ifdef DEBUG
    printf("nsSaneInstance::OpenSaneDeviceIF()\n");
#endif

    if (mSaneOpen) {
        return NS_OK;
    }

    if (!mSaneDevice) {
        NS_ERROR("Device name not set!");
        return NS_ERROR_FAILURE;
    }

    SANE_Status status = sane_open(mSaneDevice, &mSaneHandle);
    if (status != SANE_STATUS_GOOD) {
        NS_ERROR ("sane_open returned error code");
        return NS_ERROR_FAILURE;
    }

    mSaneOpen = SANE_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::GetImageParameters(char ** aImageParameters)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetImageParameters()\n");
#endif

    SANE_Status status;
    SANE_Parameters params;
    nsresult rv;
    
    NS_ASSERTION(aImageParameters != NULL, "null parameters pointer");   
    if (!aImageParameters)
        return NS_ERROR_NULL_POINTER;

    *aImageParameters = nsnull;

    rv = OpenSaneDeviceIF();;
    if (rv != NS_OK)
        return rv;
    
    status = sane_get_parameters(mSaneHandle, &params);
    if (status != SANE_STATUS_GOOD) {
        sane_close(mSaneHandle);
        NS_ERROR("Failed to get parameters for sane device!");
        return NS_ERROR_FAILURE;
    }

#define MAX_PARAM_LEN 256
    *aImageParameters = (char *) nsMemory::Alloc(MAX_PARAM_LEN);
    if (!*aImageParameters)
        return NS_ERROR_OUT_OF_MEMORY;

    char * format_string;
    switch (params.format) {

    case SANE_FRAME_GRAY:
        format_string = PL_strdup("GRAY");
        break;

    case SANE_FRAME_RGB:
        format_string = PL_strdup("RGB");
        break;

    case SANE_FRAME_RED:
        format_string = PL_strdup("RED");
        break;

    case SANE_FRAME_GREEN:
        format_string = PL_strdup("GREEN");
        break;

    case SANE_FRAME_BLUE:
        format_string = PL_strdup("BLUE");
        break;

    default:
        format_string = PL_strdup("UNKNOWN");
    }

    sprintf(*aImageParameters, "%s, %s, %i, %i, %i, %i",
            format_string,
            params.last_frame ? "TRUE" : "FALSE",
            params.lines,
            params.depth,
            params.pixels_per_line,
            params.bytes_per_line);


    
    PR_FREEIF(format_string);

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetImageParameters(const char * aImageParameters)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetImageParameters()\n");
#endif

    NS_ERROR("ImageParameters is read-only!\n");
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetAvailableDevices(char ** aAvailableDevices)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetAvailableDevices()\n");
#endif

    SANE_Status status;
    const SANE_Device ** device_list;
    int num, i;

    NS_ASSERTION(aAvailableDevices != NULL, "null parameters pointer");   
    if (!aAvailableDevices)
        return NS_ERROR_NULL_POINTER;

    *aAvailableDevices = nsnull;

    status = sane_get_devices(&device_list, SANE_TRUE);
    if (status != SANE_STATUS_GOOD)
        return NS_ERROR_FAILURE;

    
    for ( num=0; device_list[num]; num++);

    
#define MAX_DEVICE_SIZE 256
    *aAvailableDevices = (char*) nsMemory::Alloc(num * MAX_DEVICE_SIZE);
    if (!*aAvailableDevices)
        return NS_ERROR_OUT_OF_MEMORY;

    for ( i=0; i < num * MAX_DEVICE_SIZE; i++)
        (*aAvailableDevices)[i] = '\0';

    for ( i=0; i<num; i++) {
        optioncat(aAvailableDevices, device_list[i]->name,   ", ");
        optioncat(aAvailableDevices, device_list[i]->vendor, ", ");
        optioncat(aAvailableDevices, device_list[i]->model,  ", ");
        optioncat(aAvailableDevices, device_list[i]->type,   "; ");
    }

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetAvailableDevices(const char * aAvailableDevices)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetAvailableDevices()\n");
#endif

    NS_ERROR("AvailableDevices is read-only!\n");
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsSanePluginInstance::ScanImage(void)
{
#ifdef DEBUG
    printf("nsSaneInstance::ScanImage()\n");
#endif

    mState = 1;
    mSuccess = 0;

    mScanThread = PR_CreateThread( PR_USER_THREAD, scanimage_thread_routine, 
                                  (void*)this, PR_PRIORITY_HIGH, 
                                  PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);

    if (mScanThread == NULL) {
        NS_ERROR("Could not create thread!");
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetOption(const char * name, const char * value)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetOption()\n");
#endif

    nsresult rv;
    SANE_Int optnum;
    const SANE_Option_Descriptor * optdesc;
    void * option_value;
    SANE_Status status;

    NS_ASSERTION(name != NULL && value != NULL, "null pointer");
    if (!name || !value)
        return NS_ERROR_NULL_POINTER;

    rv = OpenSaneDeviceIF();
    if (rv != NS_OK) 
        return rv;

    
    for ( optnum=1; 
          ((optdesc = sane_get_option_descriptor(mSaneHandle, optnum)) != NULL)
              && (PL_strcasecmp(name, optdesc->name) != 0);
          optnum++ );
    if (!optdesc) {
        NS_ERROR("Option not found for sane device!");
        return NS_ERROR_INVALID_ARG;
    }

    
    if (optdesc->type == SANE_TYPE_BOOL) {
        SANE_Bool sbool;
        if (PL_strcasecmp(value, "TRUE") == 0) {
            sbool = SANE_TRUE;
        } else 
            sbool = SANE_FALSE;

        option_value = (void *) &sbool;

    } else if ( optdesc->type == SANE_TYPE_INT ||
                optdesc->type == SANE_TYPE_FIXED ) {
        SANE_Int sint;
        sscanf(value, "%i", &sint);

        option_value = (void *) &sint;

    } else if (optdesc->type == SANE_TYPE_STRING) {
        option_value = (void *) value;
    } else {
        option_value = NULL;
    }

    
    status = sane_control_option( mSaneHandle, optnum, 
                                  SANE_ACTION_SET_VALUE, option_value,
                                  NULL);
    if (status != SANE_STATUS_GOOD) {
        NS_ERROR("Error trying to set option!");
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

int
nsSanePluginInstance::WritePNMHeader (int fd, SANE_Frame format, 
                                  int width, int height, int depth)
{
#ifdef DEBUG
    printf("nsSaneInstance::WritePNMHeader()\n");
#endif

    char b[32];

    switch (format) {
    case SANE_FRAME_RED:
    case SANE_FRAME_GREEN:
    case SANE_FRAME_BLUE:
    case SANE_FRAME_RGB:
        sprintf (b, "P6\n%d %d\n255\n", width, height);
        break;

    default:
        if (depth == 1)
            sprintf (b, "P4\n%d %d\n", width, height);
        else
            sprintf (b, "P5\n%d %d\n255\n", width, height);
        break;
    }

    return write(fd, b, PL_strlen(b));
}







NS_IMETHODIMP
nsSanePluginInstance::GetSuccess(PRBool * aSuccess)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetSuccess()\n");
#endif

    NS_PRECONDITION(aSuccess != nsnull, "null ptr");
    if (! aSuccess) {
        NS_ERROR("Null pointer passed to GetSuccess!\n");
        return NS_ERROR_NULL_POINTER;
    }

    *aSuccess = mSuccess;
    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetSuccess(PRBool aSuccess)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetSuccess()\n");
#endif

    
    NS_ERROR("Success is a read-only flag!\n");
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSanePluginInstance::GetState(char ** aState)
{
#ifdef DEBUG
    printf("nsSaneInstance::GetState()\n");
#endif

    NS_PRECONDITION(aState != nsnull, "null ptr");
    if (! aState) {
        NS_ERROR("Null pointer passed to GetSuccess!\n");
        return NS_ERROR_NULL_POINTER;
    }

    *aState = (char*) nsMemory::Alloc(5);
    if (!*aState) {
        NS_ERROR("Unable to allocate State string!\n");
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (mState == 0)
        PL_strcpy(*aState, "IDLE");
    else
        PL_strcpy(*aState, "BUSY");

    return NS_OK;
}

NS_IMETHODIMP
nsSanePluginInstance::SetState(const char * aState)
{
#ifdef DEBUG
    printf("nsSaneInstance::SetState()\n");
#endif

    
    NS_ERROR("Success is a read-only flag!\n");
    return NS_ERROR_FAILURE;
}







nsSanePluginStreamListener::nsSanePluginStreamListener(nsSanePluginInstance* inst)
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::nsSanePluginStreamListener()\n");
#endif

    PR_AtomicIncrement(&gPluginObjectCount);

    mPlugInst = inst;
}

nsSanePluginStreamListener::~nsSanePluginStreamListener(void)
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::~nsSanePluginStreamListener()\n");
#endif

    PR_AtomicDecrement(&gPluginObjectCount);
}




NS_IMPL_ISUPPORTS1(nsSanePluginStreamListener, nsIPluginStreamListener)

NS_METHOD
nsSanePluginStreamListener::OnStartBinding( nsIPluginStreamInfo* pluginInfo )
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::OnStartBinding()\n");
#endif

    return NS_OK;
}

NS_METHOD
nsSanePluginStreamListener::OnDataAvailable( nsIPluginStreamInfo* pluginInfo, 
                                             nsIInputStream* input, 
                                             PRUint32 length )
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::OnDataAvailable()\n");
#endif

    
    
    return NS_OK;
}


NS_METHOD
nsSanePluginStreamListener::OnFileAvailable( nsIPluginStreamInfo* pluginInfo, 
                                             const char* fileName )
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::OnFileAvailable()\n");
#endif

    return NS_OK;
}

NS_METHOD
nsSanePluginStreamListener::OnStopBinding( nsIPluginStreamInfo* pluginInfo,
                                           nsresult status )
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::OnStopBinding()\n");
#endif

    return NS_OK;
}

NS_METHOD
nsSanePluginStreamListener::OnNotify( const char* url, nsresult status )
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::OnNotify()\n");
#endif

    return NS_OK;
}

NS_METHOD
nsSanePluginStreamListener::GetStreamType(nsPluginStreamType *result)
{
#ifdef DEBUG
    printf("nsSanePluginStreamListener::GetStreamType()\n");
#endif

    *result = nsPluginStreamType_Normal;
    return NS_OK;
}







gboolean draw (GtkWidget *widget, GdkEventExpose *event, gpointer data) {

    nsSanePluginInstance * pthis = (nsSanePluginInstance *)data;

    pthis->PaintImage();
    return TRUE;
}

nsresult optioncat ( char ** dest, const char * src, const char * ending )
{
    char * p;
    int i;

    NS_ASSERTION(dest != NULL, "null option string pointer");
    if (!dest)
        return NS_ERROR_NULL_POINTER;

    NS_ASSERTION(src != NULL, "null option string pointer");
    if (!src)
        return NS_ERROR_NULL_POINTER;

    NS_ASSERTION(ending != NULL, "null option string pointer");
    if (!ending)
        return NS_ERROR_NULL_POINTER;

    
    p = PL_strdup(src);
    for ( i=0; p[i]; i++ ) {
        if (p[i] == ',')
            p[i] = '~';  
        else if (p[i] == ';')
            p[i] = '|';  

    }


    PL_strcat(*dest, p);
    PL_strcat(*dest, ending);
    PR_FREEIF(p);

    return NS_OK;
}

void
my_jpeg_error_exit (j_common_ptr cinfo)
{
    emptr errmgr;

    errmgr = (emptr) cinfo->err;
    cinfo->err->output_message(cinfo);
    siglongjmp(errmgr->setjmp_buffer, 1);
    return;
}

unsigned char      *
jpeg_file_to_rgb (FILE * f, int *w, int *h)
{
  struct jpeg_decompress_struct cinfo;
  struct my_JPEG_error_mgr jerr;
  unsigned char      *data, *line[16], *ptr;
  int                 x, y, i;

  cinfo.err = jpeg_std_error(&(jerr.pub));
  jerr.pub.error_exit = my_jpeg_error_exit;

  
  if (sigsetjmp(jerr.setjmp_buffer, 1)) {
    
    
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, f);
  jpeg_read_header(&cinfo, TRUE);
  cinfo.do_fancy_upsampling = FALSE;
  cinfo.do_block_smoothing = FALSE;

  cinfo.scale_num = 2;
  cinfo.scale_denom = 1;
  jpeg_start_decompress(&cinfo);
  *w = cinfo.output_width;
  *h = cinfo.output_height;
  data = (unsigned char *)PR_Malloc(*w ** h * 3);
  if (!data) {
    NS_ERROR("jpeg_file_to_rgb: Error trying to allocate buffer!\n");
    jpeg_destroy_decompress(&cinfo);
    return NULL;
  }
  ptr = data;

  if (cinfo.rec_outbuf_height > 16) {
      NS_ERROR("ERROR: JPEG uses line buffers > 16. Cannot load.\n");
      return NULL;
  }

  if (cinfo.output_components == 3) {
    for (y = 0; y < *h; y += cinfo.rec_outbuf_height) {
      for (i = 0; i < cinfo.rec_outbuf_height; i++) {
	line[i] = ptr;
	ptr += *w * 3;
      }

      jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
    }
  } else if (cinfo.output_components == 1) {
    for (i = 0; i < cinfo.rec_outbuf_height; i++) {
      if ((line[i] = (unsigned char *)PR_Malloc(*w)) == NULL) {
	int                 t = 0;
	
	NS_ERROR("jpeg_file_to_rgb: Error tying to allocate line!\n");
	
	for (t = 0; t < i; t++)
	  PR_FREEIF(line[t]);
	jpeg_destroy_decompress(&cinfo);
	return NULL;
      }
    }

    for (y = 0; y < *h; y += cinfo.rec_outbuf_height) {
      jpeg_read_scanlines(&cinfo, line, cinfo.rec_outbuf_height);
      for (i = 0; i < cinfo.rec_outbuf_height; i++) {
	for (x = 0; x < *w; x++) {
	  *ptr++ = line[i][x];
	  *ptr++ = line[i][x];
	  *ptr++ = line[i][x];
	}
      }
    }

    for (i = 0; i < cinfo.rec_outbuf_height; i++)
      PR_FREEIF(line[i]);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  return data;
}


unsigned char *
scale_image(unsigned char *rgb_data, int rgb_width, int rgb_height,
	    int w, int h)
{
  int                 x, y, *xarray;
  unsigned char     **yarray, *ptr, *ptr2, *ptr22, *new_data;
  int                 l, r, m, pos, inc, w3;

  new_data = (unsigned char *) PR_Malloc( w * h * 3);
  if (!new_data) {
    NS_ERROR("ERROR: Cannot allocate image buffer\n");
    return NULL;
  }

  xarray = (int *)PR_Malloc(sizeof(int) * w);
  if (!xarray) {
    NS_ERROR("ERROR: Cannot allocate X co-ord buffer\n");
    PR_FREEIF(new_data);
    return NULL;
  }

  yarray = (unsigned char **)PR_Malloc(sizeof(unsigned char *) * h);
  if (!yarray) {
    NS_ERROR("ERROR: Cannot allocate Y co-ord buffer\n");
    PR_FREEIF(new_data);
    PR_FREEIF(xarray);
    return NULL;
  }
  
  ptr22 = rgb_data;
  w3 = rgb_width * 3;
  inc = 0;
  l = 0;
  r = 0;
  m = w - l - r;

  inc = (rgb_width << 16) / m;
  pos = 0;

  for (x = l; x < l + m; x++) {
    xarray[x] = (pos >> 16) + (pos >> 16) + (pos >> 16);
    pos += inc;
  }

  pos = (rgb_width - r) << 16;
  for (x = w - r; x < w; x++) {
    xarray[x] = (pos >> 16) + (pos >> 16) + (pos >> 16);
    pos += 0x10000;
  }

  l = 0;
  r = 0;
  m = h - l - r;

  if (m > 0)
    inc = (rgb_height << 16) / m;

  pos = 0;

  for (x = l; x < l + m; x++) {
    yarray[x] = ptr22 + ((pos >> 16) * w3);
    pos += inc;
  }

  pos = (rgb_height - r) << 16;
  for (x = h - r; x < h; x++) {
    yarray[x] = ptr22 + ((pos >> 16) * w3);
    pos += 0x10000;
  }

  ptr = new_data;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      ptr2 = yarray[y] + xarray[x];
      *ptr++ = (int)*ptr2++;
      *ptr++ = (int)*ptr2++;
      *ptr++ = (int)*ptr2;
    }
  }

  PR_FREEIF(xarray);
  PR_FREEIF(yarray);

  return new_data;
}



unsigned char *
crop_image(unsigned char *rgb_data, int *rgb_width, int *rgb_height,
	   gint x, gint y, gint w, gint h)
{
  unsigned char      *data;
  int                 xx, yy, w3, w4;
  unsigned char      *ptr1, *ptr2;

  if (!rgb_data)
    return NULL;

  if (x < 0)
    {
      w += x;
      x = 0;
    }
  if (y < 0)
    {
      h += y;
      y = 0;
    }
  if (x >= *rgb_width)
    return NULL;
  if (y >= *rgb_height)
    return NULL;
  if (w <= 0)
    return NULL;
  if (h <= 0)
    return NULL;
  if (x + w > *rgb_width)
    w = *rgb_width - x;
  if (y + h > *rgb_height)
    h = *rgb_height - y;
  if (w <= 0)
    return NULL;
  if (h <= 0)
    return NULL;

  w3 = *rgb_width * 3;
  w4 = (*rgb_width - w) * 3;
  data = (unsigned char *)PR_Malloc(w * h * 3);
  if (data == NULL)
    return NULL;
  ptr1 = rgb_data + (y * w3) + (x * 3);
  ptr2 = data;
  for (yy = 0; yy < h; yy++)
    {
      for (xx = 0; xx < w; xx++)
	{
	  *ptr2++ = *ptr1++;
	  *ptr2++ = *ptr1++;
	  *ptr2++ = *ptr1++;
	}
      ptr1 += w4;
    }
  
  *rgb_width = w;
  *rgb_height = h;

  return data;
}

void PR_CALLBACK scanimage_thread_routine( void * arg )
{
    nsSanePluginInstance * pthis = (nsSanePluginInstance *)arg;

    nsresult rv;
    SANE_Status status;
    SANE_Parameters param;
    int len;
    FILE * out;
    SANE_Byte * buf;
    JSAMPROW row_pointer[1];
    char *fname;

    
    struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	
    jerr.error_exit = my_jpeg_error_exit; 
	cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_compress(&cinfo);

    rv = pthis->OpenSaneDeviceIF();
    if (rv != NS_OK)
        goto error_exit;

    
    fname = pthis->GetImageFilename();
    if (!fname)
        goto error_exit;
    out = fopen(fname, "wb");
    PR_FREEIF(fname);
    if (out == NULL) {
        NS_ERROR("Unable to open mImageFilename!\n");
        sane_cancel(pthis->mSaneHandle);
        goto error_exit;       
    }
    jpeg_stdio_dest(&cinfo, out);

    status = sane_start(pthis->mSaneHandle);
    if (status != SANE_STATUS_GOOD) {
        NS_ERROR("Error trying to start sane device!");
        goto error_exit;
    }

    status = sane_get_parameters(pthis->mSaneHandle, &param);
    if (status != SANE_STATUS_GOOD) {
        NS_ERROR("Error trying to get image parameters!\n");
        sane_cancel(pthis->mSaneHandle);
        goto error_exit;
    }
    cinfo.image_width = param.pixels_per_line;
    cinfo.image_height = param.lines;

    switch (param.format) {
    case SANE_FRAME_RED:
    case SANE_FRAME_GREEN:
    case SANE_FRAME_BLUE:
        
        NS_ERROR("Multiframe devices not supported!\n");
        sane_cancel(pthis->mSaneHandle);
        goto error_exit;
        break;
                
    case SANE_FRAME_RGB:
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        break;

    case SANE_FRAME_GRAY:
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
        break;
    }
    jpeg_set_defaults(&cinfo); 
    cinfo.dct_method = pthis->mCompMethod; 
    jpeg_set_quality(&cinfo, pthis->mCompQuality, FALSE);
    jpeg_start_compress(&cinfo, TRUE);

    buf = (SANE_Byte *)PR_MALLOC(param.bytes_per_line);
    if (!buf) {
        NS_ERROR("Error trying to allocate buffer!\n");
        jpeg_destroy_compress(&cinfo);
        fclose(out);
        sane_cancel(pthis->mSaneHandle);
        goto error_exit;
    }

    int total;
    while(1) {
        total = 0;
        while (total < param.bytes_per_line) {
            
            status = sane_read(pthis->mSaneHandle, buf + total, 
                               param.bytes_per_line - total, 
                               &len);
            if (status != SANE_STATUS_GOOD) {
                if (status == SANE_STATUS_EOF)
                    
                    goto finished_scan;
                else {
                    NS_ERROR("Error trying to read from sane device!\n");
                    PR_FREEIF(buf);
                    jpeg_destroy_compress(&cinfo);
                    fclose(out);
                    sane_cancel(pthis->mSaneHandle);
                    goto error_exit;
                }
            }
            total += len;
        }

        row_pointer[0] = (JSAMPLE *) buf;
        jpeg_write_scanlines(&cinfo, row_pointer, 1); 

    } 

    
 finished_scan:
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    PR_FREEIF(buf);
    sane_cancel(pthis->mSaneHandle);
    fclose(out);
    pthis->SetSuccess(PR_TRUE); 

 error_exit:

    
    

    nsCOMPtr<nsIEventQueue> eventQ;

    
    nsCOMPtr<nsIEventQueueService> eventQService = 
             do_GetService(kEventQueueService, &rv);
    if (NS_FAILED(rv)) {
        NS_ERROR("Unable to get event queue service!\n");
        return;
    }

    rv = eventQService->GetThreadEventQueue(pthis->mUIThread,
                                            getter_AddRefs(eventQ));
    if (NS_FAILED(rv)) {
        NS_ERROR("Unable to get thread queue!\n");
        return;
    }

    PLEvent *event = new PLEvent;
    if (!event) {
        NS_ERROR("Unable to create new event!\n");
        return;
    }

    
    PL_InitEvent(event, pthis, ThreadedHandleEvent, ThreadedDestroyEvent);

    if (NS_FAILED(eventQ->PostEvent(event))) {
        NS_ERROR("Error trying to post event!\n");
        PL_DestroyEvent(event);
        return;
    }
}

void* PR_CALLBACK ThreadedHandleEvent(PLEvent * event)
{
    nsSanePluginInstance *pthis = (nsSanePluginInstance *)event->owner;    
   
    
    if (PR_FALSE == pthis->IsUIThread()) {
        NS_ERROR("Attempt to execute event from the wrong thread!\n");
        return;
    }

    pthis->SetState(0); 
    pthis->Restore();  

    
    pthis->DoScanCompleteCallback();

    return nsnull;
}

void PR_CALLBACK ThreadedDestroyEvent(PLEvent* aEvent)
{
    delete aEvent;
}

void store_filename(GtkFileSelection *selector, gpointer user_data) 
{
    nsSanePluginInstance *pthis = (nsSanePluginInstance *) user_data;    
    gchar *cmd;
    GtkWidget * fs;
    char * orig_filename;
    char * dest_filename;

    if (!user_data || !selector)
        return;

    fs = pthis->GetFileSelection();
    orig_filename = pthis->GetImageFilename();
    if ( !fs || !orig_filename )
        return;

    dest_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs));
    if ( !dest_filename )
        return;

#ifdef DEBUG
    printf("Saving %s to %s\n", orig_filename, dest_filename);
#endif

    





    
    cmd = (gchar *)PR_Malloc(PL_strlen(orig_filename) + 
                             PL_strlen(dest_filename) + 6);
    if (!cmd)
        return;
        
    sprintf(cmd, "cp %s %s", orig_filename, dest_filename);
    system(cmd);

    PR_FREEIF(cmd);
}






















