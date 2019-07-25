




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.IntRect;
import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.Layer;
import org.mozilla.fennec.gfx.LayerClient;
import org.mozilla.fennec.gfx.LayerView;
import org.mozilla.fennec.ui.PanZoomController;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View.OnTouchListener;
import java.util.ArrayList;






public class LayerController implements ScaleGestureDetector.OnScaleGestureListener {
    private Layer mRootLayer;                   
    private LayerView mView;                    
    private Context mContext;                   
    private IntRect mVisibleRect;               
    private IntSize mScreenSize;                
    private IntSize mPageSize;                  

    private PanZoomController mPanZoomController;
    




    private OnTouchListener mOnTouchListener;   
    private LayerClient mLayerClient;           

    private ArrayList<OnGeometryChangeListener> mOnGeometryChangeListeners;
    
    private ArrayList<OnPageSizeChangeListener> mOnPageSizeChangeListeners;
    

    
    public static final int TILE_WIDTH = 1024;
    public static final int TILE_HEIGHT = 2048;

    public LayerController(Context context, LayerClient layerClient) {
        mContext = context;

        mOnGeometryChangeListeners = new ArrayList<OnGeometryChangeListener>();
        mOnPageSizeChangeListeners = new ArrayList<OnPageSizeChangeListener>();

        mVisibleRect = new IntRect(0, 0, 1, 1);     
        mScreenSize = new IntSize(1, 1);

        if (layerClient != null)
            setLayerClient(layerClient);
        else
            mPageSize = new IntSize(LayerController.TILE_WIDTH, LayerController.TILE_HEIGHT);

        mView = new LayerView(context, this);
        mPanZoomController = new PanZoomController(this);
    }

    public void setRoot(Layer layer) { mRootLayer = layer; }

    public void setLayerClient(LayerClient layerClient) {
        mLayerClient = layerClient;
        mPageSize = layerClient.getPageSize();
        layerClient.setLayerController(this);
    }

    public Layer getRoot()          { return mRootLayer; }
    public LayerView getView()      { return mView; }
    public Context getContext()     { return mContext; }
    public IntRect getVisibleRect() { return mVisibleRect; }
    public IntSize getScreenSize()  { return mScreenSize; }
    public IntSize getPageSize()    { return mPageSize; }

    public Bitmap getBackgroundPattern()    { return getDrawable("pattern"); }
    public Bitmap getCheckerboardPattern()  { return getDrawable("checkerboard"); }
    public Bitmap getShadowPattern()        { return getDrawable("shadow"); }

    private Bitmap getDrawable(String name) {
        Resources resources = mContext.getResources();
        int resourceID = resources.getIdentifier(name, "drawable", mContext.getPackageName());
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        return BitmapFactory.decodeResource(mContext.getResources(), resourceID, options);
    }

    



    public float getZoomFactor() { return (float)mScreenSize.width / (float)mVisibleRect.width; }

    






    public void setScreenSize(int width, int height) {
        float zoomFactor = getZoomFactor();     

        mScreenSize = new IntSize(width, height);
        setVisibleRect(mVisibleRect.x, mVisibleRect.y,
                       (int)Math.round((float)width / zoomFactor),
                       (int)Math.round((float)height / zoomFactor));

        notifyLayerClientOfGeometryChange();
    }

    public void setNeedsDisplay() {
        
    }

    public void scrollTo(int x, int y) {
        setVisibleRect(x, y, mVisibleRect.width, mVisibleRect.height);
    }

    public void setVisibleRect(int x, int y, int width, int height) {
        mVisibleRect = new IntRect(x, y, width, height);
        setNeedsDisplay();
    }

    




    public void unzoom() {
        float zoomFactor = getZoomFactor();
        mVisibleRect = new IntRect((int)Math.round(mVisibleRect.x * zoomFactor),
                                   (int)Math.round(mVisibleRect.y * zoomFactor),
                                   mScreenSize.width, mScreenSize.height);
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
        if (checkerboarding() || mPanZoomController.getRedrawHint()) {
            Log.e("Fennec", "### checkerboarding? " + checkerboarding() + " pan/zoom? " +
                  mPanZoomController.getRedrawHint());
        }
        return checkerboarding() || mPanZoomController.getRedrawHint();
    }

    private IntRect getTileRect() {
        return new IntRect(mRootLayer.origin.x, mRootLayer.origin.y, TILE_WIDTH, TILE_HEIGHT);
    }

    
    private boolean checkerboarding() {
        return !getTileRect().contains(mVisibleRect);
    }

    




    public boolean onTouchEvent(MotionEvent event) {
        boolean result = mPanZoomController.onTouchEvent(event);
        if (mOnTouchListener != null)
            result = mOnTouchListener.onTouch(mView, event) || result;
        return result;
    }

    @Override
    public boolean onScale(ScaleGestureDetector detector) {
        return mPanZoomController.onScale(detector);
    }

    @Override
    public boolean onScaleBegin(ScaleGestureDetector detector) {
        return mPanZoomController.onScaleBegin(detector);
    }

    @Override
    public void onScaleEnd(ScaleGestureDetector detector) {
        mPanZoomController.onScaleEnd(detector);
    }

    



    public static interface OnGeometryChangeListener {
        public void onGeometryChange(LayerController sender);
    }

    



    public static interface OnPageSizeChangeListener {
        public void onPageSizeChange(LayerController sender);
    }
}

