




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PointF;
import android.graphics.RectF;
import android.os.Environment;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import java.io.File;
import java.io.ByteArrayInputStream;
import java.nio.ByteBuffer;





public class PlaceholderLayerClient extends LayerClient {
    private static final String LOGTAG = "PlaceholderLayerClient";

    private Context mContext;
    private ViewportMetrics mViewport;
    private boolean mViewportUnknown;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;

    private PlaceholderLayerClient(Context context) {
        mContext = context;
        String viewport = GeckoApp.mAppContext.getLastViewport();
        mViewportUnknown = true;
        if (viewport != null) {
            try {
                JSONObject viewportObject = new JSONObject(viewport);
                mViewport = new ViewportMetrics(viewportObject);
                mViewportUnknown = false;
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error parsing saved viewport!");
                mViewport = new ViewportMetrics();
            }
        } else {
            mViewport = new ViewportMetrics();
        }
        loadScreenshot();
    }

    public static PlaceholderLayerClient createInstance(Context context) {
        return new PlaceholderLayerClient(context);
    }

    public void destroy() {
        if (mBuffer != null) {
            GeckoAppShell.freeDirectBuffer(mBuffer);
            mBuffer = null;
        }
    }

    boolean loadScreenshot() {
            if (GeckoApp.mAppContext.mLastScreen == null)
                return false;
            Bitmap bitmap = BitmapFactory.decodeStream(new ByteArrayInputStream(GeckoApp.mAppContext.mLastScreen));
            if (bitmap == null)
                return false;

            Bitmap.Config config = bitmap.getConfig();

            mWidth = bitmap.getWidth();
            mHeight = bitmap.getHeight();
            mFormat = CairoUtils.bitmapConfigToCairoFormat(config);

            int bpp = CairoUtils.bitsPerPixelForCairoFormat(mFormat) / 8;
            mBuffer = GeckoAppShell.allocateDirectBuffer(mWidth * mHeight * bpp);

            bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());

            if (mViewportUnknown) {
                mViewport.setPageSize(new FloatSize(mWidth, mHeight));
                if (getLayerController() != null)
                    getLayerController().setPageSize(mViewport.getPageSize());
            }

            return true;
        }

    void showScreenshot() {
            BufferedCairoImage image = new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat);
            SingleTileLayer tileLayer = new SingleTileLayer(image);

            beginTransaction(tileLayer);
            tileLayer.setOrigin(PointUtils.round(mViewport.getDisplayportOrigin()));
            endTransaction(tileLayer);

            getLayerController().setRoot(tileLayer);
        }

    @Override
    public void geometryChanged() {  }
    @Override
    public void render() {  }

    @Override
    public void setLayerController(LayerController layerController) {
        super.setLayerController(layerController);

        if (mViewportUnknown)
            mViewport.setViewport(layerController.getViewport());
        layerController.setViewportMetrics(mViewport);

        BufferedCairoImage image = new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat);
        SingleTileLayer tileLayer = new SingleTileLayer(image);

        beginTransaction(tileLayer);
        tileLayer.setOrigin(PointUtils.round(mViewport.getDisplayportOrigin()));
        endTransaction(tileLayer);

        layerController.setRoot(tileLayer);
    }
}
