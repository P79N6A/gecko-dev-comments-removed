





































package org.mozilla.gecko.gfx;

import android.graphics.Rect;

public class VirtualLayer extends Layer {
    public VirtualLayer(IntSize size) {
        super(size);
    }

    @Override
    public void draw(RenderContext context) {
        
    }

    void setPositionAndResolution(int left, int top, int right, int bottom, float newResolution) {
        
        
        
        
        
        
        
        
        

        
        
        
        
        mPosition.set(left, top, right, bottom);
        mResolution = newResolution;
    }
}
