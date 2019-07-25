




































#ifndef AndroidBridge_h__
#define AndroidBridge_h__

#include <jni.h>
#include <android/log.h>

#include "nsCOMPtr.h"
#include "nsIRunnable.h"

#include "AndroidJavaWrappers.h"

#include "nsVoidArray.h"





class nsWindow;

namespace mozilla {

class AndroidBridge
{
public:
    static AndroidBridge *ConstructBridge(JNIEnv *jEnv,
                                          jclass jGeckoAppShellClass);

    static AndroidBridge *Bridge() {
        return sBridge;
    }

    static JavaVM *VM() {
        return sBridge->mJavaVM;
    }

    static JNIEnv *JNI() {
        sBridge->EnsureJNIThread();
        return sBridge->mJNIEnv;
    }

    static JNIEnv *JNIForThread() {
        return sBridge->AttachThread();
    }

    
    
    
    
    
    PRBool SetMainThread(void *thr);

    JNIEnv* AttachThread(PRBool asDaemon = PR_TRUE);

    
    void ShowIME(int aState);

    void EnableAccelerometer(bool aEnable);

    void EnableLocation(bool aEnable);

    void ReturnIMEQueryResult(const PRUnichar *result, PRUint32 len, int selectionStart, int selectionEnd);

    void NotifyXreExit();

    void ScheduleRestart();

    void SetSurfaceView(jobject jobj);
    AndroidGeckoSurfaceView& SurfaceView() { return mSurfaceView; }

    void GetHandlersForMimeType(const char *aMimeType, nsStringArray* aStringArray);

    PRBool OpenUriExternal(nsCString& aUriSpec, nsCString& aMimeType);

    void GetMimeTypeFromExtension(const nsCString& aFileExt, nsCString& aMimeType);

    void MoveTaskToBack();

    struct AutoLocalJNIFrame {
        AutoLocalJNIFrame(int nEntries = 128) : mEntries(nEntries) {
            AndroidBridge::Bridge()->JNI()->PushLocalFrame(mEntries);
        }
        
        
        
        void Purge() {
            AndroidBridge::Bridge()->JNI()->PopLocalFrame(NULL);
            AndroidBridge::Bridge()->JNI()->PushLocalFrame(mEntries);
        }
        ~AutoLocalJNIFrame() {
            AndroidBridge::Bridge()->JNI()->PopLocalFrame(NULL);
        }
        int mEntries;
    };

    
    void *CallEglCreateWindowSurface(void *dpy, void *config, AndroidGeckoSurfaceView& surfaceView);

protected:
    static AndroidBridge *sBridge;

    
    JavaVM *mJavaVM;

    
    JNIEnv *mJNIEnv;
    void *mThread;

    
    AndroidGeckoSurfaceView mSurfaceView;

    
    jclass mGeckoAppShellClass;

    AndroidBridge() { }
    PRBool Init(JNIEnv *jEnv, jclass jGeckoApp);

    void EnsureJNIThread();

    
    jmethodID jShowIME;
    jmethodID jEnableAccelerometer;
    jmethodID jEnableLocation;
    jmethodID jReturnIMEQueryResult;
    jmethodID jNotifyXreExit;
    jmethodID jScheduleRestart;
    jmethodID jGetOutstandingDrawEvents;
    jmethodID jGetHandlersForMimeType;
    jmethodID jOpenUriExternal;
    jmethodID jGetMimeTypeFromExtension;
    jmethodID jMoveTaskToBack;

    
    jclass jEGLSurfaceImplClass;
    jclass jEGLContextImplClass;
    jclass jEGLConfigImplClass;
    jclass jEGLDisplayImplClass;
    jclass jEGLContextClass;
    jclass jEGL10Class;
};

}

extern "C" JNIEnv * GetJNIForThread();
extern PRBool mozilla_AndroidBridge_SetMainThread(void *);

#endif 
