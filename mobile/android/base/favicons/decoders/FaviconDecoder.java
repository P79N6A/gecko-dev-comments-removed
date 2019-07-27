



package org.mozilla.gecko.favicons.decoders;

import android.graphics.Bitmap;
import android.util.Base64;
import android.util.Log;

import org.mozilla.gecko.gfx.BitmapUtils;

import java.util.Iterator;
import java.util.NoSuchElementException;




public class FaviconDecoder {
    private static final String LOG_TAG = "GeckoFaviconDecoder";

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
            result.offset = offset;
            result.length = length;
            result.isICO = false;

            Bitmap decodedImage = BitmapUtils.decodeByteArray(buffer, offset, length);
            if (decodedImage == null) {
                
                return null;
            }

            
            
            result.bitmapsDecoded = new SingleBitmapIterator(decodedImage);
            result.faviconBytes = buffer;

            return result;
        }

        
        ICODecoder decoder = new ICODecoder(buffer, offset, length);

        result = decoder.decode();

        if (result == null) {
            return null;
        }

        return result;
    }

    public static LoadFaviconResult decodeDataURI(String uri) {
        if (uri == null) {
            Log.w(LOG_TAG, "Can't decode null data: URI.");
            return null;
        }

        if (!uri.startsWith("data:image/")) {
            
            return null;
        }

        
        int offset = uri.indexOf(',') + 1;
        if (offset == 0) {
            Log.w(LOG_TAG, "No ',' in data: URI; malformed?");
            return null;
        }

        try {
            String base64 = uri.substring(offset);
            byte[] raw = Base64.decode(base64, Base64.DEFAULT);
            return decodeFavicon(raw);
        } catch (Exception e) {
            Log.w(LOG_TAG, "Couldn't decode data: URI.", e);
            return null;
        }
    }

    public static LoadFaviconResult decodeFavicon(byte[] buffer) {
        return decodeFavicon(buffer, 0, buffer.length);
    }

    









    public static Bitmap getMostSuitableBitmapFromDataURI(String iconURI, int desiredWidth) {
        LoadFaviconResult result = FaviconDecoder.decodeDataURI(iconURI);
        if (result == null) {
            
            Log.w(LOG_TAG, "Unable to decode icon URI.");
            return null;
        }

        final Iterator<Bitmap> bitmaps = result.getBitmaps();
        if (!bitmaps.hasNext()) {
            Log.w(LOG_TAG, "No bitmaps in decoded icon.");
            return null;
        }

        Bitmap bitmap = bitmaps.next();
        if (!bitmaps.hasNext()) {
            
            return bitmap;
        }

        
        int currentWidth = bitmap.getWidth();
        while ((currentWidth < desiredWidth) &&
               bitmaps.hasNext()) {
            final Bitmap b = bitmaps.next();
            if (b.getWidth() > currentWidth) {
                currentWidth = b.getWidth();
                bitmap = b;
            }
        }

        return bitmap;
    }

    


    static class SingleBitmapIterator implements Iterator<Bitmap> {
        private Bitmap bitmap;

        public SingleBitmapIterator(Bitmap b) {
            bitmap = b;
        }

        






        public Bitmap peek() {
            return bitmap;
        }

        @Override
        public boolean hasNext() {
            return bitmap != null;
        }

        @Override
        public Bitmap next() {
            if (bitmap == null) {
                throw new NoSuchElementException("Element already returned from SingleBitmapIterator.");
            }

            Bitmap ret = bitmap;
            bitmap = null;
            return ret;
        }

        @Override
        public void remove() {
            throw new UnsupportedOperationException("remove() not supported on SingleBitmapIterator.");
        }
    }
}
