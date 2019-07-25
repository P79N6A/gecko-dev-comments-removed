




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.FloatRect;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerRenderer;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.ui.ViewportController;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import android.content.Context;
import android.graphics.Point;
import android.graphics.Rect;
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

    private FloatRect mGeckoVisibleRect;
    

    private Rect mJSPanningToRect;
    

    private boolean mWaitingForJSPanZoom;
    


    private CairoImage mCairoImage;

    
    private static final int PAGE_WIDTH = 980;      
    private static final int PAGE_HEIGHT = 1500;

    public GeckoSoftwareLayerClient(Context context) {
        mContext = context;

        mViewportController = new ViewportController(new IntSize(PAGE_WIDTH, PAGE_HEIGHT),
                                                     new FloatRect(0, 0, 1, 1));

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
        render();
    }

    public void beginDrawing() {
        
    }

    



    public void endDrawing(int x, int y, int width, int height) {
        LayerController controller = getLayerController();
        
        controller.notifyViewOfGeometryChange();

        mViewportController.setVisibleRect(mGeckoVisibleRect);

        if (mGeckoVisibleRect != null) {
            FloatRect layerRect = mViewportController.untransformVisibleRect(mGeckoVisibleRect,
                                                                             getPageSize());
            mTileLayer.origin = layerRect.getOrigin();
        }

        repaint(new Rect(x, y, x + width, y + height));
    }

    private void repaint(Rect rect) {
        mTileLayer.paintSubimage(mCairoImage, rect);
    }

    
    public void jsPanZoomCompleted(Rect rect) {
        mGeckoVisibleRect = new FloatRect(rect);
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
        FloatRect visibleRect = layerController.getVisibleRect();
        FloatRect tileRect = mViewportController.widenRect(visibleRect);
        tileRect = mViewportController.clampRect(tileRect);

        IntSize pageSize = layerController.getPageSize();
        FloatRect viewportRect = mViewportController.transformVisibleRect(tileRect, pageSize);

        
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

        


        int viewportRectX = (int)Math.round(viewportRect.x);
        int viewportRectY = (int)Math.round(viewportRect.y);
        Rect panToRect = new Rect(viewportRectX, viewportRectY,
                                  viewportRectX + LayerController.TILE_WIDTH,
                                  viewportRectY + LayerController.TILE_HEIGHT);

        if (mWaitingForJSPanZoom && mJSPanningToRect != null &&
                mJSPanningToRect.equals(panToRect)) {
            return;
        }

        


        GeckoAppShell.sendEventToGecko(new GeckoEvent("PanZoom:PanZoom",
            "{\"x\": " + panToRect.left + ", \"y\": " + panToRect.top +
            ", \"width\": " + panToRect.width() + ", \"height\": " + panToRect.height() +
            ", \"zoomFactor\": " + getZoomFactor() + "}"));

        mJSPanningToRect = panToRect;
        mWaitingForJSPanZoom = true;
    }

    
    private FloatRect getTransformedVisibleRect() {
        LayerController layerController = getLayerController();
        return mViewportController.transformVisibleRect(layerController.getVisibleRect(),
                                                        layerController.getPageSize());
    }

    private float getZoomFactor() {
        return 1.0f;    
        



    }
}

