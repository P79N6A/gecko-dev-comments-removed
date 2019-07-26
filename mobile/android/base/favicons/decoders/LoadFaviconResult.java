



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

        
        
        
        
        
        if (isICO) {
            return faviconBytes;
        }

        
        
        final Bitmap favicon = ((FaviconDecoder.SingleBitmapIterator) bitmapsDecoded).peek();
        final ByteArrayOutputStream stream = new ByteArrayOutputStream();

        try {
            if (favicon.compress(Bitmap.CompressFormat.PNG, 100, stream)) {
                return stream.toByteArray();
            }
        } catch (OutOfMemoryError e) {
            Log.w(LOGTAG, "Out of memory re-compressing favicon.");
        }

        Log.w(LOGTAG, "Favicon re-compression failed.");
        return null;
    }

}
