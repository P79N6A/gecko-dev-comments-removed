




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.mozglue.GeneratableAndroidBridgeTarget;
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
    private boolean mServerSurfaceValid;
    private int mWidth, mHeight;

    

    private volatile boolean mCompositorCreated;

    private EGL10 mEGL;
    private EGLDisplay mEGLDisplay;
    private EGLConfig mEGLConfig;
    private EGLSurface mEGLSurfaceForCompositor;

    private static final int LOCAL_EGL_OPENGL_ES2_BIT = 4;

    private static final int[] CONFIG_SPEC_16BPP = {
        EGL10.EGL_RED_SIZE, 5,
        EGL10.EGL_GREEN_SIZE, 6,
        EGL10.EGL_BLUE_SIZE, 5,
        EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT,
        EGL10.EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
        EGL10.EGL_NONE
    };

    private static final int[] CONFIG_SPEC_24BPP = {
        EGL10.EGL_RED_SIZE, 8,
        EGL10.EGL_GREEN_SIZE, 8,
        EGL10.EGL_BLUE_SIZE, 8,
        EGL10.EGL_SURFACE_TYPE, EGL10.EGL_WINDOW_BIT,
        EGL10.EGL_RENDERABLE_TYPE, LOCAL_EGL_OPENGL_ES2_BIT,
        EGL10.EGL_NONE
    };

    private GLController() {
    }

    static GLController getInstance(LayerView view) {
        if (sInstance == null) {
            sInstance = new GLController();
        }
        sInstance.mView = view;
        return sInstance;
    }

    synchronized void serverSurfaceDestroyed() {
        ThreadUtils.assertOnUiThread();
        Log.w(LOGTAG, "GLController::serverSurfaceDestroyed() with mCompositorCreated=" + mCompositorCreated);

        mServerSurfaceValid = false;

        if (mEGLSurfaceForCompositor != null) {
          mEGL.eglDestroySurface(mEGLDisplay, mEGLSurfaceForCompositor);
          mEGLSurfaceForCompositor = null;
        }

        
        
        
        
        
        
        
        
        if (mCompositorCreated) {
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createCompositorPauseEvent());
        }
        Log.w(LOGTAG, "done GLController::serverSurfaceDestroyed()");
    }

    synchronized void serverSurfaceChanged(int newWidth, int newHeight) {
        ThreadUtils.assertOnUiThread();
        Log.w(LOGTAG, "GLController::serverSurfaceChanged(" + newWidth + ", " + newHeight + ")");

        mWidth = newWidth;
        mHeight = newHeight;
        mServerSurfaceValid = true;

        
        
        
        
        
        mView.post(new Runnable() {
            @Override
            public void run() {
                updateCompositor();
            }
        });
    }

    void updateCompositor() {
        ThreadUtils.assertOnUiThread();
        Log.w(LOGTAG, "GLController::updateCompositor with mCompositorCreated=" + mCompositorCreated);

        if (mCompositorCreated) {
            
            
            
            resumeCompositor(mWidth, mHeight);
            Log.w(LOGTAG, "done GLController::updateCompositor with compositor resume");
            return;
        }

        if (!AttemptPreallocateEGLSurfaceForCompositor()) {
            return;
        }

        
        
        
        
        if (GeckoThread.checkLaunchState(GeckoThread.LaunchState.GeckoRunning)) {
            GeckoAppShell.sendEventToGeckoSync(GeckoEvent.createCompositorCreateEvent(mWidth, mHeight));
        }
        Log.w(LOGTAG, "done GLController::updateCompositor");
    }

    void compositorCreated() {
        Log.w(LOGTAG, "GLController::compositorCreated");
        
        
        mCompositorCreated = true;
    }

    public boolean isServerSurfaceValid() {
        return mServerSurfaceValid;
    }

    public boolean isCompositorCreated() {
        return mCompositorCreated;
    }

    private void initEGL() {
        if (mEGL != null) {
            return;
        }
        mEGL = (EGL10)EGLContext.getEGL();

        mEGLDisplay = mEGL.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
        if (mEGLDisplay == EGL10.EGL_NO_DISPLAY) {
            Log.w(LOGTAG, "Can't get EGL display!");
            return;
        }

        mEGLConfig = chooseConfig();
    }

    private EGLConfig chooseConfig() {
        int[] desiredConfig;
        int rSize, gSize, bSize;
        int[] numConfigs = new int[1];

        switch (GeckoAppShell.getScreenDepth()) {
        case 24:
            desiredConfig = CONFIG_SPEC_24BPP;
            rSize = gSize = bSize = 8;
            break;
        case 16:
        default:
            desiredConfig = CONFIG_SPEC_16BPP;
            rSize = 5; gSize = 6; bSize = 5;
            break;
        }

        if (!mEGL.eglChooseConfig(mEGLDisplay, desiredConfig, null, 0, numConfigs) ||
                numConfigs[0] <= 0) {
            throw new GLControllerException("No available EGL configurations " +
                                            getEGLError());
        }

        EGLConfig[] configs = new EGLConfig[numConfigs[0]];
        if (!mEGL.eglChooseConfig(mEGLDisplay, desiredConfig, configs, numConfigs[0], numConfigs)) {
            throw new GLControllerException("No EGL configuration for that specification " +
                                            getEGLError());
        }

        
        int[] red = new int[1], green = new int[1], blue = new int[1];
        for (EGLConfig config : configs) {
            mEGL.eglGetConfigAttrib(mEGLDisplay, config, EGL10.EGL_RED_SIZE, red);
            mEGL.eglGetConfigAttrib(mEGLDisplay, config, EGL10.EGL_GREEN_SIZE, green);
            mEGL.eglGetConfigAttrib(mEGLDisplay, config, EGL10.EGL_BLUE_SIZE, blue);
            if (red[0] == rSize && green[0] == gSize && blue[0] == bSize) {
                return config;
            }
        }

        throw new GLControllerException("No suitable EGL configuration found");
    }

    private synchronized boolean AttemptPreallocateEGLSurfaceForCompositor() {
        if (mEGLSurfaceForCompositor == null) {
            initEGL();
            mEGLSurfaceForCompositor = mEGL.eglCreateWindowSurface(mEGLDisplay, mEGLConfig, mView.getNativeWindow(), null);
        }
        return mEGLSurfaceForCompositor != null;
    }

    @GeneratableAndroidBridgeTarget(allowMultithread = true, stubName = "CreateEGLSurfaceForCompositorWrapper")
    private synchronized EGLSurface createEGLSurfaceForCompositor() {
        AttemptPreallocateEGLSurfaceForCompositor();
        EGLSurface result = mEGLSurfaceForCompositor;
        mEGLSurfaceForCompositor = null;
        return result;
     }

    private String getEGLError() {
        return "Error " + (mEGL == null ? "(no mEGL)" : mEGL.eglGetError());
    }

    void resumeCompositor(int width, int height) {
        Log.w(LOGTAG, "GLController::resumeCompositor(" + width + ", " + height + ") and mCompositorCreated=" + mCompositorCreated);
        
        
        
        
        
        
        if (mCompositorCreated) {
            GeckoAppShell.scheduleResumeComposition(width, height);
            GeckoAppShell.sendEventToGecko(GeckoEvent.createCompositorResumeEvent());
        }
        Log.w(LOGTAG, "done GLController::resumeCompositor");
    }

    public static class GLControllerException extends RuntimeException {
        public static final long serialVersionUID = 1L;

        GLControllerException(String e) {
            super(e);
        }
    }
}
