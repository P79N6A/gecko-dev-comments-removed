









































#ifndef __NS_SANE_PLUGIN_H__
#define __NS_SANE_PLUGIN_H__

#include "prthread.h"
#include "nscore.h"
#include "nsplugin.h"
#include "gdksuperwin.h"
#include "gtkmozbox.h"
#include "gtk/gtk.h"
#include "nsIPlugin.h"
#include "nsSanePluginControl.h"
#include "nsIScriptContext.h"
#include "nsIServiceManager.h"
#include "nsISupports.h"
#include "nsISupportsUtils.h"
#include "nsIEventQueueService.h"
#include "nsIEventQueue.h"



extern "C"
{
#include <jpeglib.h>
}



extern "C"
{
#include <sane/sane.h>
#include <sane/sanei.h>
#include <sane/saneopts.h>
}

typedef struct _PlatformInstance
{
    Window        window;
    GtkWidget   * widget;
    GdkSuperWin * superwin;
    Display     * display;
    uint16        x;
    uint16        y;
    uint32        width; 
    uint32        height;

} PlatformInstance;





void PR_CALLBACK scanimage_thread_routine( void * arg );

class nsSanePluginInstance : public nsIPluginInstance, 
                             public nsISanePluginInstance
{
    friend void PR_CALLBACK scanimage_thread_routine( void *);

    public:
  
    nsSanePluginInstance(void);
    virtual ~nsSanePluginInstance(void);

    
    
    NS_IMETHOD Initialize(nsIPluginInstancePeer* peer);
    NS_IMETHOD GetPeer(nsIPluginInstancePeer* *result);
    NS_IMETHOD Start(void);
    NS_IMETHOD Stop(void);
    NS_IMETHOD Destroy( void );
    NS_IMETHOD SetWindow( nsPluginWindow* window );
    NS_IMETHOD NewStream( nsIPluginStreamListener** listener );
    NS_IMETHOD Print( nsPluginPrint* platformPrint );
    NS_IMETHOD GetValue( nsPluginInstanceVariable variable, void *value );

    
    NS_IMETHOD HandleEvent( nsPluginEvent* event, PRBool* handled );

    
    

    
    
    NS_DECL_ISUPPORTS ;
    
    
    NS_IMETHOD DoScanCompleteCallback();
    NS_IMETHOD DoInitCompleteCallback();

    void SetMode(nsPluginMode mode) { fMode = mode; }
    void SetState(PRInt32 aState) { mState = aState; };
    NS_IMETHOD PaintImage(void);
    char * GetImageFilename();
    GtkWidget * GetFileSelection();
    PRBool IsUIThread();
    nsresult OpenSaneDeviceIF( void );

    
    NS_DECL_NSISANEPLUGININSTANCE

private:

    GtkWidget                  *mDrawing_area;
    GtkWidget                  *mEvent_box;
    PlatformInstance            fPlatform;
    char                        mImageFilename[255];
    GtkWidget                  *mFileSelection;
    GdkRectangle                mZoom_box;
    unsigned char              *mRGBData;
    int                         mRGBWidth, mRGBHeight;

    
    PRInt32                     mLineWidth;
    GdkLineStyle                mLineStyle;
    GdkCapStyle                 mCapStyle;
    GdkJoinStyle                mJoinStyle;

    
    float                       mTopLeftXChange;
    float                       mTopLeftYChange;
    float                       mBottomRightXChange;
    float                       mBottomRightYChange;

    
    int                         mCompQuality;
    enum J_DCT_METHOD           mCompMethod;

    
    SANE_Handle                 mSaneHandle;
    SANE_String                 mSaneDevice;
    SANE_Bool                   mSaneOpen;
    PRBool                      mSuccess;
    PRInt32                     mState;

    
    char                        *mOnScanCompleteScript;
    char                        *mOnInitCompleteScript;
    PRThread                    *mScanThread;
    PRThread                    *mUIThread;

protected:

    nsIPluginInstancePeer*      fPeer;
    nsPluginWindow*             fWindow;
    nsPluginMode                fMode;
    nsIPluginManager*           mPluginManager;

private:

    int                         WritePNMHeader (int fd, SANE_Frame format, 
                                                int width, int height, 
                                                int depth);

    void                        PlatformNew( void );
    nsresult                    PlatformDestroy( void );
    PRInt16                     PlatformHandleEvent( nsPluginEvent* event );
    nsresult                    PlatformSetWindow( nsPluginWindow* window );  
};

class nsSanePluginStreamListener : public nsIPluginStreamListener
{
    public:
  
    NS_DECL_ISUPPORTS ;
  
    






    NS_IMETHOD OnStartBinding( nsIPluginStreamInfo* pluginInfo );
  
    










    NS_IMETHOD OnDataAvailable( nsIPluginStreamInfo* pluginInfo,
                                nsIInputStream* input, 
                                PRUint32 length );
    NS_IMETHOD OnFileAvailable( nsIPluginStreamInfo* pluginInfo,
                         const char* fileName );
  
    












    NS_IMETHOD OnStopBinding( nsIPluginStreamInfo* pluginInfo,
                              nsresult status );
    NS_IMETHOD OnNotify( const char* url, nsresult status );
    NS_IMETHOD GetStreamType( nsPluginStreamType *result );
  
    
    
  
    nsSanePluginStreamListener( nsSanePluginInstance* inst );
    virtual ~nsSanePluginStreamListener( void );
  
    nsSanePluginInstance* mPlugInst;
};

#endif 
