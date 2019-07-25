




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.LayerController;
import org.mozilla.fennec.gfx.TileLayer;
import javax.microedition.khronos.opengles.GL10;
import java.nio.FloatBuffer;







public class NinePatchTileLayer extends TileLayer {
    private FloatBuffer mSideTexCoordBuffer, mSideVertexBuffer;
    private FloatBuffer mTopTexCoordBuffer, mTopVertexBuffer;
    private LayerController mLayerController;

    private static final int PATCH_SIZE = 16;
    private static final int TEXTURE_SIZE = 48;

    














    private static final float[] SIDE_TEX_COORDS = {
        0.0f,   0.0f,
        0.25f,  0.0f,
        0.0f,   0.25f,
        0.25f,  0.25f,
        0.0f,   0.50f,
        0.25f,  0.50f,
        0.0f,   0.75f,
        0.25f,  0.75f,
    };

    private static final float[] TOP_TEX_COORDS = {
        0.25f,  0.0f,
        0.50f,  0.0f,
        0.25f,  0.25f,
        0.50f,  0.25f,
    };

    public NinePatchTileLayer(LayerController layerController) {
        super(false);

        mLayerController = layerController;

        mSideTexCoordBuffer = createBuffer(SIDE_TEX_COORDS);
        mTopTexCoordBuffer = createBuffer(TOP_TEX_COORDS);

        recreateVertexBuffers();
    }

    public void recreateVertexBuffers() {
        IntSize pageSize = mLayerController.getPageSize();

        float[] sideVertices = {
            -PATCH_SIZE,    -PATCH_SIZE,                    0.0f,
            0.0f,           -PATCH_SIZE,                    0.0f,
            -PATCH_SIZE,    0.0f,                           0.0f,
            0.0f,           0.0f,                           0.0f,
            -PATCH_SIZE,    pageSize.height,                0.0f,
            0.0f,           pageSize.height,                0.0f,
            -PATCH_SIZE,    PATCH_SIZE + pageSize.height,   0.0f,
            0.0f,           PATCH_SIZE + pageSize.height,   0.0f
        };

        float[] topVertices = {
            0.0f,           -PATCH_SIZE,    0.0f,
            pageSize.width, -PATCH_SIZE,    0.0f,
            0.0f,           0.0f,           0.0f,
            pageSize.width, 0.0f,           0.0f
        };

        mSideVertexBuffer = createBuffer(sideVertices);
        mTopVertexBuffer = createBuffer(topVertices);
    }

    @Override
    protected void onTileDraw(GL10 gl) {
        IntSize pageSize = mLayerController.getPageSize();

        gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
        gl.glEnable(GL10.GL_BLEND);

        gl.glBindTexture(GL10.GL_TEXTURE_2D, getTextureID());

        
        drawTriangles(gl, mSideVertexBuffer, mSideTexCoordBuffer, 8);

        
        drawTriangles(gl, mTopVertexBuffer, mTopTexCoordBuffer, 4);

        
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glPushMatrix();
        gl.glTranslatef(pageSize.width + PATCH_SIZE, 0.0f, 0.0f);
        gl.glMatrixMode(GL10.GL_TEXTURE);
        gl.glPushMatrix();
        gl.glTranslatef(0.50f, 0.0f, 0.0f);

        drawTriangles(gl, mSideVertexBuffer, mSideTexCoordBuffer, 8);

        gl.glMatrixMode(GL10.GL_TEXTURE);
        gl.glPopMatrix();
        gl.glMatrixMode(GL10.GL_MODELVIEW);     
        gl.glPopMatrix();

        
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glPushMatrix();
        gl.glTranslatef(0.0f, pageSize.height + PATCH_SIZE, 0.0f);
        gl.glMatrixMode(GL10.GL_TEXTURE);
        gl.glPushMatrix();
        gl.glTranslatef(0.0f, 0.50f, 0.0f);

        drawTriangles(gl, mTopVertexBuffer, mTopTexCoordBuffer, 4);

        gl.glMatrixMode(GL10.GL_TEXTURE);
        gl.glPopMatrix();
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glPopMatrix();

        gl.glDisable(GL10.GL_BLEND);
    }
}
