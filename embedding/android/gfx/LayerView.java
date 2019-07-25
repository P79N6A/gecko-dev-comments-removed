




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.InputConnectionHandler;
import org.mozilla.fennec.gfx.LayerController;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.view.GestureDetector;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.ScaleGestureDetector;







public class LayerView extends GLSurfaceView {
    private Context mContext;
    private LayerController mController;
    private InputConnectionHandler mInputConnectionHandler;
    private LayerRenderer mRenderer;
    private GestureDetector mGestureDetector;
    private ScaleGestureDetector mScaleGestureDetector;

    public LayerView(Context context, LayerController controller) {
        super(context);

        mContext = context;
        mController = controller;
        mRenderer = new LayerRenderer(this);
        setRenderer(mRenderer);
        mGestureDetector = new GestureDetector(context, controller.getGestureListener());
        mScaleGestureDetector = new ScaleGestureDetector(context, controller.getScaleGestureListener());
        mInputConnectionHandler = null;

        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mGestureDetector.onTouchEvent(event))
            return true;
        mScaleGestureDetector.onTouchEvent(event);
        if (mScaleGestureDetector.isInProgress())
            return true;
        return mController.onTouchEvent(event);
    }

    public LayerController getController() { return mController; }
    public void geometryChanged() {  }

    public void notifyRendererOfPageSizeChange() {
        mRenderer.pageSizeChanged();
    }

    
    public void setScreenSize(int width, int height) {
        mController.setScreenSize(width, height);
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
}

