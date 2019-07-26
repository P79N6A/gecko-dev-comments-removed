




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.util.ThreadUtils;

import android.util.Log;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;











public class GLController {
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    private static final String LOGTAG = "GeckoGLController";

    private static GLController sInstance;

    private LayerView mView;
    private boolean mSurfaceValid;
    private int mWidth, mHeight;

    

    private volatile boolean mCompositorCreated;

    private EGL10 mEGL;
    private EGLDisplay mEGLDisplay;
    private EGLConfig mEGLConfig;
    private EGLSurface mEGLSurface;

    private static final int LOCAL_EGL_OPENGL_ES2_BIT = 4;

    private static final int[] CONFIG_SPEC = {
        EGL10.EGL_RED_SIZE, 5,
        EGL10.EGL_GREEN_SIZE, 6,
        EGL10.EGL_BLUE_SIZE, 5,
        EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT,
        EGL10.EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
        EGL10.EGL_NONE
    };

    private GLController() {
        
        
        
        
        GfxInfoThread.startThread();
    }

    static GLController getInstance(LayerView view) {
        if (sInstance == null) {
            sInstance = new GLController();
        }
        sInstance.mView = view;
        return sInstance;
    }

    synchronized void surfaceDestroyed() {
        ThreadUtils.assertOnUiThread();

        mSurfaceValid = false;
        mEGLSurface = null;

        
        
        
        
        
        
        
        
        if (mCompositorCreated) {
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createCompositorPauseEvent());
        }
    }

    synchronized void surfaceChanged(int newWidth, int newHeight) {
        ThreadUtils.assertOnUiThread();

        mWidth = newWidth;
        mHeight = newHeight;

        if (mSurfaceValid) {
            
            
            
            resumeCompositor(mWidth, mHeight);
            return;
        }
        mSurfaceValid = true;

        
        
        
        
        
        
        
        
        

        mView.post(new Runnable() {
            @Override
            public void run() {
                
                
                
                
                if (!mCompositorCreated && !GfxInfoThread.hasData()) {
                    mView.postDelayed(this, 1);
                    return;
                }

                try {
                    
                    
                    
                    
                    
                    
                    if (mSurfaceValid) {
                        if (mEGL == null) {
                            initEGL();
                        }

                        mEGLSurface = mEGL.eglCreateWindowSurface(mEGLDisplay, mEGLConfig, mView.getNativeWindow(), null);
                    }
                } catch (Exception e) {
                    Log.e(LOGTAG, "Unable to create window surface", e);
                }
                if (mEGLSurface == null || mEGLSurface == EGL10.EGL_NO_SURFACE) {
                    mSurfaceValid = false;
                    mEGLSurface = null; 
                    Log.e(LOGTAG, "EGL window surface could not be created: " + getEGLError());
                    return;
                }
                
                
                createCompositor();
            }
        });
    }

    void createCompositor() {
        ThreadUtils.assertOnUiThread();

        if (mCompositorCreated) {
            
            
            
            resumeCompositor(mWidth, mHeight);
            return;
        }

        
        
        
        
        if (mEGLSurface != null && GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createCompositorResumeEvent());
        }
    }

    void compositorCreated() {
        
        
        mCompositorCreated = true;
    }

    public boolean hasValidSurface() {
        return mSurfaceValid;
    }

    private void initEGL() {
        mEGL = (EGL10)EGLContext.getEGL();

        mEGLDisplay = mEGL.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        if (mEGLDisplay == EGL10.EGL_NO_DISPLAY) {
            throw new GLControllerException("eglGetDisplay() failed");
        }

        mEGLConfig = chooseConfig();
    }

    private EGLConfig chooseConfig() {
        int[] numConfigs = new int[1];
        if (!mEGL.eglChooseConfig(mEGLDisplay, CONFIG_SPEC, null, 0, numConfigs) ||
                numConfigs[0] <= 0) {
            throw new GLControllerException("No available EGL configurations " +
                                            getEGLError());
        }

        EGLConfig[] configs = new EGLConfig[numConfigs[0]];
        if (!mEGL.eglChooseConfig(mEGLDisplay, CONFIG_SPEC, configs, numConfigs[0], numConfigs)) {
            throw new GLControllerException("No EGL configuration for that specification " +
                                            getEGLError());
        }

        
        int[] red = new int[1], green = new int[1], blue = new int[1];
        for (EGLConfig config : configs) {
            mEGL.eglGetConfigAttrib(mEGLDisplay, config, EGL10.EGL_RED_SIZE, red);
            mEGL.eglGetConfigAttrib(mEGLDisplay, config, EGL10.EGL_GREEN_SIZE, green);
            mEGL.eglGetConfigAttrib(mEGLDisplay, config, EGL10.EGL_BLUE_SIZE, blue);
            if (red[0] == 5 && green[0] == 6 && blue[0] == 5) {
                return config;
            }
        }

        throw new GLControllerException("No suitable EGL configuration found");
    }

    
    private EGLSurface provideEGLSurface() {
        return mEGLSurface;
    }

    private String getEGLError() {
        return "Error " + mEGL.eglGetError();
    }

    void resumeCompositor(int width, int height) {
        
        
        
        
        
        
        if (mCompositorCreated) {
            GeckoAppShell.scheduleResumeComposition(width, height);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createCompositorResumeEvent());
        }
    }

    public static class GLControllerException extends RuntimeException {
        public static final long serialVersionUID = 1L;

        GLControllerException(String e) {
            super(e);
        }
    }
}
