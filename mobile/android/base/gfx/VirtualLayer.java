





































package org.mozilla.gecko.gfx;

import android.graphics.Rect;

public class VirtualLayer extends Layer {
    public VirtualLayer(IntSize size) {
        super(size);
    }

    @Override
    public void draw(RenderContext context) {
        
    }

    void setPositionAndResolution(Rect newPosition, float newResolution) {
        
        
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        mPosition = newPosition;
        mResolution = newResolution;
    }
}
