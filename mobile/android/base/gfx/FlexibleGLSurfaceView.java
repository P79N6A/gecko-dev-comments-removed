




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import javax.microedition.khronos.opengles.GL10;













public class FlexibleGLSurfaceView extends SurfaceView implements SurfaceHolder.Callback {
    private static final String LOGTAG = "GeckoFlexibleGLSurfaceView";

    private GLSurfaceView.Renderer mRenderer;
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
        if (mListener != null) {
            mListener.renderRequested();
        }
    }

    public synchronized GLController getGLController() {
        return mController;
    }

    
    public synchronized void surfaceChanged(SurfaceHolder holder, int format, int width,
                                            int height) {
        mController.sizeChanged(width, height);

        if (mListener != null) {
            mListener.surfaceChanged(width, height);
        }
    }

    
    public synchronized void surfaceCreated(SurfaceHolder holder) {
        mController.surfaceCreated();
    }

    
    public synchronized void surfaceDestroyed(SurfaceHolder holder) {
        mController.surfaceDestroyed();
        if (mListener != null) {
            mListener.compositionPauseRequested();
        }
    }

    
    public static GLController registerCxxCompositor() {
        try {
            FlexibleGLSurfaceView flexView = (FlexibleGLSurfaceView)GeckoApp.mAppContext.getLayerController().getView();
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
        void surfaceChanged(int width, int height);
    }

    public static class FlexibleGLSurfaceViewException extends RuntimeException {
        public static final long serialVersionUID = 1L;

        FlexibleGLSurfaceViewException(String e) {
            super(e);
        }
    }
}

