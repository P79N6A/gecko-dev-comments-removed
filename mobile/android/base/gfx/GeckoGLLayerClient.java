




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

public class GeckoGLLayerClient extends GeckoLayerClient {
    private static final String LOGTAG = "GeckoGLLayerClient";

    private Context mContext;
    private boolean mLayerRendererInitialized;

    public GeckoGLLayerClient(Context context) {
        super(context);
        mContext = context;
    }

    
    public ViewTransform getViewTransform() {
        Log.e(LOGTAG, "### getViewTransform()");

        
        

        synchronized (mLayerController) {
            ViewportMetrics viewportMetrics = mLayerController.getViewportMetrics();
            PointF viewportOrigin = viewportMetrics.getOrigin();
            Point tileOrigin = mTileLayer.getOrigin();
            float scrollX = viewportOrigin.x; 
            float scrollY = viewportOrigin.y;
            float zoomFactor = viewportMetrics.getZoomFactor();
            Log.e(LOGTAG, "### Viewport metrics = " + viewportMetrics + " tile reso = " +
                  mTileLayer.getResolution());
            return new ViewTransform(scrollX, scrollY, zoomFactor);
        }
    }

    
    public LayerRenderer.Frame createFrame() {
        
        if (!mLayerRendererInitialized) {
            mLayerRenderer.createProgram();
            mLayerRendererInitialized = true;
        }

        
        Layer.RenderContext pageContext = mLayerRenderer.createPageContext();
        Layer.RenderContext screenContext = mLayerRenderer.createScreenContext();
        return mLayerRenderer.createFrame(pageContext, screenContext);
    }

    
    public void activateProgram() {
        mLayerRenderer.activateProgram();
    }

    
    public void deactivateProgram() {
        mLayerRenderer.deactivateProgram();
    }

    
    public SurfaceView createSurfaceViewForBackingSurface(final int width, final int height) {
        final SurfaceView[] surfaceViewResult = new SurfaceView[1];

        mLayerController.getView().post(new Runnable() {
            @Override
            public void run() {
                final SurfaceView surfaceView = new SurfaceView(mContext) {
                    @Override
                    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
                        setMeasuredDimension(width, height);
                    }
                };

                surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
                    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                                               int height) {
                        surfaceViewResult[0] = surfaceView;
                        GeckoGLLayerClient.this.notifyAll();
                    }

                    public void surfaceCreated(SurfaceHolder holder) {
                        
                    }

                    public void surfaceDestroyed(SurfaceHolder holder) {
                        
                    }
                });

                GeckoApp.mAppContext.addSurfaceViewForBackingSurface(surfaceView, width, height);
            }
        });

        while (surfaceViewResult[0] == null) {
            try {
                wait();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }

        return surfaceViewResult[0];
    }
}

