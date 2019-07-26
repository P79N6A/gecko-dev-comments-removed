




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAccessibility;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.PrefsHelper;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.TouchEventInterceptor;
import org.mozilla.gecko.ZoomConstraints;
import org.mozilla.gecko.mozglue.generatorannotations.WrapElementForJNI;
import org.mozilla.gecko.util.EventDispatcher;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.FrameLayout;

import java.nio.IntBuffer;
import java.util.ArrayList;






public class LayerView extends FrameLayout implements Tabs.OnTabsChangedListener {
    private static String LOGTAG = "GeckoLayerView";

    private GeckoLayerClient mLayerClient;
    private PanZoomController mPanZoomController;
    private LayerMarginsAnimator mMarginsAnimator;
    private GLController mGLController;
    private InputConnectionHandler mInputConnectionHandler;
    private LayerRenderer mRenderer;
    
    private int mPaintState;
    private int mBackgroundColor;
    private boolean mFullScreen;

    private SurfaceView mSurfaceView;
    private TextureView mTextureView;

    private Listener mListener;

    
    private final ArrayList<TouchEventInterceptor> mTouchInterceptors;
    private final Overscroll mOverscroll;

    
    public static final int PAINT_START = 0;
    public static final int PAINT_BEFORE_FIRST = 1;
    public static final int PAINT_AFTER_FIRST = 2;

    public boolean shouldUseTextureView() {
        
        
        
        return false;

        














    }

    public LayerView(Context context, AttributeSet attrs) {
        super(context, attrs);

        mGLController = GLController.getInstance(this);
        mPaintState = PAINT_START;
        mBackgroundColor = Color.WHITE;

        mTouchInterceptors = new ArrayList<TouchEventInterceptor>();
        mOverscroll = new Overscroll(this);
        Tabs.registerOnTabsChangedListener(this);
    }

    public void initializeView(EventDispatcher eventDispatcher) {
        mLayerClient = new GeckoLayerClient(getContext(), this, eventDispatcher);
        mLayerClient.setOverscrollHandler(mOverscroll);

        mPanZoomController = mLayerClient.getPanZoomController();
        mMarginsAnimator = mLayerClient.getLayerMarginsAnimator();

        mRenderer = new LayerRenderer(this);
        mInputConnectionHandler = null;

        setFocusable(true);
        setFocusableInTouchMode(true);

        GeckoAccessibility.setDelegate(this);
    }

    private Point getEventRadius(MotionEvent event) {
        if (Build.VERSION.SDK_INT >= 9) {
            return new Point((int)event.getToolMajor()/2,
                             (int)event.getToolMinor()/2);
        }

        float size = event.getSize();
        DisplayMetrics displaymetrics = getContext().getResources().getDisplayMetrics();
        size = size * Math.min(displaymetrics.heightPixels, displaymetrics.widthPixels);
        return new Point((int)size, (int)size);
    }

    public void geckoConnected() {
        
        PrefsHelper.getPref("gfx.android.rgb16.force", new PrefsHelper.PrefHandlerBase() {
            @Override public void prefValue(String pref, boolean force16bit) {
                if (force16bit) {
                    GeckoAppShell.setScreenDepthOverride(16);
                }
            }
        });

        mLayerClient.notifyGeckoReady();
        addTouchInterceptor(new TouchEventInterceptor() {
            private PointF mInitialTouchPoint = null;

            @Override
            public boolean onInterceptTouchEvent(View view, MotionEvent event) {
                return false;
            }

            @Override
            public boolean onTouch(View view, MotionEvent event) {
                if (event == null) {
                    return true;
                }

                int action = event.getActionMasked();
                PointF point = new PointF(event.getX(), event.getY());
                if (action == MotionEvent.ACTION_DOWN) {
                    mInitialTouchPoint = point;
                }

                if (mInitialTouchPoint != null && action == MotionEvent.ACTION_MOVE) {
                    Point p = getEventRadius(event);

                    if (PointUtils.subtract(point, mInitialTouchPoint).length() <
                        Math.max(PanZoomController.CLICK_THRESHOLD, Math.min(Math.min(p.x, p.y), PanZoomController.PAN_THRESHOLD))) {
                        
                        
                        return true;
                    } else {
                        mInitialTouchPoint = null;
                    }
                }

                GeckoAppShell.sendEventToGecko(GeckoEvent.createMotionEvent(event, false));
                return true;
            }
        });
    }

    public void show() {
        
        mSurfaceView.setVisibility(View.VISIBLE);
    }

    public void hide() {
        
        mSurfaceView.setVisibility(View.INVISIBLE);
    }

    public void destroy() {
        if (mLayerClient != null) {
            mLayerClient.destroy();
        }
        if (mRenderer != null) {
            mRenderer.destroy();
        }
        Tabs.unregisterOnTabsChangedListener(this);
    }

    public void addTouchInterceptor(final TouchEventInterceptor aTouchInterceptor) {
        post(new Runnable() {
            @Override
            public void run() {
                mTouchInterceptors.add(aTouchInterceptor);
            }
        });
    }

    public void removeTouchInterceptor(final TouchEventInterceptor aTouchInterceptor) {
        post(new Runnable() {
            @Override
            public void run() {
                mTouchInterceptors.remove(aTouchInterceptor);
            }
        });
    }

    private boolean runTouchInterceptors(MotionEvent event, boolean aOnTouch) {
        boolean result = false;
        for (TouchEventInterceptor i : mTouchInterceptors) {
            if (aOnTouch) {
                result |= i.onTouch(this, event);
            } else {
                result |= i.onInterceptTouchEvent(this, event);
            }
        }

        return result;
    }

    @Override
    public void dispatchDraw(final Canvas canvas) {
        super.dispatchDraw(canvas);

        
        if (mLayerClient != null) {
            mOverscroll.draw(canvas, getViewportMetrics());
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_DOWN) {
            requestFocus();
        }

        if (runTouchInterceptors(event, false)) {
            return true;
        }
        if (mPanZoomController != null && mPanZoomController.onTouchEvent(event)) {
            return true;
        }
        if (runTouchInterceptors(event, true)) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onHoverEvent(MotionEvent event) {
        if (runTouchInterceptors(event, true)) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event) {
        if (mPanZoomController != null && mPanZoomController.onMotionEvent(event)) {
            return true;
        }
        return false;
    }

    @Override
    protected void onAttachedToWindow() {
        
        
        
        
        if (shouldUseTextureView()) {
            mTextureView = new TextureView(getContext());
            mTextureView.setSurfaceTextureListener(new SurfaceTextureListener());

            
            
            
            
            mTextureView.setBackgroundColor(Color.WHITE);
            addView(mTextureView, ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        } else {
            
            
            setWillNotCacheDrawing(false);

            mSurfaceView = new LayerSurfaceView(getContext(), this);
            mSurfaceView.setBackgroundColor(Color.WHITE);
            addView(mSurfaceView, ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);

            SurfaceHolder holder = mSurfaceView.getHolder();
            holder.addCallback(new SurfaceListener());
            holder.setFormat(PixelFormat.RGB_565);
        }
    }

    public GeckoLayerClient getLayerClient() { return mLayerClient; }
    public PanZoomController getPanZoomController() { return mPanZoomController; }
    public LayerMarginsAnimator getLayerMarginsAnimator() { return mMarginsAnimator; }

    public ImmutableViewportMetrics getViewportMetrics() {
        return mLayerClient.getViewportMetrics();
    }

    public void abortPanning() {
        if (mPanZoomController != null) {
            mPanZoomController.abortPanning();
        }
    }

    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        return mLayerClient.convertViewPointToLayerPoint(viewPoint);
    }

    int getBackgroundColor() {
        return mBackgroundColor;
    }

    @Override
    public void setBackgroundColor(int newColor) {
        mBackgroundColor = newColor;
        requestRender();
    }

    public void setZoomConstraints(ZoomConstraints constraints) {
        mLayerClient.setZoomConstraints(constraints);
    }

    public void setIsRTL(boolean aIsRTL) {
        mLayerClient.setIsRTL(aIsRTL);
    }

    public void setInputConnectionHandler(InputConnectionHandler inputConnectionHandler) {
        mInputConnectionHandler = inputConnectionHandler;
        mLayerClient.forceRedraw(null);
    }

    @Override
    public Handler getHandler() {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.getHandler(super.getHandler());
        return super.getHandler();
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (mInputConnectionHandler != null)
            return mInputConnectionHandler.onCreateInputConnection(outAttrs);
        return null;
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null && mInputConnectionHandler.onKeyPreIme(keyCode, event)) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (mPanZoomController != null && mPanZoomController.onKeyEvent(event)) {
            return true;
        }
        if (mInputConnectionHandler != null && mInputConnectionHandler.onKeyDown(keyCode, event)) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null && mInputConnectionHandler.onKeyLongPress(keyCode, event)) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyMultiple(int keyCode, int repeatCount, KeyEvent event) {
        if (mInputConnectionHandler != null && mInputConnectionHandler.onKeyMultiple(keyCode, repeatCount, event)) {
            return true;
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (mInputConnectionHandler != null && mInputConnectionHandler.onKeyUp(keyCode, event)) {
            return true;
        }
        return false;
    }

    public boolean isIMEEnabled() {
        if (mInputConnectionHandler != null) {
            return mInputConnectionHandler.isIMEEnabled();
        }
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

    public void postRenderTask(RenderTask task) {
        mRenderer.postRenderTask(task);
    }

    public void removeRenderTask(RenderTask task) {
        mRenderer.removeRenderTask(task);
    }

    public int getMaxTextureSize() {
        return mRenderer.getMaxTextureSize();
    }

    
    public IntBuffer getPixels() {
        return mRenderer.getPixels();
    }

    
    public void setPaintState(int paintState) {
        mPaintState = paintState;
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
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        Context context = getContext();
        int resId = context.getResources().getIdentifier(name, "drawable", context.getPackageName());
        return BitmapUtils.decodeResource(context, resId, options);
    }

    Bitmap getShadowPattern() {
        return getDrawable("shadow");
    }

    Bitmap getScrollbarImage() {
        return getDrawable("scrollbar");
    }

    

















    private void onSizeChanged(int width, int height) {
        if (!mGLController.isServerSurfaceValid() || mSurfaceView == null) {
            surfaceChanged(width, height);
            return;
        }

        if (mListener != null) {
            mListener.sizeChanged(width, height);
        }

        mOverscroll.setSize(width, height);
    }

    private void surfaceChanged(int width, int height) {
        mGLController.serverSurfaceChanged(width, height);

        if (mListener != null) {
            mListener.surfaceChanged(width, height);
        }

        mOverscroll.setSize(width, height);
    }

    private void onDestroyed() {
        mGLController.serverSurfaceDestroyed();
    }

    public Object getNativeWindow() {
        if (mSurfaceView != null)
            return mSurfaceView.getHolder();

        return mTextureView.getSurfaceTexture();
    }

    @WrapElementForJNI(allowMultithread = true, stubName = "RegisterCompositorWrapper")
    public static GLController registerCxxCompositor() {
        try {
            LayerView layerView = GeckoAppShell.getLayerView();
            GLController controller = layerView.getGLController();
            controller.compositorCreated();
            return controller;
        } catch (Exception e) {
            Log.e(LOGTAG, "Error registering compositor!", e);
            return null;
        }
    }

    public interface Listener {
        void renderRequested();
        void sizeChanged(int width, int height);
        void surfaceChanged(int width, int height);
    }

    private class SurfaceListener implements SurfaceHolder.Callback {
        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width,
                                                int height) {
            onSizeChanged(width, height);
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            onDestroyed();
        }
    }

    


    private class LayerSurfaceView extends SurfaceView {
        LayerView mParent;

        public LayerSurfaceView(Context aContext, LayerView aParent) {
            super(aContext);
            mParent = aParent;
        }

        @Override
        protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
            if (changed) {
                mParent.surfaceChanged(right - left, bottom - top);
            }
        }
    }

    private class SurfaceTextureListener implements TextureView.SurfaceTextureListener {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            
            
            onSizeChanged(width, height);
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            onDestroyed();
            return true; 
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
            onSizeChanged(width, height);
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {

        }
    }

    @Override
    public void setOverScrollMode(int overscrollMode) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
            super.setOverScrollMode(overscrollMode);
        }
        if (mPanZoomController != null) {
            mPanZoomController.setOverScrollMode(overscrollMode);
        }
    }

    @Override
    public int getOverScrollMode() {
        if (mPanZoomController != null) {
            return mPanZoomController.getOverScrollMode();
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
            return super.getOverScrollMode();
        }
        return View.OVER_SCROLL_ALWAYS;
    }

    @Override
    public void onFocusChanged (boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(gainFocus, direction, previouslyFocusedRect);
        GeckoAccessibility.onLayerViewFocusChanged(this, gainFocus);
    }

    public void setFullScreen(boolean fullScreen) {
        mFullScreen = fullScreen;
    }

    public boolean isFullScreen() {
        return mFullScreen;
    }

    @Override
    public void onTabChanged(Tab tab, Tabs.TabEvents msg, Object data) {
        if (msg == Tabs.TabEvents.VIEWPORT_CHANGE && Tabs.getInstance().isSelectedTab(tab)) {
            setZoomConstraints(tab.getZoomConstraints());
            setIsRTL(tab.getIsRTL());
        }
    }
}
