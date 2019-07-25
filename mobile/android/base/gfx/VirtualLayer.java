





































package org.mozilla.gecko.gfx;

import android.graphics.Point;

public class VirtualLayer extends Layer {
    private Listener mListener;
    private IntSize mSize;

    public void setListener(Listener listener) {
        mListener = listener;
    }

    @Override
    public void draw(RenderContext context) {
        
    }

    @Override
    public IntSize getSize() {
        return mSize;
    }

    public void setSize(IntSize size) {
        mSize = size;
    }

    @Override
    protected boolean performUpdates(RenderContext context) {
        boolean dimensionsChanged = dimensionChangesPending();
        boolean result = super.performUpdates(context);
        if (dimensionsChanged && mListener != null) {
            mListener.dimensionsChanged(getOrigin(), getResolution());
        }

        return result;
    }

    public interface Listener {
        void dimensionsChanged(Point newOrigin, float newResolution);
    }
}

