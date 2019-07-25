





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoInputConnection;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.InputConnectionHandler;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.View;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.ScaleGestureDetector;
import android.widget.RelativeLayout;
import android.util.Log;
import java.nio.IntBuffer;
import java.util.LinkedList;

import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import javax.microedition.khronos.opengles.GL10;










public class LayerView extends SurfaceView implements SurfaceHolder.Callback {
    private Context mContext;
    private LayerController mController;
    private GLController mGLController;
    private InputConnectionHandler mInputConnectionHandler;
    private LayerRenderer mRenderer;
    private GestureDetector mGestureDetector;
    private SimpleScaleGestureDetector mScaleGestureDetector;
    private long mRenderTime;
    private boolean mRenderTimeReset;
    private static String LOGTAG = "GeckoLayerView";
    
    private LinkedList<MotionEvent> mEventQueue = new LinkedList<MotionEvent>();
    
    private int mPaintState = PAINT_NONE;

    private Listener mListener;

    

    public static final int PAINT_NONE = 0;
    public static final int PAINT_BEFORE_FIRST = 1;
    public static final int PAINT_AFTER_FIRST = 2;


    public LayerView(Context context, LayerController controller) {
        super(context);

        SurfaceHolder holder = getHolder();
        holder.addCallback(this);
        holder.setFormat(PixelFormat.RGB_565);

        mGLController = new GLController(this);
        mContext = context;
        mController = controller;
        mRenderer = new LayerRenderer(this);
        mGestureDetector = new GestureDetector(context, controller.getGestureListener());
        mScaleGestureDetector =
            new SimpleScaleGestureDetector(controller.getScaleGestureListener());
        mGestureDetector.setOnDoubleTapListener(controller.getDoubleTapListener());
        mInputConnectionHandler = null;

        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    private void addToEventQueue(MotionEvent event) {
        MotionEvent copy = MotionEvent.obtain(event);
        mEventQueue.add(copy);
    }

    public void processEventQueue() {
        MotionEvent event = mEventQueue.poll();
        while(event != null) {
            processEvent(event);
            event = mEventQueue.poll();
        }
    }

    public void clearEventQueue() {
        mEventQueue.clear();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mController.onTouchEvent(event)) {
            addToEventQueue(event);
            return true;
        }
        return processEvent(event);
    }

    private boolean processEvent(MotionEvent event) {
        if (mGestureDetector.onTouchEvent(event))
            return true;
        mScaleGestureDetector.onTouchEvent(event);
        if (mScaleGestureDetector.isInProgress())
            return true;
        mController.getPanZoomController().onTouchEvent(event);
        return true;
    }

    public LayerController getController() { return mController; }

    
    public void setViewportSize(IntSize size) {
        mController.setViewportSize(new FloatSize(size));
    }

    public GeckoInputConnection setInputConnectionHandler() {
        GeckoInputConnection geckoInputConnection = GeckoInputConnection.create(this);
        mInputConnectionHandler = geckoInputConnection;
        return geckoInputConnection;
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onCreateInputConnection(outAttrs);
        return null;
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyPreIme(keyCode, event);
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyDown(keyCode, event);
        return false;
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyLongPress(keyCode, event);
        return false;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyMultiple(keyCode, repeatCount, event);
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onKeyUp(keyCode, event);
        return false;
    }

    public synchronized void requestRender() {
        if (mListener != null) {
            mListener.renderRequested();
        }
    }

    public void addLayer(Layer layer) {
        mRenderer.addLayer(layer);
    }

    public void removeLayer(Layer layer) {
        mRenderer.removeLayer(layer);
    }

    



    public long getRenderTime() {
        synchronized(this) {
            mRenderTimeReset = false;
            return System.nanoTime() - mRenderTime;
        }
    }

    public int getMaxTextureSize() {
        return mRenderer.getMaxTextureSize();
    }

    
    public IntBuffer getPixels() {
        return mRenderer.getPixels();
    }

    public void setLayerRenderer(LayerRenderer renderer) {
        mRenderer = renderer;
    }

    public LayerRenderer getLayerRenderer() {
        return mRenderer;
    }

    

    public void setPaintState(int paintState) {
        if (paintState > mPaintState) {
            Log.d(LOGTAG, "LayerView paint state set to " + paintState);
            mPaintState = paintState;
        }
    }

    public int getPaintState() {
        return mPaintState;
    }


    public GLSurfaceView.Renderer getRenderer() {
        return mRenderer;
    }

    public void setListener(Listener listener) {
        mListener = listener;
    }

    public synchronized GLController getGLController() {
        return mGLController;
    }

    
    public synchronized void surfaceChanged(SurfaceHolder holder, int format, int width,
                                            int height) {
        mGLController.sizeChanged(width, height);

        if (mListener != null) {
            mListener.surfaceChanged(width, height);
        }
    }

    
    public synchronized void surfaceCreated(SurfaceHolder holder) {
        mGLController.surfaceCreated();
    }

    
    public synchronized void surfaceDestroyed(SurfaceHolder holder) {
        mGLController.surfaceDestroyed();

        if (mListener != null) {
            mListener.compositionPauseRequested();
        }
    }

    
    public static GLController registerCxxCompositor() {
        try {
            LayerView layerView = GeckoApp.mAppContext.getLayerController().getView();
            return layerView.getGLController();
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


}
