




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.FloatSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.PointUtils;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.PointF;
import android.graphics.RectF;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import java.io.File;
import java.nio.ByteBuffer;





public class PlaceholderLayerClient extends LayerClient {
    private static final String LOGTAG = "PlaceholderLayerClient";

    private Context mContext;
    private ViewportMetrics mViewport;
    private boolean mViewportUnknown;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;
    private FetchImageTask mTask;

    private PlaceholderLayerClient(Context context) {
        mContext = context;
        SharedPreferences prefs = GeckoApp.mAppContext.getPlaceholderPrefs();
        mViewportUnknown = true;
        if (prefs.contains("viewport")) {
            try {
                JSONObject viewportObject = new JSONObject(prefs.getString("viewport", null));
                mViewport = new ViewportMetrics(viewportObject);
                mViewportUnknown = false;
            } catch (JSONException e) {
                Log.e(LOGTAG, "Error parsing saved viewport!");
                mViewport = new ViewportMetrics();
            }
        } else {
            mViewport = new ViewportMetrics();
        }
    }

    public static PlaceholderLayerClient createInstance(Context context) {
        return new PlaceholderLayerClient(context);
    }

    public void destroy() {
        if (mTask != null) {
            mTask.cancel(false);
            mTask = null;
        }
    }

    private class FetchImageTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... unused) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inScaled = false;
            Bitmap bitmap = BitmapFactory.decodeFile(GeckoApp.getStartupBitmapFilePath(),
                                                     options);
            if (bitmap == null)
                return null;

            Bitmap.Config config = bitmap.getConfig();

            mWidth = bitmap.getWidth();
            mHeight = bitmap.getHeight();
            mFormat = CairoUtils.bitmapConfigToCairoFormat(config);

            int bpp = CairoUtils.bitsPerPixelForCairoFormat(mFormat) / 8;
            mBuffer = ByteBuffer.allocateDirect(mWidth * mHeight * bpp);

            bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());

            if (mViewportUnknown) {
                mViewport.setPageSize(new FloatSize(mWidth, mHeight));
                if (getLayerController() != null)
                    getLayerController().setPageSize(mViewport.getPageSize());
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void unused) {
            BufferedCairoImage image = new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat);
            SingleTileLayer tileLayer = new SingleTileLayer(image);

            tileLayer.beginTransaction();
            tileLayer.setOrigin(PointUtils.round(mViewport.getDisplayportOrigin()));
            tileLayer.endTransaction();

            getLayerController().setRoot(tileLayer);
        }
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
        layerController.setRoot(tileLayer);
    }
}

