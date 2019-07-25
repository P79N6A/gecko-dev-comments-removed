




































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
import android.os.Environment;
import android.util.Log;
import java.io.File;
import java.nio.ByteBuffer;





public class PlaceholderLayerClient extends LayerClient {
    private Context mContext;
    private IntSize mPageSize;
    private int mWidth, mHeight, mFormat;
    private ByteBuffer mBuffer;

    private PlaceholderLayerClient(Context context, Bitmap bitmap) {
        mContext = context;
        mPageSize = new IntSize(995, 1250); 

        mWidth = bitmap.getWidth();
        mHeight = bitmap.getHeight();
        mFormat = CairoUtils.bitmapConfigToCairoFormat(bitmap.getConfig());
        mBuffer = ByteBuffer.allocateDirect(mWidth * mHeight * 4);
        bitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }

    public static PlaceholderLayerClient createInstance(Context context) {
        File path = new File(GeckoApp.getStartupBitmapFilePath());
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inScaled = false;
        Bitmap bitmap = BitmapFactory.decodeFile("" + path, options);
        if (bitmap == null)
            return null;

        return new PlaceholderLayerClient(context, bitmap);
    }

    public void init() {
        SingleTileLayer tileLayer = new SingleTileLayer();
        getLayerController().setRoot(tileLayer);
        tileLayer.paintImage(new BufferedCairoImage(mBuffer, mWidth, mHeight, mFormat));
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

