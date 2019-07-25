




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.util.Log;
import android.view.View;

public class GeckoGLLayerClient extends GeckoLayerClient {
    private static final String LOGTAG = "GeckoGLLayerClient";

    private boolean mLayerRendererInitialized;

    public GeckoGLLayerClient(Context context) {
        super(context);
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
}

