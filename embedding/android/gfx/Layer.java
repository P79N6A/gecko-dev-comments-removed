




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.IntPoint;
import javax.microedition.khronos.opengles.GL10;

public abstract class Layer {
    public IntPoint origin;

    public Layer() {
        origin = new IntPoint(0, 0);
    }

    
    public final void draw(GL10 gl) {
        gl.glPushMatrix();
        gl.glTranslatef(origin.x, origin.y, 0.0f);
        onDraw(gl);
        gl.glPopMatrix();
    }

    




    protected abstract void onDraw(GL10 gl);
}

