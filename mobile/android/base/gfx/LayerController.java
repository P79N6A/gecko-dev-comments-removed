





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.MotionEvent;
import android.view.GestureDetector;
import android.view.ScaleGestureDetector;
import android.view.View.OnTouchListener;
import java.lang.Math;








public class LayerController {
    private static final String LOGTAG = "GeckoLayerController";

    private Layer mRootLayer;                   
    private LayerView mView;                    
    private Context mContext;                   
    private ViewportMetrics mViewportMetrics;   

    private PanZoomController mPanZoomController;
    




    private OnTouchListener mOnTouchListener;   
    private LayerClient mLayerClient;           

    private boolean mForceRedraw;

    



    public static final IntSize MIN_BUFFER = new IntSize(512, 1024);

    

    private static final int DANGER_ZONE_X = 75;
    private static final int DANGER_ZONE_Y = 150;

    public LayerController(Context context) {
        mContext = context;

        mForceRedraw = true;
        mViewportMetrics = new ViewportMetrics();
        mPanZoomController = new PanZoomController(this);
        mView = new LayerView(context, this);
    }

    public void setRoot(Layer layer) { mRootLayer = layer; }

    public void setLayerClient(LayerClient layerClient) {
        mLayerClient = layerClient;
        layerClient.setLayerController(this);
    }

    public void setForceRedraw() {
        mForceRedraw = true;
    }

    public LayerClient getLayerClient()           { return mLayerClient; }
    public Layer getRoot()                        { return mRootLayer; }
    public LayerView getView()                    { return mView; }
    public Context getContext()                   { return mContext; }
    public ViewportMetrics getViewportMetrics()   { return mViewportMetrics; }

    public RectF getViewport() {
        return mViewportMetrics.getViewport();
    }

    public FloatSize getViewportSize() {
        return mViewportMetrics.getSize();
    }

    public FloatSize getPageSize() {
        return mViewportMetrics.getPageSize();
    }

    public PointF getOrigin() {
        return mViewportMetrics.getOrigin();
    }

    public float getZoomFactor() {
        return mViewportMetrics.getZoomFactor();
    }

    public Bitmap getCheckerboardPattern()  { return getDrawable("checkerboard"); }
    public Bitmap getShadowPattern()        { return getDrawable("shadow"); }

    public GestureDetector.OnGestureListener getGestureListener()                   { return mPanZoomController; }
    public ScaleGestureDetector.OnScaleGestureListener getScaleGestureListener()    { return mPanZoomController; }
    public GestureDetector.OnDoubleTapListener getDoubleTapListener()               { return mPanZoomController; }

    private Bitmap getDrawable(String name) {
        Resources resources = mContext.getResources();
        int resourceID = resources.getIdentifier(name, "drawable", mContext.getPackageName());
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        return BitmapFactory.decodeResource(mContext.getResources(), resourceID, options);
    }

    







    public void setViewportSize(FloatSize size) {
        mViewportMetrics.setSize(size);
        Log.d(LOGTAG, "setViewportSize: " + mViewportMetrics);
        setForceRedraw();

        if (mLayerClient != null)
            mLayerClient.viewportSizeChanged();

        notifyLayerClientOfGeometryChange();
        mPanZoomController.abortAnimation();
        mView.requestRender();
    }

    
    public void scrollTo(PointF point) {
        mViewportMetrics.setOrigin(point);
        Log.d(LOGTAG, "scrollTo: " + mViewportMetrics);
        notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    
    public void scrollBy(PointF point) {
        PointF origin = mViewportMetrics.getOrigin();
        origin.offset(point.x, point.y);
        mViewportMetrics.setOrigin(origin);
        Log.d(LOGTAG, "scrollBy: " + mViewportMetrics);

        notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    
    public void setViewport(RectF viewport) {
        mViewportMetrics.setViewport(viewport);
        Log.d(LOGTAG, "setViewport: " + mViewportMetrics);
        notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    
    public void setPageSize(FloatSize size) {
        if (mViewportMetrics.getPageSize().fuzzyEquals(size))
            return;

        mViewportMetrics.setPageSize(size);
        Log.d(LOGTAG, "setPageSize: " + mViewportMetrics);

        
        
        mView.requestRender();
    }

    





    public void setViewportMetrics(ViewportMetrics viewport) {
        mViewportMetrics = new ViewportMetrics(viewport);
        Log.d(LOGTAG, "setViewportMetrics: " + mViewportMetrics);
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    
    public void scaleTo(float zoomFactor) {
        scaleWithFocus(zoomFactor, new PointF(0,0));
    }

    



    public void scaleWithFocus(float zoomFactor, PointF focus) {
        mViewportMetrics.scaleTo(zoomFactor, focus);
        Log.d(LOGTAG, "scaleWithFocus: " + mViewportMetrics + "; zf=" + zoomFactor);

        
        
        notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    



    public void scaleWithOrigin(float zoomFactor, PointF origin) {
        mViewportMetrics.setOrigin(origin);
        Log.d(LOGTAG, "scaleWithOrigin: " + mViewportMetrics + "; zf=" + zoomFactor);
        scaleTo(zoomFactor);
    }

    public boolean post(Runnable action) { return mView.post(action); }

    public void setOnTouchListener(OnTouchListener onTouchListener) {
        mOnTouchListener = onTouchListener;
    }

    



    public void notifyLayerClientOfGeometryChange() {
        if (mLayerClient != null)
            mLayerClient.geometryChanged();
    }

    
    public void abortPanZoomAnimation() {
        if (mPanZoomController != null)
            mPanZoomController.abortAnimation();
    }

    



    public boolean getRedrawHint() {
        if (mForceRedraw) {
            mForceRedraw = false;
            return true;
        }

        return aboutToCheckerboard() && mPanZoomController.getRedrawHint();
    }

    private RectF getTileRect() {
        if (mRootLayer == null)
            return new RectF();

        float x = mRootLayer.getOrigin().x, y = mRootLayer.getOrigin().y;
        IntSize layerSize = mRootLayer.getSize();
        return new RectF(x, y, x + layerSize.width, y + layerSize.height);
    }

    public RectF restrictToPageSize(RectF aRect) {
        FloatSize pageSize = getPageSize();
        return RectUtils.restrict(aRect, new RectF(0, 0, pageSize.width, pageSize.height));
    }

    
    private boolean aboutToCheckerboard() {
        
        
        
        FloatSize pageSize = getPageSize();
        RectF adjustedViewport = RectUtils.expand(getViewport(), DANGER_ZONE_X, DANGER_ZONE_Y);
        if (adjustedViewport.top < 0) adjustedViewport.top = 0;
        if (adjustedViewport.left < 0) adjustedViewport.left = 0;
        if (adjustedViewport.right > pageSize.width) adjustedViewport.right = pageSize.width;
        if (adjustedViewport.bottom > pageSize.height) adjustedViewport.bottom = pageSize.height;

        return !getTileRect().contains(adjustedViewport);
    }

    






    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        if (mRootLayer == null)
            return null;

        
        PointF origin = mViewportMetrics.getOrigin();
        PointF newPoint = new PointF(origin.x, origin.y);
        newPoint.offset(viewPoint.x, viewPoint.y);

        Point rootOrigin = mRootLayer.getOrigin();
        newPoint.offset(-rootOrigin.x, -rootOrigin.y);

        return newPoint;
    }

    



    public boolean onTouchEvent(MotionEvent event) {
        if (mPanZoomController.onTouchEvent(event))
            return true;
        if (mOnTouchListener != null)
            return mOnTouchListener.onTouch(mView, event);
        return false;
    }
}

