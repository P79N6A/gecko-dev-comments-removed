




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.FloatRect;
import org.mozilla.gecko.gfx.IntRect;
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
import android.graphics.PointF;
import android.util.Log;
import android.view.MotionEvent;
import android.view.GestureDetector;
import android.view.ScaleGestureDetector;
import android.view.View.OnTouchListener;
import java.util.ArrayList;






public class LayerController {
    private Layer mRootLayer;                   
    private LayerView mView;                    
    private Context mContext;                   
    private FloatRect mVisibleRect;             
    private IntSize mScreenSize;                
    private IntSize mPageSize;                  

    private PanZoomController mPanZoomController;
    




    private OnTouchListener mOnTouchListener;   
    private LayerClient mLayerClient;           

    public static final int TILE_WIDTH = 1024;
    public static final int TILE_HEIGHT = 2048;
    

    private static final int DANGER_ZONE_X = 150;
    private static final int DANGER_ZONE_Y = 300;
    


    public LayerController(Context context) {
        mContext = context;

        mVisibleRect = new FloatRect(0.0f, 0.0f, 1.0f, 1.0f);
        

        mScreenSize = new IntSize(1, 1);
        mPageSize = new IntSize(LayerController.TILE_WIDTH, LayerController.TILE_HEIGHT);

        mPanZoomController = new PanZoomController(this);
        mView = new LayerView(context, this);
    }

    public void setRoot(Layer layer) { mRootLayer = layer; }

    public void setLayerClient(LayerClient layerClient) {
        mLayerClient = layerClient;
        mPageSize = layerClient.getPageSize();
        layerClient.setLayerController(this);
        layerClient.init();
    }

    public Layer getRoot()              { return mRootLayer; }
    public LayerView getView()          { return mView; }
    public Context getContext()         { return mContext; }
    public FloatRect getVisibleRect()   { return mVisibleRect; }
    public IntSize getScreenSize()      { return mScreenSize; }
    public IntSize getPageSize()        { return mPageSize; }

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

    



    public float getZoomFactor() { return (float)mScreenSize.width / mVisibleRect.width; }

    






    public void setScreenSize(int width, int height) {
        float zoomFactor = getZoomFactor();     

        mScreenSize = new IntSize(width, height);
        setVisibleRect(mVisibleRect.x, mVisibleRect.y, width / zoomFactor, height / zoomFactor);

        notifyLayerClientOfGeometryChange();
    }

    public void setNeedsDisplay() {
        
    }

    public void scrollTo(float x, float y) {
        setVisibleRect(x, y, mVisibleRect.width, mVisibleRect.height);
    }

    public void setVisibleRect(float x, float y, float width, float height) {
        mVisibleRect = new FloatRect(x, y, width, height);
        setNeedsDisplay();
        GeckoApp.mAppContext.repositionPluginViews();
    }

    




    public void unzoom() {
        float zoomFactor = getZoomFactor();
        mVisibleRect = new FloatRect(Math.round(mVisibleRect.x * zoomFactor),
                                     Math.round(mVisibleRect.y * zoomFactor),
                                     mScreenSize.width,
                                     mScreenSize.height);
        mPageSize = mPageSize.scale(zoomFactor);
        setNeedsDisplay();
    }

    public void setPageSize(IntSize size) {
        mPageSize = size.scale(getZoomFactor());
        mView.notifyRendererOfPageSizeChange();
    }

    public boolean post(Runnable action) { return mView.post(action); }

    public void setOnTouchListener(OnTouchListener onTouchListener) {
        mOnTouchListener = onTouchListener;
    }

    



    public void notifyLayerClientOfGeometryChange() {
        if (mLayerClient != null)
            mLayerClient.geometryChanged();
    }

    
    public void notifyViewOfGeometryChange() {
        mView.geometryChanged();
        mPanZoomController.geometryChanged();
    }

    



    public boolean getRedrawHint() {
        return aboutToCheckerboard();
    }

    private FloatRect getTileRect() {
        return new FloatRect(mRootLayer.origin.x, mRootLayer.origin.y, TILE_WIDTH, TILE_HEIGHT);
    }

    
    private boolean aboutToCheckerboard() {
        IntRect pageRect = new IntRect(0, 0, mPageSize.width, mPageSize.height);
        IntRect adjustedPageRect = pageRect.contract(DANGER_ZONE_X, DANGER_ZONE_Y);
        FloatRect visiblePageRect = mVisibleRect.intersect(new FloatRect(adjustedPageRect));
        FloatRect adjustedTileRect = getTileRect().contract(DANGER_ZONE_X, DANGER_ZONE_Y);
        return !adjustedTileRect.contains(visiblePageRect);
    }

    
    public FloatRect clampRect(FloatRect rect) {
        float x = clamp(0, rect.x, mPageSize.width - LayerController.TILE_WIDTH);
        float y = clamp(0, rect.y, mPageSize.height - LayerController.TILE_HEIGHT);
        return new FloatRect(x, y, rect.width, rect.height);
    }

    private float clamp(float min, float value, float max) {
        if (max < min)
            return min;
        return (value < min) ? min : (value > max) ? max : value;
    }

    
    private static FloatRect widenRect(FloatRect rect, float scaleFactor) {
        PointF center = rect.getCenter();
        float halfTileWidth = TILE_WIDTH * scaleFactor / 2.0f;
        float halfTileHeight = TILE_HEIGHT * scaleFactor / 2.0f;
        return new FloatRect(center.x - halfTileWidth, center.y - halfTileHeight,
                             halfTileWidth, halfTileHeight);
    }

    
    public static FloatRect widenRect(FloatRect rect) {
        return widenRect(rect, 1.0f);
    }

    






    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        if (mRootLayer == null)
            return null;

        
        PointF scaledPoint = PointUtils.scale(viewPoint, 1.0f / getZoomFactor());
        return PointUtils.subtract(PointUtils.add(mVisibleRect.getOrigin(), scaledPoint),
                                   mRootLayer.origin);
    }

    




    public boolean onTouchEvent(MotionEvent event) {
        if (mPanZoomController.onTouchEvent(event))
            return true;
        if (mOnTouchListener != null)
            return mOnTouchListener.onTouch(mView, event);
        return false;
    }
}

