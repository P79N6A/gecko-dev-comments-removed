









package org.webrtc.videoengine;

import java.util.concurrent.locks.ReentrantLock;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.opengl.GLSurfaceView;
import android.util.Log;

public class ViEAndroidGLES20 extends GLSurfaceView
        implements GLSurfaceView.Renderer {
    
    private boolean surfaceCreated = false;
    private boolean openGLCreated = false;
    
    private boolean nativeFunctionsRegisted = false;
    private ReentrantLock nativeFunctionLock = new ReentrantLock();
    
    private long nativeObject = 0;
    private int viewWidth = 0;
    private int viewHeight = 0;

    public static boolean UseOpenGL2(Object renderWindow) {
        return ViEAndroidGLES20.class.isInstance(renderWindow);
    }

    public ViEAndroidGLES20(Context context) {
        super(context);

        
        
        setEGLContextFactory(new ContextFactory());

        
        
        
        
        setEGLConfigChooser( new ConfigChooser(5, 6, 5, 0, 0, 0) );

        this.setRenderer(this);
        this.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
    }

    
    
    public static boolean IsSupported(Context context) {
        ActivityManager am =
                (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        ConfigurationInfo info = am.getDeviceConfigurationInfo();
        if(info.reqGlEsVersion >= 0x20000) {
            
            return true;
        }
        return false;
    }

    public void onDrawFrame(GL10 gl) {
        nativeFunctionLock.lock();
        if(!nativeFunctionsRegisted || !surfaceCreated) {
            nativeFunctionLock.unlock();
            return;
        }

        if(!openGLCreated) {
            if(0 != CreateOpenGLNative(nativeObject, viewWidth, viewHeight)) {
                return; 
            }
            openGLCreated = true; 
        }
        DrawNative(nativeObject); 
        nativeFunctionLock.unlock();
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        surfaceCreated = true;
        viewWidth = width;
        viewHeight = height;

        nativeFunctionLock.lock();
        if(nativeFunctionsRegisted) {
            if(CreateOpenGLNative(nativeObject,width,height) == 0)
                openGLCreated = true;
        }
        nativeFunctionLock.unlock();
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    }

    public void RegisterNativeObject(long nativeObject) {
        nativeFunctionLock.lock();
        nativeObject = nativeObject;
        nativeFunctionsRegisted = true;
        nativeFunctionLock.unlock();
    }

    public void DeRegisterNativeObject() {
        nativeFunctionLock.lock();
        nativeFunctionsRegisted = false;
        openGLCreated = false;
        nativeObject = 0;
        nativeFunctionLock.unlock();
    }

    public void ReDraw() {
        if(surfaceCreated) {
            
            this.requestRender();
        }
    }

    
    
    
    
    private static class ContextFactory
            implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        public EGLContext createContext(EGL10 egl,
                EGLDisplay display,
                EGLConfig eglConfig) {
            
            int[] attrib_list = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
            
            EGLContext context = egl.eglCreateContext(display,
                    eglConfig,
                    EGL10.EGL_NO_CONTEXT,
                    attrib_list);
            checkEglError("ContextFactory eglCreateContext", egl);
            return context;
        }

        public void destroyContext(EGL10 egl, EGLDisplay display,
                EGLContext context) {
            egl.eglDestroyContext(display, context);
        }
    }

    private static void checkEglError(String prompt, EGL10 egl) {
        int error;
        while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
            Log.e("*WEBRTC*", String.format("%s: EGL error: 0x%x", prompt, error));
        }
    }

    
    private static class ConfigChooser
            implements GLSurfaceView.EGLConfigChooser {

        public ConfigChooser(int r, int g, int b, int a, int depth, int stencil) {
            mRedSize = r;
            mGreenSize = g;
            mBlueSize = b;
            mAlphaSize = a;
            mDepthSize = depth;
            mStencilSize = stencil;
        }

        
        
        
        private static int EGL_OPENGL_ES2_BIT = 4;
        private static int[] s_configAttribs2 =
        {
            EGL10.EGL_RED_SIZE, 4,
            EGL10.EGL_GREEN_SIZE, 4,
            EGL10.EGL_BLUE_SIZE, 4,
            EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL10.EGL_NONE
        };

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {

            
            int[] num_config = new int[1];
            egl.eglChooseConfig(display, s_configAttribs2, null, 0, num_config);

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                throw new IllegalArgumentException("No configs match configSpec");
            }

            
            EGLConfig[] configs = new EGLConfig[numConfigs];
            egl.eglChooseConfig(display, s_configAttribs2, configs,
                    numConfigs, num_config);

            
            return chooseConfig(egl, display, configs);
        }

        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display,
                EGLConfig[] configs) {
            for(EGLConfig config : configs) {
                int d = findConfigAttrib(egl, display, config,
                        EGL10.EGL_DEPTH_SIZE, 0);
                int s = findConfigAttrib(egl, display, config,
                        EGL10.EGL_STENCIL_SIZE, 0);

                
                if (d < mDepthSize || s < mStencilSize)
                    continue;

                
                int r = findConfigAttrib(egl, display, config,
                        EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib(egl, display, config,
                        EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib(egl, display, config,
                        EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib(egl, display, config,
                        EGL10.EGL_ALPHA_SIZE, 0);

                if (r == mRedSize && g == mGreenSize &&
                        b == mBlueSize && a == mAlphaSize)
                    return config;
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display,
                EGLConfig config, int attribute,
                int defaultValue) {

            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }

        
        protected int mRedSize;
        protected int mGreenSize;
        protected int mBlueSize;
        protected int mAlphaSize;
        protected int mDepthSize;
        protected int mStencilSize;
        private int[] mValue = new int[1];
    }

    private native int CreateOpenGLNative(long nativeObject,
            int width, int height);
    private native void DrawNative(long nativeObject);

}
