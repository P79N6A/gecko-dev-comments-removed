




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;

import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.opengl.Matrix;
import android.util.Log;
import android.view.Surface;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.opengles.GL11Ext;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import android.hardware.Camera;

public class SurfaceTextureLayer extends Layer implements SurfaceTexture.OnFrameAvailableListener {
    private static final String LOGTAG = "SurfaceTextureLayer";
    private static final int LOCAL_GL_TEXTURE_EXTERNAL_OES = 0x00008d65; 

    private final SurfaceTexture mSurfaceTexture;
    private final Surface mSurface;
    private int mTextureId;
    private boolean mHaveFrame;

    private IntSize mSize;
    private IntSize mNewSize;

    private boolean mInverted;
    private boolean mNewInverted;
    private boolean mBlend;
    private boolean mNewBlend;

	private FloatBuffer textureBuffer;
    private FloatBuffer textureBufferInverted;

    public SurfaceTextureLayer(int textureId) {
        mTextureId = textureId;
        mHaveFrame = true;
        mInverted = false;

        mSurfaceTexture = new SurfaceTexture(mTextureId);
        mSurfaceTexture.setOnFrameAvailableListener(this);

        mSurface = new Surface(mSurfaceTexture);

        float textureMap[] = {
                0.0f, 1.0f,	
                0.0f, 0.0f,	
                1.0f, 1.0f,	
                1.0f, 0.0f,	
        };

        textureBuffer = createBuffer(textureMap);

        float textureMapInverted[] = {
                0.0f, 0.0f,	
                0.0f, 1.0f,	
                1.0f, 0.0f,	
                1.0f, 1.0f,	
        };

        textureBufferInverted = createBuffer(textureMapInverted);
    }

    public static SurfaceTextureLayer create() {
        int textureId = TextureGenerator.get().take();
        if (textureId == 0)
            return null;

        return new SurfaceTextureLayer(textureId);
    }

    
    public void onFrameAvailable(SurfaceTexture texture) {
        
        mHaveFrame = true;
        GeckoApp.mAppContext.requestRender();
    }

    private FloatBuffer createBuffer(float[] input) {
        
		ByteBuffer byteBuffer = ByteBuffer.allocateDirect(input.length * 4);
		byteBuffer.order(ByteOrder.nativeOrder());

        FloatBuffer floatBuffer = byteBuffer.asFloatBuffer();
        floatBuffer.put(input);
		floatBuffer.position(0);

        return floatBuffer;
    }

    public void update(Point origin, IntSize size, float resolution, boolean inverted, boolean blend) {
        beginTransaction(null);

        setOrigin(origin);
        setResolution(resolution);

        mNewSize = size;
        mNewInverted = inverted;
        mNewBlend = blend;

        endTransaction();
    }

    @Override
    public IntSize getSize() { return mSize; }

    @Override
    protected void finalize() throws Throwable {
        if (mSurfaceTexture != null)
            mSurfaceTexture.release();

        if (mTextureId > 0)
            TextureReaper.get().add(mTextureId);
    }

    @Override
    protected boolean performUpdates(GL10 gl, RenderContext context) {
        super.performUpdates(gl, context);

        if (mNewSize != null) {
            mSize = mNewSize;
            mNewSize = null;
        }

        mInverted = mNewInverted;
        mBlend = mNewBlend;

        gl.glEnable(LOCAL_GL_TEXTURE_EXTERNAL_OES);
        gl.glBindTexture(LOCAL_GL_TEXTURE_EXTERNAL_OES, mTextureId);
        mSurfaceTexture.updateTexImage();
        gl.glDisable(LOCAL_GL_TEXTURE_EXTERNAL_OES);

        
        
        return false;
    }

    private float mapToGLCoords(float input, float viewport, boolean flip) {
        if (flip) input = viewport - input;
        return ((input / viewport) * 2.0f) - 1.0f;
    }

    @Override
    public void draw(RenderContext context) {

        
        GLES11.glEnable(LOCAL_GL_TEXTURE_EXTERNAL_OES);
        GLES11.glBindTexture(LOCAL_GL_TEXTURE_EXTERNAL_OES, mTextureId);

        
        GLES11.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        GLES11.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

        
        float[] matrix = new float[16];
        mSurfaceTexture.getTransformMatrix(matrix);
        GLES11.glMatrixMode(GLES11.GL_TEXTURE);
        GLES11.glLoadMatrixf(matrix, 0);

        
        IntSize size = getSize();
        RectF bounds = getBounds(context, new FloatSize(size));
        RectF viewport = context.viewport;
        bounds.offset(-viewport.left, -viewport.top);

        float vertices[] = new float[8];

        
        vertices[0] = mapToGLCoords(bounds.left, viewport.width(), false);
        vertices[1] = mapToGLCoords(bounds.bottom, viewport.height(), true);

        
        vertices[2] = mapToGLCoords(bounds.left, viewport.width(), false);
        vertices[3] = mapToGLCoords(bounds.top, viewport.height(), true);

        
        vertices[4] = mapToGLCoords(bounds.right, viewport.width(), false);
        vertices[5] = mapToGLCoords(bounds.bottom, viewport.height(), true);

        
        vertices[6] = mapToGLCoords(bounds.right, viewport.width(), false);
        vertices[7] = mapToGLCoords(bounds.top, viewport.height(), true);

        
        GLES11.glVertexPointer(2, GL10.GL_FLOAT, 0, createBuffer(vertices));
        GLES11.glTexCoordPointer(2, GL10.GL_FLOAT, 0, mInverted ? textureBufferInverted : textureBuffer);

        if (mBlend) {
            GLES11.glEnable(GL10.GL_BLEND);
            GLES11.glBlendFunc(GL10.GL_ONE, GL10.GL_ONE_MINUS_SRC_ALPHA);
        }

        
        GLES11.glDrawArrays(GL10.GL_TRIANGLE_STRIP, 0, vertices.length / 2);

        
        GLES11.glDisableClientState(GL10.GL_VERTEX_ARRAY);
        GLES11.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
        GLES11.glDisable(LOCAL_GL_TEXTURE_EXTERNAL_OES);
        GLES11.glLoadIdentity();

        if (mBlend) {
            GLES11.glDisable(GL10.GL_BLEND);
        }
    }

    public SurfaceTexture getSurfaceTexture() {
        return mSurfaceTexture;
    }

    public Surface getSurface() {
        return mSurface;
    }
}

