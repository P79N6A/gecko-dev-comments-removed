





































package org.mozilla.gecko.gfx;

public class VirtualLayer extends Layer {
    private IntSize mSize;

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
}

