




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.CairoImage;
import org.mozilla.fennec.gfx.IntRect;
import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.LayerClient;
import org.mozilla.fennec.gfx.LayerController;
import org.mozilla.fennec.gfx.LayerRenderer;
import org.mozilla.fennec.gfx.SingleTileLayer;
import org.mozilla.fennec.ui.ViewportController;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.content.Context;
import android.graphics.Point;
import android.util.Log;
import java.nio.ByteBuffer;
import java.util.concurrent.Semaphore;
import java.util.Timer;
import java.util.TimerTask;







public class GeckoSoftwareLayerClient extends LayerClient {
    private Context mContext;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;
    private Semaphore mBufferSemaphore;
    private SingleTileLayer mTileLayer;
    private ViewportController mViewportController;

    private IntRect mGeckoVisibleRect;  

    private IntRect mJSPanningToRect;
    

    private boolean mWaitingForJSPanZoom;
    


    private CairoImage mCairoImage;

    
    private static final int PAGE_WIDTH = 980;      
    private static final int PAGE_HEIGHT = 1500;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mViewportController = new ViewportController(new IntSize(PAGE_WIDTH, PAGE_HEIGHT),
                                                     new IntRect(0, 0, 1, 1));

        mWidth = LayerController.TILE_WIDTH;
        mHeight = LayerController.TILE_HEIGHT;
        mFormat = CairoImage.FORMAT_RGB16_565;

        mBuffer = ByteBuffer.allocateDirect(mWidth * mHeight * 2);
        mBufferSemaphore = new Semaphore(1);

        mWaitingForJSPanZoom = false;

        mCairoImage = new CairoImage() {
            @Override
            public ByteBuffer lockBuffer() {
                try {
                    mBufferSemaphore.acquire();
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);
                }
                return mBuffer;
            }
            @Override
            public void unlockBuffer() {
                mBufferSemaphore.release();
            }
            @Override
            public int getWidth() { return mWidth; }
            @Override
            public int getHeight() { return mHeight; }
            @Override
            public int getFormat() { return mFormat; }
        };

        mTileLayer = new SingleTileLayer();
    }

    
    @Override
    public void init() {
        getLayerController().setRoot(mTileLayer);
    }

    public void beginDrawing() {
        
    }

    



    public void endDrawing(int x, int y, int width, int height) {
        LayerController controller = getLayerController();
        
        controller.notifyViewOfGeometryChange();

        mViewportController.setVisibleRect(mGeckoVisibleRect);

        if (mGeckoVisibleRect != null) {
            IntRect layerRect = mViewportController.untransformVisibleRect(mGeckoVisibleRect,
                                                                           getPageSize());
            mTileLayer.origin = layerRect.getOrigin();
        }

        repaint(new IntRect(x, y, width, height));
    }

    private void repaint(IntRect rect) {
        mTileLayer.paintSubimage(mCairoImage, rect);
    }

    
    public void jsPanZoomCompleted(IntRect rect) {
        mGeckoVisibleRect = rect;
        if (mWaitingForJSPanZoom)
            render();
    }

    



    public ByteBuffer lockBuffer() {
        try {
            mBufferSemaphore.acquire();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }
        return mBuffer;
    }

    



    public void unlockBuffer() {
        mBufferSemaphore.release();
    }

    
    public void setPageSize(IntSize pageSize) {
        Log.e("Fennec", "### Setting page size to " + pageSize);
        mViewportController.setPageSize(pageSize);
        getLayerController().setPageSize(pageSize);
    }

    @Override
    public void geometryChanged() {
        mViewportController.setVisibleRect(getTransformedVisibleRect());
        render();
    }

    @Override
    public IntSize getPageSize() { return mViewportController.getPageSize(); }

    @Override
    public void render() {
        LayerController layerController = getLayerController();
        IntRect visibleRect = layerController.getVisibleRect();
        IntRect tileRect = mViewportController.widenRect(visibleRect);
        tileRect = mViewportController.clampRect(tileRect);

        IntSize pageSize = layerController.getPageSize();
        IntRect viewportRect = mViewportController.transformVisibleRect(tileRect, pageSize);

        
        if (mGeckoVisibleRect == null)
            mGeckoVisibleRect = viewportRect;

        if (!getLayerController().getRedrawHint())
            return;

        

        if (mGeckoVisibleRect.equals(viewportRect)) {
            mWaitingForJSPanZoom = false;
            mJSPanningToRect = null;
            GeckoAppShell.scheduleRedraw();
            return;
        }

        


        if (mWaitingForJSPanZoom && mJSPanningToRect != null &&
                mJSPanningToRect.equals(viewportRect)) {
            return;
        }
            
        

        GeckoAppShell.sendEventToGecko(new GeckoEvent("PanZoom:PanZoom",
            "{\"x\": " + viewportRect.x +
            ", \"y\": " + viewportRect.y +
            ", \"width\": " + LayerController.TILE_WIDTH +
            ", \"height\": " + LayerController.TILE_HEIGHT +
            ", \"zoomFactor\": " + getZoomFactor() + "}"));

        mWaitingForJSPanZoom = true;
        mJSPanningToRect = viewportRect;
    }

    
    private IntRect getTransformedVisibleRect() {
        LayerController layerController = getLayerController();
        return mViewportController.transformVisibleRect(layerController.getVisibleRect(),
                                                        layerController.getPageSize());
    }

    private float getZoomFactor() {
        return 1.0f;    
        



    }
}

