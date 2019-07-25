





































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
import java.util.ArrayList;






public class LayerController {
    private Layer mRootLayer;                   
    private LayerView mView;                    
    private Context mContext;                   
    private ViewportMetrics mViewportMetrics;   

    private PanZoomController mPanZoomController;
    




    private OnTouchListener mOnTouchListener;   
    private LayerClient mLayerClient;           

    
    public static final int TILE_WIDTH = 1024;
    public static final int TILE_HEIGHT = 2048;

    

    private static final int DANGER_ZONE_X = 150;
    private static final int DANGER_ZONE_Y = 300;

    public LayerController(Context context) {
        mContext = context;

        mViewportMetrics = new ViewportMetrics();
        mPanZoomController = new PanZoomController(this);
        mView = new LayerView(context, this);
    }

    public void setRoot(Layer layer) { mRootLayer = layer; }

    public void setLayerClient(LayerClient layerClient) {
        mLayerClient = layerClient;
        layerClient.setLayerController(this);
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

    private Bitmap getDrawable(String name) {
        Resources resources = mContext.getResources();
        int resourceID = resources.getIdentifier(name, "drawable", mContext.getPackageName());
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        return BitmapFactory.decodeResource(mContext.getResources(), resourceID, options);
    }

    






    public void setViewportSize(FloatSize size) {
        mViewportMetrics.setSize(size);

        notifyLayerClientOfGeometryChange();
        mPanZoomController.geometryChanged();
    }

    public void scrollTo(PointF point) {
        mViewportMetrics.setOrigin(point);
        notifyLayerClientOfGeometryChange();
        mPanZoomController.geometryChanged();
        GeckoApp.mAppContext.repositionPluginViews(false);
    }

    public void scrollBy(PointF point) {
        PointF origin = mViewportMetrics.getOrigin();
        origin.offset(point.x, point.y);
        mViewportMetrics.setOrigin(origin);

        notifyLayerClientOfGeometryChange();
        mPanZoomController.geometryChanged();
        GeckoApp.mAppContext.repositionPluginViews(false);
    }

    public void setViewport(RectF viewport) {
        mViewportMetrics.setViewport(viewport);
        notifyLayerClientOfGeometryChange();
        mPanZoomController.geometryChanged();
        GeckoApp.mAppContext.repositionPluginViews(false);
    }

    public void setPageSize(FloatSize size) {
        if (mViewportMetrics.getPageSize().fuzzyEquals(size))
            return;

        mViewportMetrics.setPageSize(size);

        
        
        mPanZoomController.geometryChanged();
    }

    public void setViewportMetrics(ViewportMetrics viewport) {
        mViewportMetrics = new ViewportMetrics(viewport);

        
        
        mPanZoomController.geometryChanged();
        GeckoApp.mAppContext.repositionPluginViews(false);
    }

    public void scaleTo(float zoomFactor, PointF focus) {
        mViewportMetrics.scaleTo(zoomFactor, focus);

        
        
        notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.repositionPluginViews(false);
    }

    public boolean post(Runnable action) { return mView.post(action); }

    public void setOnTouchListener(OnTouchListener onTouchListener) {
        mOnTouchListener = onTouchListener;
    }

    



    public void notifyLayerClientOfGeometryChange() {
        if (mLayerClient != null)
            mLayerClient.geometryChanged();
    }

    



    public boolean getRedrawHint() {
        return true;
    }

    private RectF getTileRect() {
        float x = mRootLayer.getOrigin().x, y = mRootLayer.getOrigin().y;
        return new RectF(x, y, x + TILE_WIDTH, y + TILE_HEIGHT);
    }

    
    private boolean aboutToCheckerboard() {
        RectF adjustedTileRect =
            RectUtils.contract(getTileRect(), DANGER_ZONE_X, DANGER_ZONE_Y);
        return !adjustedTileRect.contains(new RectF(mViewportMetrics.getViewport()));
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

