




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import java.io.ByteArrayInputStream;
import java.nio.ByteBuffer;





public class PlaceholderLayerClient {
    private static final String LOGTAG = "PlaceholderLayerClient";

    private final LayerController mLayerController;

    private ViewportMetrics mViewport;
    private boolean mViewportUnknown;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;

    public PlaceholderLayerClient(LayerController controller, String lastViewport) {
        mLayerController = controller;

        mViewportUnknown = true;
        if (lastViewport != null) {
            try {
                JSONObject viewportObject = new JSONObject(lastViewport);
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


        if (mViewportUnknown)
            mViewport.setViewport(mLayerController.getViewport());
        mLayerController.setViewportMetrics(mViewport);

        BufferedCairoImage image = new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat);
        SingleTileLayer tileLayer = new SingleTileLayer(image);

        tileLayer.beginTransaction();   
        try {
            Point origin = PointUtils.round(mViewport.getOrigin());
            tileLayer.setPosition(new Rect(origin.x, origin.y, origin.x + mWidth, origin.y + mHeight));
        } finally {
            tileLayer.endTransaction();
        }

        mLayerController.setRoot(tileLayer);
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
            mLayerController.setPageSize(mViewport.getPageSize());
        }

        return true;
    }
}
