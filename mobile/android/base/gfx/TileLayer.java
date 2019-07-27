




package org.mozilla.gecko.gfx;

import android.graphics.Rect;
import android.opengl.GLES20;





public abstract class TileLayer extends Layer {
    private static final String LOGTAG = "GeckoTileLayer";

    public enum PaintMode { NORMAL, REPEAT, STRETCH };
    private PaintMode mPaintMode;

    public TileLayer(IntSize size, PaintMode paintMode) {
        super(size);

        mPaintMode = paintMode;
    }

    protected boolean repeats() { return mPaintMode == PaintMode.REPEAT; }
    protected boolean stretches() { return mPaintMode == PaintMode.STRETCH; }

    public abstract void destroy();

    public void setPaintMode(PaintMode mode) {
        mPaintMode = mode;
    }
}

