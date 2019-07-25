




































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoApp;

import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.util.Log;
import android.view.Surface;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

public class SurfaceTextureLayer extends Layer implements SurfaceTexture.OnFrameAvailableListener {
    private static final String LOGTAG = "SurfaceTextureLayer";
    private static final int LOCAL_GL_TEXTURE_EXTERNAL_OES = 0x00008d65; 

    private final SurfaceTexture mSurfaceTexture;
    private final Surface mSurface;
    private int mTextureId;
    private boolean mHaveFrame;
    private float[] mTextureTransform = new float[16];

    private boolean mInverted;
    private boolean mNewInverted;
    private boolean mBlend;
    private boolean mNewBlend;

    private static int mProgram;
    private static int mPositionHandle;
    private static int mTextureHandle;
    private static int mSampleHandle;
    private static int mProjectionMatrixHandle;
    private static int mTextureMatrixHandle;

    private static final float[] PROJECTION_MATRIX = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    };

    private static final String VERTEX_SHADER =
        "uniform mat4 projectionMatrix;\n" +
        "uniform mat4 textureMatrix;\n" +
        "attribute vec4 vPosition;\n" +
        "attribute vec4 aTexCoord;\n" +
        "varying vec2 vTexCoord;\n" +
        "void main() {\n" +
        "  gl_Position = projectionMatrix * vPosition;\n" +
        "  vTexCoord = (textureMatrix * vec4(aTexCoord.x, aTexCoord.y, 0.0, 1.0)).xy;\n" +
        "}\n";

    private static String FRAGMENT_SHADER_OES =
        "#extension GL_OES_EGL_image_external : require\n" +
        "precision mediump float;\n" +
        "varying vec2 vTexCoord; \n" +
        "uniform samplerExternalOES sTexture; \n" +
        "void main() {\n" +
        "  gl_FragColor = texture2D(sTexture, vTexCoord); \n" +
        "}\n";
  

    private static final float TEXTURE_MAP[] = {
                0.0f, 1.0f, 
                0.0f, 0.0f, 
                1.0f, 1.0f, 
                1.0f, 0.0f, 
    };

    private static final float TEXTURE_MAP_INVERTED[] = {
                0.0f, 0.0f, 
                0.0f, 1.0f, 
                1.0f, 0.0f, 
                1.0f, 1.0f, 
    };

    private SurfaceTextureLayer(int textureId) {
        mTextureId = textureId;
        mHaveFrame = true;
        mInverted = false;

        mSurfaceTexture = new SurfaceTexture(mTextureId);
        mSurfaceTexture.setOnFrameAvailableListener(this);

        Surface tmp = null;
        try {
            tmp = Surface.class.getConstructor(SurfaceTexture.class).newInstance(mSurfaceTexture); }
        catch (Exception ie) {
            Log.e(LOGTAG, "error constructing the surface", ie);
        }

        mSurface = tmp;
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

    public void update(Rect position, float resolution, boolean inverted, boolean blend) {
        beginTransaction(); 

        setPosition(position);
        setResolution(resolution);

        mNewInverted = inverted;
        mNewBlend = blend;

        endTransaction();
    }

    @Override
    protected void finalize() throws Throwable {
        if (mSurfaceTexture != null) {
            try {
                SurfaceTexture.class.getDeclaredMethod("release").invoke(mSurfaceTexture);
            } catch (NoSuchMethodException nsme) {
                Log.e(LOGTAG, "error finding release method on mSurfaceTexture", nsme);
            } catch (IllegalAccessException iae) {
                Log.e(LOGTAG, "error invoking release method on mSurfaceTexture", iae);
            } catch (Exception e) {
                Log.e(LOGTAG, "some other exception while invoking release method on mSurfaceTexture", e);
            }
        }
        if (mTextureId > 0)
            TextureReaper.get().add(mTextureId);
    }

    @Override
    protected boolean performUpdates(RenderContext context) {
        super.performUpdates(context);

        mInverted = mNewInverted;
        mBlend = mNewBlend;

        return true;
    }

    private static boolean ensureProgram() {
        if (mProgram != 0)
            return true;

        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, VERTEX_SHADER);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, FRAGMENT_SHADER_OES);

        mProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(mProgram, vertexShader);
        GLES20.glAttachShader(mProgram, fragmentShader);
        GLES20.glLinkProgram(mProgram);

        mPositionHandle = GLES20.glGetAttribLocation(mProgram, "vPosition");
        mTextureHandle = GLES20.glGetAttribLocation(mProgram, "aTexCoord");
        mSampleHandle = GLES20.glGetUniformLocation(mProgram, "sTexture");
        mProjectionMatrixHandle = GLES20.glGetUniformLocation(mProgram, "projectionMatrix");
        mTextureMatrixHandle = GLES20.glGetUniformLocation(mProgram, "textureMatrix");

        return mProgram != 0;
    }

    private static int loadShader(int type, String shaderCode) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);
        return shader;
    }

    private static void activateProgram() {
        GLES20.glUseProgram(mProgram);
    }

    public static void deactivateProgram() {
        GLES20.glDisableVertexAttribArray(mTextureHandle);
        GLES20.glDisableVertexAttribArray(mPositionHandle);
        GLES20.glUseProgram(0);
    }

    @Override
    public void draw(RenderContext context) {
        if (!ensureProgram() || !mHaveFrame)
            return;

        RectF rect = getBounds(context);
        RectF viewport = context.viewport;
        rect.offset(-viewport.left, -viewport.top);

        float viewWidth = viewport.width();
        float viewHeight = viewport.height();

        float top = viewHeight - rect.top;
        float bot = viewHeight - rect.bottom;

        float[] textureCoords = mInverted ? TEXTURE_MAP_INVERTED : TEXTURE_MAP;

        
        float[] coords = {
            
            rect.left/viewWidth, bot/viewHeight, 0,
            textureCoords[0], textureCoords[1],

            rect.left/viewWidth, (bot+rect.height())/viewHeight, 0,
            textureCoords[2], textureCoords[3],

            (rect.left+rect.width())/viewWidth, bot/viewHeight, 0,
            textureCoords[4], textureCoords[5],

            (rect.left+rect.width())/viewWidth, (bot+rect.height())/viewHeight, 0,
            textureCoords[6], textureCoords[7]
        };

        FloatBuffer coordBuffer = context.coordBuffer;
        coordBuffer.position(0);
        coordBuffer.put(coords);

        activateProgram();

        
        GLES20.glUniformMatrix4fv(mProjectionMatrixHandle, 1, false, PROJECTION_MATRIX, 0);

        
        GLES20.glEnableVertexAttribArray(mPositionHandle);
        GLES20.glEnableVertexAttribArray(mTextureHandle);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glUniform1i(mSampleHandle, 0);
        GLES20.glBindTexture(LOCAL_GL_TEXTURE_EXTERNAL_OES, mTextureId);
      
        mSurfaceTexture.updateTexImage();
        mSurfaceTexture.getTransformMatrix(mTextureTransform);

        GLES20.glUniformMatrix4fv(mTextureMatrixHandle, 1, false, mTextureTransform, 0);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(mPositionHandle, 3, GLES20.GL_FLOAT, false, 20,
                coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(mTextureHandle, 3, GLES20.GL_FLOAT, false, 20,
                coordBuffer);

        if (mBlend) {
            GLES20.glEnable(GLES20.GL_BLEND);
            GLES20.glBlendFunc(GLES20.GL_ONE, GLES20.GL_ONE_MINUS_SRC_ALPHA);
        }
        
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        if (mBlend)
            GLES20.glDisable(GLES20.GL_BLEND);
        
        deactivateProgram();
    }

    public SurfaceTexture getSurfaceTexture() {
        return mSurfaceTexture;
    }

    public Surface getSurface() {
        return mSurface;
    }
}

