




package org.mozilla.gecko.gfx;

import android.opengl.GLES20;
import android.util.Log;

import java.util.concurrent.SynchronousQueue;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;

public class GfxInfoThread extends Thread {

    private static final String LOGTAG = "GfxInfoThread";

    private SynchronousQueue<String> mDataQueue;

    public GfxInfoThread() {
        mDataQueue = new SynchronousQueue<String>();
    }

    private void error(String msg) {
        Log.e(LOGTAG, msg);
        try {
            mDataQueue.put("ERROR\n" + msg + "\n");
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    private void eglError(EGL10 egl, String msg) {
        error(msg + " (EGL error " + Integer.toHexString(egl.eglGetError()) + ")");
    }

    public String getData() {
        String data = mDataQueue.poll();
        if (data != null)
            return data;

        error("We need the GfxInfo data, but it is not yet available. " +
              "We have to wait for it, so expect abnormally long startup times. " +
              "Please report a Mozilla bug.");
        try {
            data = mDataQueue.take();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        Log.i(LOGTAG, "GfxInfo data is finally available.");
        return data;
    }

    @Override
    public void run() {
        
        EGL10 egl = (EGL10) EGLContext.getEGL();
        EGLDisplay eglDisplay = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

        if (eglDisplay == EGL10.EGL_NO_DISPLAY) {
            eglError(egl, "eglGetDisplay failed");
            return;
        }

        int[] returnedVersion = new int[2];
        if (!egl.eglInitialize(eglDisplay, returnedVersion)) {
            eglError(egl, "eglInitialize failed");
            return;
        }

        
        int[] returnedNumberOfConfigs = new int[1];
        int EGL_OPENGL_ES2_BIT = 4;
        int[] configAttribs = new int[] {
            EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL10.EGL_NONE
        };

        String noES2SupportMsg = "Maybe this device does not support OpenGL ES2?";

        if (!egl.eglChooseConfig(eglDisplay,
                                 configAttribs,
                                 null,
                                 0,
                                 returnedNumberOfConfigs))
        {
            eglError(egl, "eglChooseConfig failed to query OpenGL ES2 configs. " +
                          noES2SupportMsg);
            return;
        }

        
        int numConfigs = returnedNumberOfConfigs[0];
        if (numConfigs == 0) {
            error("eglChooseConfig returned zero OpenGL ES2 configs. " +
                  noES2SupportMsg);
            return;
        }

        EGLConfig[] returnedConfigs = new EGLConfig[numConfigs];
        if (!egl.eglChooseConfig(eglDisplay,
                                 configAttribs,
                                 returnedConfigs,
                                 numConfigs,
                                 returnedNumberOfConfigs))
        {
            eglError(egl, "eglChooseConfig failed (listing OpenGL ES2 configs). " +
                          noES2SupportMsg);
            return;
        }

        EGLConfig eglConfig = returnedConfigs[0];

        
        int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        int[] contextAttribs = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL10.EGL_NONE };
        EGLContext eglContext = egl.eglCreateContext(eglDisplay,
                                                     eglConfig,
                                                     EGL10.EGL_NO_CONTEXT,
                                                     contextAttribs);
        if (eglContext == EGL10.EGL_NO_CONTEXT) {
            eglError(egl, "eglCreateContext failed to create a OpenGL ES2 context" +
                          noES2SupportMsg);
            return;
        }

        
        
        
        int[] surfaceAttribs = new int[] {
            EGL10.EGL_WIDTH, 16,
            EGL10.EGL_HEIGHT, 16,
            EGL10.EGL_NONE
        };
        EGLSurface eglSurface = egl.eglCreatePbufferSurface(eglDisplay,
                                                            eglConfig,
                                                            surfaceAttribs);
        if (eglSurface == EGL10.EGL_NO_SURFACE) {
            eglError(egl, "eglCreatePbufferSurface failed");
            return;
        }

        
        if (!egl.eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
            eglError(egl, "eglMakeCurrent failed");
            return;
        }

        {
            int error = egl.eglGetError();
            if (error != EGL10.EGL_SUCCESS) {
                error("EGL error " + Integer.toHexString(error));
                return;
            }
        }

        String data =
            "VENDOR\n"   + GLES20.glGetString(GLES20.GL_VENDOR)   + "\n" +
            "RENDERER\n" + GLES20.glGetString(GLES20.GL_RENDERER) + "\n" +
            "VERSION\n"  + GLES20.glGetString(GLES20.GL_VERSION)  + "\n";

        {
            int error = GLES20.glGetError();
            if (error != GLES20.GL_NO_ERROR) {
                error("OpenGL error " + Integer.toHexString(error));
                return;
            }
        }

        
        
        egl.eglMakeCurrent(eglDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
        egl.eglDestroySurface(eglDisplay, eglSurface);
        egl.eglDestroyContext(eglDisplay, eglContext);
        

        
        
        try {
            mDataQueue.put(data);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }
}
