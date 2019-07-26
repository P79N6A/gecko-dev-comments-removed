



package org.mozilla.gecko.favicons.decoders;




public class IconDirectoryEntry implements Comparable<IconDirectoryEntry> {

    public static int maxBPP;

    int width;
    int height;
    int paletteSize;
    int bitsPerPixel;
    int payloadSize;
    int payloadOffset;
    boolean payloadIsPNG;

    
    int index;
    boolean isErroneous;

    public IconDirectoryEntry(int width, int height, int paletteSize, int bitsPerPixel, int payloadSize, int payloadOffset, boolean payloadIsPNG) {
        this.width = width;
        this.height = height;
        this.paletteSize = paletteSize;
        this.bitsPerPixel = bitsPerPixel;
        this.payloadSize = payloadSize;
        this.payloadOffset = payloadOffset;
        this.payloadIsPNG = payloadIsPNG;
    }

    




    public static IconDirectoryEntry getErroneousEntry() {
        IconDirectoryEntry ret = new IconDirectoryEntry(-1, -1, -1, -1, -1, -1, false);
        ret.isErroneous = true;

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
        return ICODecoder.ICO_HEADER_LENGTH_BYTES + (index * ICODecoder.ICO_ICONDIRENTRY_LENGTH_BYTES);
    }

    @Override
    public int compareTo(IconDirectoryEntry another) {
        if (width > another.width) {
            return 1;
        }

        if (width < another.width) {
            return -1;
        }

        
        if (bitsPerPixel >= maxBPP && another.bitsPerPixel >= maxBPP) {
            if (bitsPerPixel < another.bitsPerPixel) {
                return 1;
            }

            if (bitsPerPixel > another.bitsPerPixel) {
                return -1;
            }
        }

        
        if (bitsPerPixel > another.bitsPerPixel) {
            return 1;
        }

        if (bitsPerPixel < another.bitsPerPixel) {
            return -1;
        }

        
        if (paletteSize > another.paletteSize) {
            return 1;
        }

        if (paletteSize < another.paletteSize) {
            return -1;
        }

        
        if (payloadSize < another.payloadSize) {
            return 1;
        }

        if (payloadSize > another.payloadSize) {
            return -1;
        }

        
        if (payloadIsPNG && !another.payloadIsPNG) {
            return 1;
        }

        if (!payloadIsPNG && another.payloadIsPNG) {
            return -1;
        }

        return 0;
    }

    public static void setMaxBPP(int maxBPP) {
        IconDirectoryEntry.maxBPP = maxBPP;
    }

    @Override
    public String toString() {
        return "IconDirectoryEntry{" +
                "\nwidth=" + width +
                ", \nheight=" + height +
                ", \npaletteSize=" + paletteSize +
                ", \nbitsPerPixel=" + bitsPerPixel +
                ", \npayloadSize=" + payloadSize +
                ", \npayloadOffset=" + payloadOffset +
                ", \npayloadIsPNG=" + payloadIsPNG +
                ", \nindex=" + index +
                '}';
    }
}
