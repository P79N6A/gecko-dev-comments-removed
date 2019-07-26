



package org.mozilla.gecko;

import org.mozilla.gecko.mozglue.generatorannotations.WrapEntireClassForJNI;

import java.nio.ByteBuffer;

@WrapEntireClassForJNI
public class SurfaceBits {
    public int width;
    public int height;
    public int format;
    public ByteBuffer buffer;
}
