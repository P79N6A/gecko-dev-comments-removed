





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.PointF;
import android.graphics.RectF;
import android.util.Log;
import android.view.GestureDetector;
import android.view.View.OnTouchListener;








public class LayerController {
    private static final String LOGTAG = "GeckoLayerController";

    private Layer mRootLayer;                   
    private LayerView mView;                    
    private Context mContext;                   

    










    private volatile ImmutableViewportMetrics mViewportMetrics;   

    



    private PanZoomController mPanZoomController;

    private GeckoLayerClient mLayerClient;          

    
    private int mCheckerboardColor = Color.WHITE;
    private boolean mCheckerboardShouldShowChecks;

    private boolean mForceRedraw;

    public LayerController(Context context) {
        mContext = context;

        mForceRedraw = true;
        mViewportMetrics = new ImmutableViewportMetrics(new ViewportMetrics());
        mPanZoomController = new PanZoomController(this);
        mView = new LayerView(context, this);
        mCheckerboardShouldShowChecks = true;
    }

    public void setRoot(Layer layer) { mRootLayer = layer; }

    public void setLayerClient(GeckoLayerClient layerClient) {
        mLayerClient = layerClient;
        layerClient.setLayerController(this);
    }

    public void setForceRedraw() {
        mForceRedraw = true;
    }

    public Layer getRoot()                        { return mRootLayer; }
    public LayerView getView()                    { return mView; }
    public Context getContext()                   { return mContext; }
    public ImmutableViewportMetrics getViewportMetrics()   { return mViewportMetrics; }

    public RectF getViewport() {
        return mViewportMetrics.getViewport();
    }

    public RectF getCssViewport() {
        return mViewportMetrics.getCssViewport();
    }

    public FloatSize getViewportSize() {
        return mViewportMetrics.getSize();
    }

    public FloatSize getPageSize() {
        return mViewportMetrics.getPageSize();
    }

    public FloatSize getCssPageSize() {
        return mViewportMetrics.getCssPageSize();
    }

    public PointF getOrigin() {
        return mViewportMetrics.getOrigin();
    }

    public float getZoomFactor() {
        return mViewportMetrics.zoomFactor;
    }

    public Bitmap getBackgroundPattern()    { return getDrawable("background"); }
    public Bitmap getShadowPattern()        { return getDrawable("shadow"); }

    public PanZoomController getPanZoomController()                                 { return mPanZoomController; }
    public GestureDetector.OnGestureListener getGestureListener()                   { return mPanZoomController; }
    public SimpleScaleGestureDetector.SimpleScaleGestureListener getScaleGestureListener() {
        return mPanZoomController;
    }
    public GestureDetector.OnDoubleTapListener getDoubleTapListener()               { return mPanZoomController; }

    private Bitmap getDrawable(String name) {
        Resources resources = mContext.getResources();
        int resourceID = resources.getIdentifier(name, "drawable", mContext.getPackageName());
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        return BitmapFactory.decodeResource(mContext.getResources(), resourceID, options);
    }

    







    public void setViewportSize(FloatSize size) {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.setSize(size);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        if (mLayerClient != null) {
            mLayerClient.viewportSizeChanged();
        }
    }

    
    public void scrollBy(PointF point) {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        PointF origin = viewportMetrics.getOrigin();
        origin.offset(point.x, point.y);
        viewportMetrics.setOrigin(origin);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        notifyLayerClientOfGeometryChange();
        mView.requestRender();
    }

    
    public void setPageSize(FloatSize size, FloatSize cssSize) {
        if (mViewportMetrics.getCssPageSize().equals(cssSize))
            return;

        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.setPageSize(size, cssSize);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        
        

        mView.post(new Runnable() {
            public void run() {
                mPanZoomController.pageSizeUpdated();
                mView.requestRender();
            }
        });
    }

    





    public void setViewportMetrics(ViewportMetrics viewport) {
        mViewportMetrics = new ImmutableViewportMetrics(viewport);
        mView.requestRender();
    }

    public void setAnimationTarget(ViewportMetrics viewport) {
        if (mLayerClient != null) {
            
            
            
            
            ImmutableViewportMetrics metrics = new ImmutableViewportMetrics(viewport);
            DisplayPortMetrics displayPort = DisplayPortCalculator.calculate(metrics, null);
            mLayerClient.adjustViewport(displayPort);
        }
    }

    



    public void scaleWithFocus(float zoomFactor, PointF focus) {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.scaleTo(zoomFactor, focus);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        
        
        notifyLayerClientOfGeometryChange();
        mView.requestRender();
    }

    public boolean post(Runnable action) { return mView.post(action); }

    



    public void notifyLayerClientOfGeometryChange() {
        if (mLayerClient != null)
            mLayerClient.geometryChanged();
    }

    
    public void abortPanZoomAnimation() {
        if (mPanZoomController != null) {
            mView.post(new Runnable() {
                public void run() {
                    mPanZoomController.abortAnimation();
                }
            });
        }
    }

    



    public boolean getRedrawHint() {
        if (mForceRedraw) {
            mForceRedraw = false;
            return true;
        }

        if (!mPanZoomController.getRedrawHint()) {
            return false;
        }

        return DisplayPortCalculator.aboutToCheckerboard(mViewportMetrics,
                mPanZoomController.getVelocityVector(), mLayerClient.getDisplayPort());
    }

    






    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        if (mLayerClient == null) {
            return null;
        }

        ImmutableViewportMetrics viewportMetrics = mViewportMetrics;
        PointF origin = viewportMetrics.getOrigin();
        float zoom = viewportMetrics.zoomFactor;
        ViewportMetrics geckoViewport = mLayerClient.getGeckoViewportMetrics();
        PointF geckoOrigin = geckoViewport.getOrigin();
        float geckoZoom = geckoViewport.getZoomFactor();

        
        
        
        
        
        PointF layerPoint = new PointF(
                ((viewPoint.x + origin.x) / zoom) - (geckoOrigin.x / geckoZoom),
                ((viewPoint.y + origin.y) / zoom) - (geckoOrigin.y / geckoZoom));

        return layerPoint;
    }

    
    public boolean checkerboardShouldShowChecks() {
        return mCheckerboardShouldShowChecks;
    }

    
    public int getCheckerboardColor() {
        return mCheckerboardColor;
    }

    
    public void setCheckerboardShowChecks(boolean showChecks) {
        mCheckerboardShouldShowChecks = showChecks;
        mView.requestRender();
    }

    
    public void setCheckerboardColor(int newColor) {
        mCheckerboardColor = newColor;
        mView.requestRender();
    }
}
