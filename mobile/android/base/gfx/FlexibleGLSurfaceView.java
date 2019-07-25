




































package org.mozilla.gecko.gfx;

import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FlexibleGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private static final String LOGTAG = "GeckoFlexibleGLSurfaceView";

    private GLSurfaceView.Renderer mRenderer;
    private GLThread mGLThread; 
    private GLController mController;

    public FlexibleGLSurfaceView(Context context) {
        super(context);
        init();
    }

    public FlexibleGLSurfaceView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        init();
    }

    public void init() {
        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        holder.setFormat(PixelFormat.RGB_565);

        mController = new GLController(this);
    }

    public void setRenderer(GLSurfaceView.Renderer renderer) {
        mRenderer = renderer;
    }

    public GLSurfaceView.Renderer getRenderer() {
        return mRenderer;
    }

    public synchronized void requestRender() {
        if (mGLThread != null) {
            mGLThread.renderFrame();
        }
    }

    



    public synchronized void createGLThread() {
        if (mGLThread != null) {
            throw new FlexibleGLSurfaceViewException("createGLThread() called with a GL thread " +
                                                     "already in place!");
        }

        mGLThread = new GLThread(mController);
        mGLThread.start();
    }

    



    public synchronized Thread destroyGLThread() {
        if (mGLThread == null) {
            throw new FlexibleGLSurfaceViewException("destroyGLThread() called with no GL " +
                                                     "thread active!");
        }

        Thread glThread = mGLThread;
        mGLThread.shutdown();
        mGLThread = null;
        return glThread;
    }

    public synchronized void recreateSurface() {
        if (mGLThread == null) {
            throw new FlexibleGLSurfaceViewException("recreateSurface() called with no GL " +
                                                     "thread active!");
        }

        mGLThread.recreateSurface();
    }

    public synchronized GLController getGLController() {
        if (mGLThread != null) {
            throw new FlexibleGLSurfaceViewException("getGLController() called with a GL thread " +
                                                     "active; shut down the GL thread first!");
        }

        return mController;
    }

    public synchronized void surfaceChanged(SurfaceHolder holder, int format, int width,
                                            int height) {
        mController.sizeChanged(width, height);
        if (mGLThread != null) {
            mGLThread.surfaceChanged(width, height);
        }
    }

    public synchronized void surfaceCreated(SurfaceHolder holder) {
        mController.surfaceCreated();
        if (mGLThread != null) {
            mGLThread.surfaceCreated();
        }
    }

    public synchronized void surfaceDestroyed(SurfaceHolder holder) {
        mController.surfaceDestroyed();
        if (mGLThread != null) {
            mGLThread.surfaceDestroyed();
        }
    }

    public static class FlexibleGLSurfaceViewException extends RuntimeException {
        public static final long serialVersionUID = 1L;

        FlexibleGLSurfaceViewException(String e) {
            super(e);
        }
    }
}

