




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.CairoImage;
import org.mozilla.gecko.gfx.CairoUtils;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.TileLayer;
import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import javax.microedition.khronos.opengles.GL10;




public class SingleTileLayer extends TileLayer {
    private FloatBuffer mTexCoordBuffer, mVertexBuffer;

    private static final float[] VERTICES = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f
    };

    private static final float[] TEX_COORDS = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    public SingleTileLayer() { this(false); }

    public SingleTileLayer(boolean repeat) {
        super(repeat);

        mVertexBuffer = createBuffer(VERTICES);
        mTexCoordBuffer = createBuffer(TEX_COORDS);
    }

    @Override
    protected void onTileDraw(GL10 gl) {
        IntSize size = getSize();

        if (repeats()) {
            gl.glMatrixMode(GL10.GL_TEXTURE);
            gl.glPushMatrix();
            gl.glScalef(LayerController.TILE_WIDTH / size.width,
                        LayerController.TILE_HEIGHT / size.height,
                        1.0f);

            gl.glMatrixMode(GL10.GL_MODELVIEW);
            gl.glScalef(LayerController.TILE_WIDTH, LayerController.TILE_HEIGHT, 1.0f);
        } else {
            gl.glScalef(size.width, size.height, 1.0f);
        }

        gl.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());
        drawTriangles(gl, mVertexBuffer, mTexCoordBuffer, 4);

        if (repeats()) {
            gl.glMatrixMode(GL10.GL_TEXTURE);
            gl.glPopMatrix();
            gl.glMatrixMode(GL10.GL_MODELVIEW);
        }
    }
}

