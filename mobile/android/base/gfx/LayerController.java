





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.Layer;
import org.mozilla.gecko.ui.PanZoomController;
import org.mozilla.gecko.ui.SimpleScaleGestureDetector;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.Tab;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.MotionEvent;
import android.view.GestureDetector;
import android.view.ScaleGestureDetector;
import android.view.View.OnTouchListener;
import android.view.ViewConfiguration;
import java.lang.Math;
import java.util.Timer;
import java.util.TimerTask;
import java.util.regex.Matcher;
import java.util.regex.Pattern;








public class LayerController implements Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoLayerController";

    private Layer mRootLayer;                   
    private LayerView mView;                    
    private Context mContext;                   

    










    private volatile ImmutableViewportMetrics mViewportMetrics;   

    private boolean mWaitForTouchListeners;

    private PanZoomController mPanZoomController;
    




    private OnTouchListener mOnTouchListener;       
    private GeckoLayerClient mLayerClient;          

    
    private int mCheckerboardColor;
    private boolean mCheckerboardShouldShowChecks;

    private boolean mForceRedraw;

    



    public static final IntSize MIN_BUFFER = new IntSize(512, 1024);

    

    private static final int DANGER_ZONE_X = 75;
    private static final int DANGER_ZONE_Y = 150;

    

    private int mTimeout = 200;

    private boolean allowDefaultActions = true;
    private Timer allowDefaultTimer =  null;
    private PointF initialTouchLocation = null;

    private static Pattern sColorPattern;

    public LayerController(Context context) {
        mContext = context;

        mForceRedraw = true;
        mViewportMetrics = new ImmutableViewportMetrics(new ViewportMetrics());
        mPanZoomController = new PanZoomController(this);
        mView = new LayerView(context, this);
        mCheckerboardShouldShowChecks = true;

        Tabs.getInstance().registerOnTabsChangedListener(this);

        ViewConfiguration vc = ViewConfiguration.get(mContext); 
        mTimeout = vc.getLongPressTimeout();
    }

    public void onDestroy() {
        Tabs.getInstance().unregisterOnTabsChangedListener(this);
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
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    
    public void setPageSize(FloatSize size) {
        if (mViewportMetrics.getPageSize().fuzzyEquals(size))
            return;

        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.setPageSize(size);
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
        
        
        GeckoApp.mAppContext.runOnUiThread(new Runnable() {
            public void run() {
                GeckoApp.mAppContext.repositionPluginViews(false);
            }
        });
        mView.requestRender();
    }

    



    public void scaleWithFocus(float zoomFactor, PointF focus) {
        ViewportMetrics viewportMetrics = new ViewportMetrics(mViewportMetrics);
        viewportMetrics.scaleTo(zoomFactor, focus);
        mViewportMetrics = new ImmutableViewportMetrics(viewportMetrics);

        
        
        notifyLayerClientOfGeometryChange();
        GeckoApp.mAppContext.repositionPluginViews(false);
        mView.requestRender();
    }

    public boolean post(Runnable action) { return mView.post(action); }

    public void setOnTouchListener(OnTouchListener onTouchListener) {
        mOnTouchListener = onTouchListener;
    }

    



    public void notifyLayerClientOfGeometryChange() {
        if (mLayerClient != null)
            mLayerClient.geometryChanged();
    }

    
    public void abortPanZoomAnimation(final boolean notifyLayerClient) {
        if (mPanZoomController != null) {
            mView.post(new Runnable() {
                public void run() {
                    mPanZoomController.abortAnimation();
                    if (notifyLayerClient) {
                        notifyLayerClientOfGeometryChange();
                    }
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

        return aboutToCheckerboard();
    }

    
    private boolean aboutToCheckerboard() {
        
        
        
        FloatSize pageSize = getPageSize();
        RectF adjustedViewport = RectUtils.expand(getViewport(), DANGER_ZONE_X, DANGER_ZONE_Y);
        if (adjustedViewport.top < 0) adjustedViewport.top = 0;
        if (adjustedViewport.left < 0) adjustedViewport.left = 0;
        if (adjustedViewport.right > pageSize.width) adjustedViewport.right = pageSize.width;
        if (adjustedViewport.bottom > pageSize.height) adjustedViewport.bottom = pageSize.height;

        RectF displayPort = (mLayerClient == null ? new RectF() : mLayerClient.getDisplayPort());
        return !displayPort.contains(adjustedViewport);
    }

    






    public PointF convertViewPointToLayerPoint(PointF viewPoint) {
        if (mRootLayer == null)
            return null;

        ImmutableViewportMetrics viewportMetrics = mViewportMetrics;
        PointF origin = viewportMetrics.getOrigin();
        float zoom = viewportMetrics.zoomFactor;
        Rect rootPosition = mRootLayer.getPosition();
        float rootScale = mRootLayer.getResolution();

        
        
        
        
        
        PointF layerPoint = new PointF(
                ((viewPoint.x + origin.x) / zoom) - (rootPosition.left / rootScale),
                ((viewPoint.y + origin.y) / zoom) - (rootPosition.top / rootScale));

        return layerPoint;
    }

    



    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getAction();
        PointF point = new PointF(event.getX(), event.getY());

        
        if ((action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_DOWN) {
            initialTouchLocation = point;
            allowDefaultActions = !mWaitForTouchListeners;

            
            
            if (allowDefaultTimer != null) {
              allowDefaultTimer.cancel();
            } else {
              
              mView.clearEventQueue();
            }
            allowDefaultTimer = new Timer();
            allowDefaultTimer.schedule(new TimerTask() {
                public void run() {
                    post(new Runnable() {
                        public void run() {
                            preventPanning(false);
                        }
                    });
                }
            }, mTimeout);
        }

        
        if (initialTouchLocation != null && (action & MotionEvent.ACTION_MASK) == MotionEvent.ACTION_MOVE) {
            if (PointUtils.subtract(point, initialTouchLocation).length() > PanZoomController.PAN_THRESHOLD) {
                initialTouchLocation = null;
            } else {
                return !allowDefaultActions;
            }
        }

        
        if (mOnTouchListener != null)
            mOnTouchListener.onTouch(mView, event);

        return !allowDefaultActions;
    }

    public void preventPanning(boolean aValue) {
        if (allowDefaultTimer != null) {
            allowDefaultTimer.cancel();
            allowDefaultTimer = null;
        }
        if (aValue == allowDefaultActions) {
            allowDefaultActions = !aValue;
    
            if (aValue) {
                mView.clearEventQueue();
                mPanZoomController.cancelTouch();
            } else {
                mView.processEventQueue();
            }
        }
    }

    public void onTabChanged(Tab tab, Tabs.TabEvents msg) {
        if ((Tabs.getInstance().isSelectedTab(tab) && msg == Tabs.TabEvents.STOP) || msg == Tabs.TabEvents.SELECTED) {
            mWaitForTouchListeners = tab.getHasTouchListeners();
        }
    }
    public void setWaitForTouchListeners(boolean aValue) {
        mWaitForTouchListeners = aValue;
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

    
    public void setCheckerboardColor(String newColor) {
        setCheckerboardColor(parseColorFromGecko(newColor));
    }

    
    
    private static int parseColorFromGecko(String string) {
        if (sColorPattern == null) {
            sColorPattern = Pattern.compile("rgb\\((\\d+),\\s*(\\d+),\\s*(\\d+)\\)");
        }

        Matcher matcher = sColorPattern.matcher(string);
        if (!matcher.matches()) {
            return Color.WHITE;
        }

        int r = Integer.parseInt(matcher.group(1));
        int g = Integer.parseInt(matcher.group(2));
        int b = Integer.parseInt(matcher.group(3));
        return Color.rgb(r, g, b);
    } 

}

