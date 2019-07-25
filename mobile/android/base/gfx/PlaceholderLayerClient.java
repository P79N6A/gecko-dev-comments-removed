




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerClient;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.GeckoApp;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.AsyncTask;
import android.os.Environment;
import android.util.Log;
import java.io.File;
import java.nio.ByteBuffer;





public class PlaceholderLayerClient extends LayerClient {
    private Context mContext;
    private IntSize mPageSize;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;
    private FetchImageTask mTask;

    private PlaceholderLayerClient(Context context) {
        mContext = context;
        mPageSize = new IntSize(995, 1250); 
    }

    public static PlaceholderLayerClient createInstance(Context context) {
        return new PlaceholderLayerClient(context);
    }

    public void init() {
        
        
        
        
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
            getLayerController().setRoot(tileLayer);
        }
    }

    @Override
    public void geometryChanged() {  }
    @Override
    public IntSize getPageSize() { return mPageSize; }
    @Override
    public void render() {  }

    
    @Override
    public void setPageSize(IntSize pageSize) { mPageSize = pageSize; }
}

