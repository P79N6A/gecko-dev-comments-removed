



package org.mozilla.gecko.favicons.decoders;




public class IconDirectoryEntry implements Comparable<IconDirectoryEntry> {

    public static int sMaxBPP;

    int mWidth;
    int mHeight;
    int mPaletteSize;
    int mBitsPerPixel;
    int mPayloadSize;
    int mPayloadOffset;
    boolean mPayloadIsPNG;

    
    int mIndex;
    boolean mIsErroneous;

    public IconDirectoryEntry(int width, int height, int paletteSize, int bitsPerPixel, int payloadSize, int payloadOffset, boolean payloadIsPNG) {
        mWidth = width;
        mHeight = height;
        mPaletteSize = paletteSize;
        mBitsPerPixel = bitsPerPixel;
        mPayloadSize = payloadSize;
        mPayloadOffset = payloadOffset;
        mPayloadIsPNG = payloadIsPNG;
    }

    




    public static IconDirectoryEntry getErroneousEntry() {
        IconDirectoryEntry ret = new IconDirectoryEntry(-1, -1, -1, -1, -1, -1, false);
        ret.mIsErroneous = true;

        return ret;
    }

    










    public static IconDirectoryEntry createFromBuffer(byte[] buffer, int regionOffset, int regionLength, int entryOffset) {
        
        if (buffer[entryOffset + 3] != 0) {
            return getErroneousEntry();
        }

        
        int fieldPtr = entryOffset + 8;
        int entryLength = (buffer[fieldPtr] & 0xFF) |
                          (buffer[fieldPtr + 1] & 0xFF) << 8 |
                          (buffer[fieldPtr + 2] & 0xFF) << 16 |
                          (buffer[fieldPtr + 3] & 0xFF) << 24;

        
        fieldPtr += 4;

        int payloadOffset = (buffer[fieldPtr] & 0xFF) |
                            (buffer[fieldPtr + 1] & 0xFF) << 8 |
                            (buffer[fieldPtr + 2] & 0xFF) << 16 |
                            (buffer[fieldPtr + 3] & 0xFF) << 24;

        
        if (payloadOffset < 0 || entryLength < 0 || payloadOffset + entryLength > regionOffset + regionLength) {
            return getErroneousEntry();
        }

        
        int imageWidth = buffer[entryOffset] & 0xFF;
        int imageHeight = buffer[entryOffset+1] & 0xFF;

        
        if (imageWidth == 0) {
            imageWidth = 256;
        }

        if (imageHeight == 0) {
            imageHeight = 256;
        }

        
        int paletteSize = buffer[entryOffset + 2] & 0xFF;

        
        int colorPlanes = buffer[entryOffset + 4] & 0xFF;

        int bitsPerPixel = (buffer[entryOffset + 6] & 0xFF) |
                           (buffer[entryOffset + 7] & 0xFF) << 8;

        if (colorPlanes > 1) {
            bitsPerPixel *= colorPlanes;
        }

        
        boolean payloadIsPNG = FaviconDecoder.bufferStartsWith(buffer, FaviconDecoder.ImageMagicNumbers.PNG.value, regionOffset + payloadOffset);

        return new IconDirectoryEntry(imageWidth, imageHeight, paletteSize, bitsPerPixel, entryLength, payloadOffset, payloadIsPNG);
    }

    


    public int getOffset() {
        return ICODecoder.ICO_HEADER_LENGTH_BYTES + (mIndex * ICODecoder.ICO_ICONDIRENTRY_LENGTH_BYTES);
    }

    @Override
    public int compareTo(IconDirectoryEntry another) {
        if (mWidth > another.mWidth) {
            return 1;
        }

        if (mWidth < another.mWidth) {
            return -1;
        }

        
        if (mBitsPerPixel >= sMaxBPP && another.mBitsPerPixel >= sMaxBPP) {
            if (mBitsPerPixel < another.mBitsPerPixel) {
                return 1;
            }

            if (mBitsPerPixel > another.mBitsPerPixel) {
                return -1;
            }
        }

        
        if (mBitsPerPixel > another.mBitsPerPixel) {
            return 1;
        }

        if (mBitsPerPixel < another.mBitsPerPixel) {
            return -1;
        }

        
        if (mPaletteSize > another.mPaletteSize) {
            return 1;
        }

        if (mPaletteSize < another.mPaletteSize) {
            return -1;
        }

        
        if (mPayloadSize < another.mPayloadSize) {
            return 1;
        }

        if (mPayloadSize > another.mPayloadSize) {
            return -1;
        }

        
        if (mPayloadIsPNG && !another.mPayloadIsPNG) {
            return 1;
        }

        if (!mPayloadIsPNG && another.mPayloadIsPNG) {
            return -1;
        }

        return 0;
    }

    public static void setMaxBPP(int maxBPP) {
        sMaxBPP = maxBPP;
    }

    @Override
    public String toString() {
        return "IconDirectoryEntry{" +
                "\nmWidth=" + mWidth +
                ", \nmHeight=" + mHeight +
                ", \nmPaletteSize=" + mPaletteSize +
                ", \nmBitsPerPixel=" + mBitsPerPixel +
                ", \nmPayloadSize=" + mPayloadSize +
                ", \nmPayloadOffset=" + mPayloadOffset +
                ", \nmPayloadIsPNG=" + mPayloadIsPNG +
                ", \nmIndex=" + mIndex +
                '}';
    }
}
