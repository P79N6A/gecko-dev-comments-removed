




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoInputConnection;

import android.content.Context;
import android.content.res.Resources;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.ViewGroup;
import android.view.TextureView;
import android.widget.FrameLayout;
import android.util.AttributeSet;
import android.util.Log;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PixelFormat;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.os.Build;
import android.graphics.SurfaceTexture;

import java.nio.IntBuffer;






public class LayerView extends FrameLayout {
    private static String LOGTAG = "GeckoLayerView";

    private GeckoLayerClient mLayerClient;
    private TouchEventHandler mTouchEventHandler;
    private GLController mGLController;
    private InputConnectionHandler mInputConnectionHandler;
    private LayerRenderer mRenderer;
    
    private int mPaintState = PAINT_NONE;

    private SurfaceView mSurfaceView;
    private TextureView mTextureView;

    private Listener mListener;

    

    public static final int PAINT_NONE = 0;
    public static final int PAINT_BEFORE_FIRST = 1;
    public static final int PAINT_AFTER_FIRST = 2;

    public LayerView(Context context, AttributeSet attrs) {
        super(context, attrs);

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            mSurfaceView = new SurfaceView(context);
            addView(mSurfaceView, ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

            SurfaceHolder holder = mSurfaceView.getHolder();
            holder.addCallback(new SurfaceListener());
            holder.setFormat(PixelFormat.RGB_565);
        } else {
            mTextureView = new TextureView(context);
            mTextureView.setSurfaceTextureListener(new SurfaceTextureListener());

            addView(mTextureView, ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        }

        mGLController = new GLController(this);
    }

    void connect(GeckoLayerClient layerClient) {
        mLayerClient = layerClient;
        mTouchEventHandler = new TouchEventHandler(getContext(), this, layerClient);
        mRenderer = new LayerRenderer(this);
        mInputConnectionHandler = null;

        setFocusable(true);
        setFocusableInTouchMode(true);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN)
            requestFocus();

        
        if (GeckoApp.mAppContext != null)
            GeckoApp.mAppContext.hideFormAssistPopup();

        return mTouchEventHandler.handleEvent(event);
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        return mTouchEventHandler.handleEvent(event);
    }

    public GeckoLayerClient getLayerClient() { return mLayerClient; }
    public TouchEventHandler getTouchEventHandler() { return mTouchEventHandler; }

    
    public void setViewportSize(IntSize size) {
        mLayerClient.setViewportSize(new FloatSize(size));
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

    public void requestRender() {
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


    public LayerRenderer getRenderer() {
        return mRenderer;
    }

    public void setListener(Listener listener) {
        mListener = listener;
    }

    Listener getListener() {
        return mListener;
    }

    public GLController getGLController() {
        return mGLController;
    }

    private Bitmap getDrawable(String name) {
        Context context = getContext();
        Resources resources = context.getResources();
        int resourceID = resources.getIdentifier(name, "drawable", context.getPackageName());
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        return BitmapFactory.decodeResource(context.getResources(), resourceID, options);
    }

    Bitmap getBackgroundPattern() {
        return getDrawable("tabs_tray_selected_bg");
    }

    Bitmap getShadowPattern() {
        return getDrawable("shadow");
    }

    private void onSizeChanged(int width, int height) {
        mGLController.surfaceChanged(width, height);

        if (mListener != null) {
            mListener.surfaceChanged(width, height);
        }
    }

    private void onDestroyed() {
        mGLController.surfaceDestroyed();

        if (mListener != null) {
            mListener.compositionPauseRequested();
        }
    }

    public Object getNativeWindow() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH)
            return mSurfaceView.getHolder();

        return mTextureView.getSurfaceTexture();
    }

    
    public static GLController registerCxxCompositor() {
        try {
            LayerView layerView = GeckoApp.mAppContext.getLayerClient().getView();
            layerView.mListener.compositorCreated();
            return layerView.getGLController();
        } catch (Exception e) {
            Log.e(LOGTAG, "Error registering compositor!", e);
            return null;
        }
    }

    public interface Listener {
        void compositorCreated();
        void renderRequested();
        void compositionPauseRequested();
        void compositionResumeRequested(int width, int height);
        void surfaceChanged(int width, int height);
    }

    private class SurfaceListener implements SurfaceHolder.Callback {
        public void surfaceChanged(SurfaceHolder holder, int format, int width,
                                                int height) {
            onSizeChanged(width, height);
        }

        public void surfaceCreated(SurfaceHolder holder) {
        }

        public void surfaceDestroyed(SurfaceHolder holder) {
            onDestroyed();
        }
    }

    private class SurfaceTextureListener implements TextureView.SurfaceTextureListener {
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            
            
            onSizeChanged(width, height);
        }

        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            onDestroyed();
            return true; 
        }

        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
            onSizeChanged(width, height);
        }

        public void onSurfaceTextureUpdated(SurfaceTexture surface) {

        }
    }
}
