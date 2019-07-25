




































package org.mozilla.gecko.gfx;

import java.nio.ByteBuffer;




public abstract class CairoImage {
    public abstract ByteBuffer lockBuffer();
    public void unlockBuffer() {  }

    public abstract int getWidth();
    public abstract int getHeight();
    public abstract int getFormat();

    public static final int FORMAT_INVALID = -1;
    public static final int FORMAT_ARGB32 = 0;
    public static final int FORMAT_RGB24 = 1;
    public static final int FORMAT_A8 = 2;
    public static final int FORMAT_A1 = 3;
    public static final int FORMAT_RGB16_565 = 4;
}

