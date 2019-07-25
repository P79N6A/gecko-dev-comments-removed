




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.InputConnectionHandler;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.util.Log;
import java.nio.IntBuffer;
import java.util.LinkedList;







public class LayerView extends GLSurfaceView {
    private Context mContext;
    private LayerController mController;
    private InputConnectionHandler mInputConnectionHandler;
    private LayerRenderer mRenderer;
    private GestureDetector mGestureDetector;
    private SimpleScaleGestureDetector mScaleGestureDetector;
    private long mRenderTime;
    private boolean mRenderTimeReset;
    private static String LOGTAG = "GeckoLayerView";
    
    private LinkedList<MotionEvent> mEventQueue = new LinkedList<MotionEvent>();


    public LayerView(Context context, LayerController controller) {
        super(context);

        mContext = context;
        mController = controller;
        mRenderer = new LayerRenderer(this);
        setRenderer(mRenderer);
        setRenderMode(RENDERMODE_WHEN_DIRTY);
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

    public void setInputConnectionHandler(InputConnectionHandler handler) {
        mInputConnectionHandler = handler;
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

    @Override
    public void requestRender() {
        super.requestRender();

        synchronized(this) {
            if (!mRenderTimeReset) {
                mRenderTimeReset = true;
                mRenderTime = System.nanoTime();
            }
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
}

