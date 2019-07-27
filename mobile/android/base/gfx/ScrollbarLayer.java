




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.util.FloatUtils;

import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.util.Log;

import java.nio.FloatBuffer;
import java.nio.ByteBuffer;

public class ScrollbarLayer extends Layer {
    private static final String LOGTAG = "GeckoScrollbarLayer";

    public static final long FADE_DELAY = 500; 
    private static final float FADE_AMOUNT = 0.03f; 

    private final boolean mVertical;
    private float mOpacity;
    private final Rect mDirtyRect;
    private IntSize mSize;

    private int[] mTextureIDs;

    private final BufferedImage mImage;

    
    
    private final RectF mBarRectF;
    private final Rect mBarRect;
    private final float[] mCoords;
    private final RectF mCapRectF;

    private final LayerRenderer mRenderer;
    private int mProgram;
    private int mPositionHandle;
    private int mTextureHandle;
    private int mSampleHandle;
    private int mTMatrixHandle;
    private int mOpacityHandle;

    
    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n" +
        "varying vec2 vTexCoord;\n" +
        "uniform sampler2D sTexture;\n" +
        "uniform float uOpacity;\n" +
        "void main() {\n" +
        "    gl_FragColor = texture2D(sTexture, vTexCoord);\n" +
        "    gl_FragColor.a *= uOpacity;\n" +
        "}\n";

    
    private final int mTexWidth;
    private final int mTexHeight;
    
    private final int mBarWidth;
    private final int mCapLength;

    private final Rect mStartCapTexCoords;  
    private final Rect mBodyTexCoords;      
    private final Rect mEndCapTexCoords;    

    ScrollbarLayer(LayerRenderer renderer, Bitmap scrollbarImage, IntSize imageSize, boolean vertical) {
        super(new IntSize(scrollbarImage.getHeight(), scrollbarImage.getWidth()));
        mImage = new BufferedImage(scrollbarImage);
        mRenderer = renderer;
        mVertical = vertical;

        mBarRectF = new RectF();
        mBarRect = new Rect();
        mCoords = new float[20];
        mCapRectF = new RectF();
        mDirtyRect = new Rect();
        mSize = new IntSize(0, 0);

        mTexHeight = scrollbarImage.getHeight();
        mTexWidth = scrollbarImage.getWidth();

        if (mVertical) {
            mBarWidth = imageSize.width;
            mCapLength = imageSize.height / 2;
            mStartCapTexCoords = new Rect(0, mTexHeight - mCapLength, imageSize.width, mTexHeight);
            mBodyTexCoords = new Rect(0, mTexHeight - (mCapLength + 1), imageSize.width, mTexHeight - mCapLength);
            mEndCapTexCoords = new Rect(0, mTexHeight - imageSize.height, imageSize.width, mTexHeight - (mCapLength + 1));
        } else {
            mBarWidth = imageSize.height;
            mCapLength = imageSize.width / 2;
            mStartCapTexCoords = new Rect(0, mTexHeight - imageSize.height, mCapLength, mTexHeight);
            mBodyTexCoords = new Rect(mCapLength, mTexHeight - imageSize.height, mCapLength + 1, mTexHeight);
            mEndCapTexCoords = new Rect(mCapLength + 1, mTexHeight - imageSize.height, imageSize.width, mTexHeight);
        }
    }

    private void createProgram() {
        int vertexShader = LayerRenderer.loadShader(GLES20.GL_VERTEX_SHADER,
                                                    LayerRenderer.DEFAULT_VERTEX_SHADER);
        int fragmentShader = LayerRenderer.loadShader(GLES20.GL_FRAGMENT_SHADER,
                                                      FRAGMENT_SHADER);

        mProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(mProgram, vertexShader);   
        GLES20.glAttachShader(mProgram, fragmentShader); 
        GLES20.glLinkProgram(mProgram);                  

        
        mPositionHandle = GLES20.glGetAttribLocation(mProgram, "vPosition");
        mTextureHandle = GLES20.glGetAttribLocation(mProgram, "aTexCoord");
        mSampleHandle = GLES20.glGetUniformLocation(mProgram, "sTexture");
        mTMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uTMatrix");
        mOpacityHandle = GLES20.glGetUniformLocation(mProgram, "uOpacity");
    }

    private void activateProgram() {
        
        GLES20.glUseProgram(mProgram);

        
        GLES20.glUniformMatrix4fv(mTMatrixHandle, 1, false,
                                  LayerRenderer.DEFAULT_TEXTURE_MATRIX, 0);

        
        GLES20.glEnableVertexAttribArray(mPositionHandle);
        GLES20.glEnableVertexAttribArray(mTextureHandle);

        GLES20.glUniform1i(mSampleHandle, 0);
        GLES20.glUniform1f(mOpacityHandle, mOpacity);
    }

    private void deactivateProgram() {
        GLES20.glDisableVertexAttribArray(mTextureHandle);
        GLES20.glDisableVertexAttribArray(mPositionHandle);
        GLES20.glUseProgram(0);
    }

    




    public boolean fade() {
        if (FloatUtils.fuzzyEquals(mOpacity, 0.0f)) {
            return false;
        }
        beginTransaction(); 
        mOpacity = Math.max(mOpacity - FADE_AMOUNT, 0.0f);
        endTransaction();
        return true;
    }

    




    public boolean unfade() {
        if (FloatUtils.fuzzyEquals(mOpacity, 1.0f)) {
            return false;
        }
        beginTransaction(); 
        mOpacity = 1.0f;
        endTransaction();
        return true;
    }

    @Override
    public void draw(RenderContext context) {
        if (!initialized())
            return;

        
        if (mProgram == 0) {
            createProgram();
        }

        
        mRenderer.deactivateDefaultProgram();
        activateProgram();

        GLES20.glEnable(GLES20.GL_BLEND);
        GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);

        if (mVertical) {
            getVerticalRect(context, mBarRectF);
        } else {
            getHorizontalRect(context, mBarRectF);
        }
        RectUtils.round(mBarRectF, mBarRect);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, getTextureID());

        float viewWidth = context.viewport.width();
        float viewHeight = context.viewport.height();

        mBarRectF.set(mBarRect.left, viewHeight - mBarRect.top, mBarRect.right, viewHeight - mBarRect.bottom);
        mBarRectF.offset(context.offset.x, -context.offset.y);

        
        fillRectCoordBuffer(mCoords, mBarRectF, viewWidth, viewHeight, mBodyTexCoords, mTexWidth, mTexHeight);

        
        FloatBuffer coordBuffer = context.coordBuffer;
        int positionHandle = mPositionHandle;
        int textureHandle = mTextureHandle;

        
        
        coordBuffer.position(0);
        coordBuffer.put(mCoords);

        
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20, coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20, coordBuffer);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        
        coordBuffer.position(0);
        if (mVertical) {
            
            mCapRectF.set(mBarRectF.left, mBarRectF.top + mCapLength, mBarRectF.right, mBarRectF.top);
        } else {
            
            mCapRectF.set(mBarRectF.left - mCapLength, mBarRectF.bottom + mBarWidth, mBarRectF.left, mBarRectF.bottom);
        }

        fillRectCoordBuffer(mCoords, mCapRectF, viewWidth, viewHeight, mStartCapTexCoords, mTexWidth, mTexHeight);
        coordBuffer.put(mCoords);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20, coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20, coordBuffer);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        
        coordBuffer.position(0);
        if (mVertical) {
            
            mCapRectF.set(mBarRectF.left, mBarRectF.bottom, mBarRectF.right, mBarRectF.bottom - mCapLength);
        } else {
            
            mCapRectF.set(mBarRectF.right, mBarRectF.bottom + mBarWidth, mBarRectF.right + mCapLength, mBarRectF.bottom);
        }
        fillRectCoordBuffer(mCoords, mCapRectF, viewWidth, viewHeight, mEndCapTexCoords, mTexWidth, mTexHeight);
        coordBuffer.put(mCoords);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20, coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20, coordBuffer);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        
        coordBuffer.position(0);

        
        deactivateProgram();
        mRenderer.activateDefaultProgram();
    }

    private void getVerticalRect(RenderContext context, RectF dest) {
        RectF viewport = context.viewport;
        RectF pageRect = context.pageRect;
        float viewportHeight = viewport.height() - context.offset.y;
        float barStart = ((viewport.top - context.offset.y - pageRect.top) * (viewportHeight / pageRect.height())) + mCapLength;
        float barEnd = ((viewport.bottom - context.offset.y - pageRect.top) * (viewportHeight / pageRect.height())) - mCapLength;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        dest.set(viewport.width() - mBarWidth, barStart, viewport.width(), barEnd);
    }

    private void getHorizontalRect(RenderContext context, RectF dest) {
        RectF viewport = context.viewport;
        RectF pageRect = context.pageRect;
        float viewportWidth = viewport.width() - context.offset.x;
        float barStart = ((viewport.left - context.offset.x - pageRect.left) * (viewport.width() / pageRect.width())) + mCapLength;
        float barEnd = ((viewport.right - context.offset.x - pageRect.left) * (viewport.width() / pageRect.width())) - mCapLength;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        dest.set(barStart, viewport.height() - mBarWidth, barEnd, viewport.height());
    }

    private void validateTexture() {
        






        IntSize textureSize = mImage.getSize().nextPowerOfTwo();

        if (!textureSize.equals(mSize)) {
            mSize = textureSize;

            
            if (mTextureIDs != null) {
                TextureReaper.get().add(mTextureIDs);
                mTextureIDs = null;

                
                
                TextureReaper.get().reap();
            }
        }
    }

    @Override
    protected void performUpdates(RenderContext context) {
        super.performUpdates(context);

        
        validateTexture();

        
        if (!mImage.getSize().isPositive())
            return;

        
        if (mTextureIDs == null) {
            uploadFullTexture();
        } else {
            uploadDirtyRect(mDirtyRect);
        }

        mDirtyRect.setEmpty();
    }

    private void uploadFullTexture() {
        IntSize bufferSize = mImage.getSize();
        uploadDirtyRect(new Rect(0, 0, bufferSize.width, bufferSize.height));
    }

    private void uploadDirtyRect(Rect dirtyRect) {
        
        if (dirtyRect.isEmpty())
            return;

        
        ByteBuffer imageBuffer = mImage.getBuffer();
        if (imageBuffer == null)
            return;

        if (mTextureIDs == null) {
            mTextureIDs = new int[1];
            GLES20.glGenTextures(mTextureIDs.length, mTextureIDs, 0);
        }

        int imageFormat = mImage.getFormat();
        BufferedImageGLInfo glInfo = new BufferedImageGLInfo(imageFormat);

        bindAndSetGLParameters();

        
        
        IntSize bufferSize = mImage.getSize();
        if (mSize.equals(bufferSize)) {
            GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, glInfo.internalFormat, mSize.width,
                                mSize.height, 0, glInfo.format, glInfo.type, imageBuffer);
        } else {
            
            
            throw new RuntimeException("Buffer/image size mismatch in ScrollbarLayer!");
        }
    }

    private void bindAndSetGLParameters() {
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureIDs[0]);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MIN_FILTER,
                               GLES20.GL_LINEAR);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_MAG_FILTER,
                               GLES20.GL_LINEAR);

        int repeatMode = GLES20.GL_CLAMP_TO_EDGE;
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_S, repeatMode);
        GLES20.glTexParameterf(GLES20.GL_TEXTURE_2D, GLES20.GL_TEXTURE_WRAP_T, repeatMode);
    }

    @Override
    public void destroy() {
        try {
            if (mImage != null) {
                mImage.destroy();
            }
        } catch (Exception ex) {
            Log.e(LOGTAG, "error clearing buffers: ", ex);
        }
    }

    protected int getTextureID() { return mTextureIDs[0]; }
    protected boolean initialized() { return mImage != null && mTextureIDs != null; }

    @Override
    protected void finalize() throws Throwable {
        try {
            if (mTextureIDs != null)
                TextureReaper.get().add(mTextureIDs);
        } finally {
            super.finalize();
        }
    }
}
