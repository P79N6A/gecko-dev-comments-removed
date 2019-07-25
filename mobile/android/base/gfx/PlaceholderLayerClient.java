




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;
import java.io.File;
import java.nio.ByteBuffer;





public class PlaceholderLayerClient extends LayerClient {
    private Context mContext;
    private ViewportMetrics mViewport;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;
    private FetchImageTask mTask;

    private PlaceholderLayerClient(Context context) {
        mContext = context;
        SharedPreferences prefs = GeckoApp.mAppContext.getPlaceholderPrefs();
        IntSize pageSize = new IntSize(prefs.getInt("page-width", 995),
                                       prefs.getInt("page-height", 1250));
        Rect viewport = new Rect(prefs.getInt("viewport-left", 0),
                                 prefs.getInt("viewport-top", 0),
                                 prefs.getInt("viewport-right", 1),
                                 prefs.getInt("viewport-bottom", 1));
        Point offset = new Point(prefs.getInt("viewport-offset-x", 0),
                                 prefs.getInt("viewport-offset-y", 0));

        mViewport = new ViewportMetrics();
        mViewport.setPageSize(pageSize);
        mViewport.setViewport(viewport);
        mViewport.setViewportOffset(offset);
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
            return null;
        }

        @Override
        protected void onPostExecute(Void unused) {
            BufferedCairoImage image = new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat);
            SingleTileLayer tileLayer = new SingleTileLayer(image);

            tileLayer.beginTransaction();
            tileLayer.setOrigin(mViewport.getDisplayportOrigin());
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

        layerController.setViewportMetrics(mViewport);

        BufferedCairoImage image = new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat);
        SingleTileLayer tileLayer = new SingleTileLayer(image);
        layerController.setRoot(tileLayer);
    }
}

