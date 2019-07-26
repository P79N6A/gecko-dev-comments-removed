



package org.mozilla.gecko.favicons.decoders;

import android.graphics.Bitmap;
import org.mozilla.gecko.gfx.BitmapUtils;

import java.util.Iterator;
import java.util.NoSuchElementException;




public class FaviconDecoder {
    static enum ImageMagicNumbers {
        
        PNG(new byte[] {(byte) (0x89 & 0xFF), 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}),
        GIF(new byte[] {0x47, 0x49, 0x46, 0x38}),
        JPEG(new byte[] {-0x1, -0x28, -0x1, -0x20}),
        BMP(new byte[] {0x42, 0x4d}),
        WEB(new byte[] {0x57, 0x45, 0x42, 0x50, 0x0a});

        public byte[] value;

        private ImageMagicNumbers(byte[] value) {
            this.value = value;
        }
    }

    






    private static boolean isDecodableByAndroid(byte[] buffer, int offset) {
        for (ImageMagicNumbers m : ImageMagicNumbers.values()) {
            if (bufferStartsWith(buffer, m.value, offset)) {
                return true;
            }
        }

        return false;
    }

    









    static boolean bufferStartsWith(byte[] buffer, byte[] test, int bufferOffset) {
        if (buffer.length < test.length) {
            return false;
        }

        for (int i = 0; i < test.length; ++i) {
            if (buffer[bufferOffset + i] != test[i]) {
                return false;
            }
        }
        return true;
    }

    










    public static LoadFaviconResult decodeFavicon(byte[] buffer, int offset, int length) {
        LoadFaviconResult result;
        if (isDecodableByAndroid(buffer, offset)) {
            result = new LoadFaviconResult();
            result.mOffset = offset;
            result.mLength = length;
            result.mIsICO = false;

            
            
            result.mBitmapsDecoded = new SingleBitmapIterator(BitmapUtils.decodeByteArray(buffer, offset, length));
            result.mFaviconBytes = buffer;

            return result;
        }

        
        ICODecoder decoder = new ICODecoder(buffer, offset, length);

        result = decoder.decode();

        if (result == null) {
            return null;
        }

        return result;
    }

    public static LoadFaviconResult decodeFavicon(byte[] buffer) {
        return decodeFavicon(buffer, 0, buffer.length);
    }

    


    static class SingleBitmapIterator implements Iterator<Bitmap> {
        private Bitmap mBitmap;

        public SingleBitmapIterator(Bitmap b) {
            mBitmap = b;
        }

        






        public Bitmap peek() {
            return mBitmap;
        }

        @Override
        public boolean hasNext() {
            return mBitmap != null;
        }

        @Override
        public Bitmap next() {
            if (mBitmap == null) {
                throw new NoSuchElementException("Element already returned from SingleBitmapIterator.");
            }

            Bitmap ret = mBitmap;
            mBitmap = null;
            return ret;
        }

        @Override
        public void remove() {
            throw new UnsupportedOperationException("remove() not supported on SingleBitmapIterator.");
        }
    }
}
