




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FlexibleGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private static final String LOGTAG = "GeckoFlexibleGLSurfaceView";

    private GLSurfaceView.Renderer mRenderer;
    private GLThread mGLThread; 
    private GLController mController;
    private Listener mListener;

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

    public void setListener(Listener listener) {
        mListener = listener;
    }

    public synchronized void requestRender() {
        if (mGLThread != null) {
            mGLThread.renderFrame();
        }
        if (mListener != null) {
            mListener.renderRequested();
        }
    }

    



    public synchronized void createGLThread() {
        if (mGLThread != null) {
            throw new FlexibleGLSurfaceViewException("createGLThread() called with a GL thread " +
                                                     "already in place!");
        }

        Log.e(LOGTAG, "### Creating GL thread!");
        mGLThread = new GLThread(mController);
        mGLThread.start();
        notifyAll();
    }

    



    public synchronized Thread destroyGLThread() {
        
        Log.e(LOGTAG, "### Waiting for GL thread to be created...");
        while (mGLThread == null) {
            try {
                wait();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        Log.e(LOGTAG, "### Destroying GL thread!");
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
        
        if (mListener != null) {
            mListener.compositionResumeRequested();
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
        
        if (mListener != null) {
            mListener.compositionPauseRequested();
        }
    }

    
    public static GLController registerCxxCompositor() {
        try {
            Log.e(LOGTAG, "### registerCxxCompositor point A");
            System.out.println("register layer comp");
            Log.e(LOGTAG, "### registerCxxCompositor point B");
            FlexibleGLSurfaceView flexView = (FlexibleGLSurfaceView)GeckoApp.mAppContext.getLayerController().getView();
            Log.e(LOGTAG, "### registerCxxCompositor point C: " + flexView);
            try {
                flexView.destroyGLThread().join();
            } catch (InterruptedException e) {}
            Log.e(LOGTAG, "### registerCxxCompositor point D: " + flexView.getGLController());
            return flexView.getGLController();
        } catch (Exception e) {
            Log.e(LOGTAG, "### Exception! " + e);
            return null;
        }
    }

    public interface Listener {
        void renderRequested();
        void compositionPauseRequested();
        void compositionResumeRequested();
    }

    public static class FlexibleGLSurfaceViewException extends RuntimeException {
        public static final long serialVersionUID = 1L;

        FlexibleGLSurfaceViewException(String e) {
            super(e);
        }
    }
}

