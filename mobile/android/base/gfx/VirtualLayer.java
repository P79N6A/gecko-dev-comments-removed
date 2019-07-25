





































package org.mozilla.gecko.gfx;

import android.graphics.Rect;

public class VirtualLayer extends Layer {
    public VirtualLayer(IntSize size) {
        super(size);
    }

    @Override
    public void draw(RenderContext context) {
        
    }

    void setMetrics(Rect newPosition, Rect newDisplayPort, float newResolution) {
        
        
        
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        mPosition = newPosition;
        mDisplayPort = newDisplayPort;
        mResolution = newResolution;
    }
}
