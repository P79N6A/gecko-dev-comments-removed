




































package org.mozilla.gecko.gfx;

import android.graphics.PointF;
import javax.microedition.khronos.opengles.GL10;

public abstract class Layer {
    public PointF origin;

    public Layer() {
        origin = new PointF(0.0f, 0.0f);
    }

    
    public final void draw(GL10 gl) {
        gl.glPushMatrix();
        gl.glTranslatef(origin.x, origin.y, 0.0f);
        onDraw(gl);
        gl.glPopMatrix();
    }

    




    protected abstract void onDraw(GL10 gl);
}

