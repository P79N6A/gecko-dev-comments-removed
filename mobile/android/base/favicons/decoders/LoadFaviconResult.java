



package org.mozilla.gecko.favicons.decoders;

import android.graphics.Bitmap;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.util.Iterator;









public class LoadFaviconResult {
    private static final String LOGTAG = "LoadFaviconResult";

    byte[] mFaviconBytes;
    int mOffset;
    int mLength;

    boolean mIsICO;
    Iterator<Bitmap> mBitmapsDecoded;

    public Iterator<Bitmap> getBitmaps() {
        return mBitmapsDecoded;
    }

    





    public byte[] getBytesForDatabaseStorage() {
        
        if (mOffset != 0 || mLength != mFaviconBytes.length) {
            final byte[] normalised = new byte[mLength];
            System.arraycopy(mFaviconBytes, mOffset, normalised, 0, mLength);
            mOffset = 0;
            mFaviconBytes = normalised;
        }

        
        
        if (!mIsICO) {
            Bitmap favicon = ((FaviconDecoder.SingleBitmapIterator) mBitmapsDecoded).peek();
            byte[] data = null;
            ByteArrayOutputStream stream = new ByteArrayOutputStream();

            if (favicon.compress(Bitmap.CompressFormat.PNG, 100, stream)) {
                data = stream.toByteArray();
            } else {
                Log.w(LOGTAG, "Favicon compression failed.");
            }

            return data;
        }

        
        
        
        
        
        return mFaviconBytes;
    }

}
