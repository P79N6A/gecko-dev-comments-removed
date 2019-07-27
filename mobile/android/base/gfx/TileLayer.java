




package org.mozilla.gecko.gfx;

import android.graphics.Rect;
import android.opengl.GLES20;





public abstract class TileLayer extends Layer {
    private static final String LOGTAG = "GeckoTileLayer";

    public TileLayer(IntSize size) {
        super(size);
    }

    public abstract void destroy();
}

