



package org.mozilla.gecko.favicons.decoders;

import android.graphics.Bitmap;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.util.Iterator;









public class LoadFaviconResult {
    private static final String LOGTAG = "LoadFaviconResult";

    byte[] faviconBytes;
    int offset;
    int length;

    boolean isICO;
    Iterator<Bitmap> bitmapsDecoded;

    public Iterator<Bitmap> getBitmaps() {
        return bitmapsDecoded;
    }

    





    public byte[] getBytesForDatabaseStorage() {
        
        if (offset != 0 || length != faviconBytes.length) {
            final byte[] normalised = new byte[length];
            System.arraycopy(faviconBytes, offset, normalised, 0, length);
            offset = 0;
            faviconBytes = normalised;
        }

        
        
        if (!isICO) {
            Bitmap favicon = ((FaviconDecoder.SingleBitmapIterator) bitmapsDecoded).peek();
            byte[] data = null;
            ByteArrayOutputStream stream = new ByteArrayOutputStream();

            if (favicon.compress(Bitmap.CompressFormat.PNG, 100, stream)) {
                data = stream.toByteArray();
            } else {
                Log.w(LOGTAG, "Favicon compression failed.");
            }

            return data;
        }

        
        
        
        
        
        return faviconBytes;
    }

}
