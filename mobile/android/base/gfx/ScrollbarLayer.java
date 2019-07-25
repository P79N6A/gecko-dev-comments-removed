




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.mozglue.DirectBufferAllocator;
import org.mozilla.gecko.util.FloatUtils;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES20;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;




public class ScrollbarLayer extends TileLayer {
    public static final long FADE_DELAY = 500; 
    private static final float FADE_AMOUNT = 0.03f; 

    private static final int PADDING = 1;   
    private static final int BAR_SIZE = 6;
    private static final int CAP_RADIUS = (BAR_SIZE / 2);

    private final boolean mVertical;
    private final Bitmap mBitmap;
    private final Canvas mCanvas;
    private float mOpacity;

    
    
    private final RectF mBarRectF;
    private final Rect mBarRect;
    private final float[] mBodyCoords;
    private final float[] mCap;

    private LayerRenderer mRenderer;
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

    
    private static final float TEX_HEIGHT = 8.0f;
    private static final float TEX_WIDTH = 8.0f;

    
    
    private static final float[] BODY_TEX_COORDS = {
        
        CAP_RADIUS/TEX_WIDTH, CAP_RADIUS/TEX_HEIGHT,
        CAP_RADIUS/TEX_WIDTH, (CAP_RADIUS+1)/TEX_HEIGHT,
        (CAP_RADIUS+1)/TEX_WIDTH, CAP_RADIUS/TEX_HEIGHT,
        (CAP_RADIUS+1)/TEX_WIDTH, (CAP_RADIUS+1)/TEX_HEIGHT
    };

    
    private static final float[] TOP_CAP_TEX_COORDS = {
        
        0                 , 1.0f - CAP_RADIUS/TEX_HEIGHT,
        0                 , 1.0f,
        BAR_SIZE/TEX_WIDTH, 1.0f - CAP_RADIUS/TEX_HEIGHT,
        BAR_SIZE/TEX_WIDTH, 1.0f
    };

    
    private static final float[] BOT_CAP_TEX_COORDS = {
        
        0                 , 1.0f - BAR_SIZE/TEX_HEIGHT,
        0                 , 1.0f - CAP_RADIUS/TEX_HEIGHT,
        BAR_SIZE/TEX_WIDTH, 1.0f - BAR_SIZE/TEX_HEIGHT,
        BAR_SIZE/TEX_WIDTH, 1.0f - CAP_RADIUS/TEX_HEIGHT
    };

    
    private static final float[] LEFT_CAP_TEX_COORDS = {
        
        0                   , 1.0f - BAR_SIZE/TEX_HEIGHT,
        0                   , 1.0f,
        CAP_RADIUS/TEX_WIDTH, 1.0f - BAR_SIZE/TEX_HEIGHT,
        CAP_RADIUS/TEX_WIDTH, 1.0f
    };

    
    private static final float[] RIGHT_CAP_TEX_COORDS = {
        
        CAP_RADIUS/TEX_WIDTH, 1.0f - BAR_SIZE/TEX_HEIGHT,
        CAP_RADIUS/TEX_WIDTH, 1.0f,
        BAR_SIZE/TEX_WIDTH  , 1.0f - BAR_SIZE/TEX_HEIGHT,
        BAR_SIZE/TEX_WIDTH  , 1.0f
    };

    private ScrollbarLayer(LayerRenderer renderer, CairoImage image, boolean vertical, ByteBuffer buffer) {
        super(image, TileLayer.PaintMode.NORMAL);
        mVertical = vertical;
        mRenderer = renderer;

        IntSize size = image.getSize();
        mBitmap = Bitmap.createBitmap(size.width, size.height, Bitmap.Config.ARGB_8888);
        mCanvas = new Canvas(mBitmap);

        
        Paint foregroundPaint = new Paint();
        foregroundPaint.setAntiAlias(true);
        foregroundPaint.setStyle(Paint.Style.FILL);
        foregroundPaint.setColor(Color.argb(127, 0, 0, 0));

        mCanvas.drawColor(Color.argb(0, 0, 0, 0), PorterDuff.Mode.CLEAR);
        mCanvas.drawCircle(CAP_RADIUS, CAP_RADIUS, CAP_RADIUS, foregroundPaint);

        mBitmap.copyPixelsToBuffer(buffer.asIntBuffer());

        mBarRectF = new RectF();
        mBarRect = new Rect();
        mBodyCoords = new float[20];
        mCap = new float[20];
    }

    public static ScrollbarLayer create(LayerRenderer renderer, boolean vertical) {
        
        
        int imageSize = IntSize.nextPowerOfTwo(BAR_SIZE);
        ByteBuffer buffer = DirectBufferAllocator.allocate(imageSize * imageSize * 4);
        CairoImage image = new BufferedCairoImage(buffer, imageSize, imageSize,
                                                  CairoImage.FORMAT_ARGB32);
        return new ScrollbarLayer(renderer, image, vertical, buffer);
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

        float top = viewHeight - mBarRect.top;
        float bot = viewHeight - mBarRect.bottom;

        
        
        mBodyCoords[0] = mBarRect.left/viewWidth;
        mBodyCoords[1] = bot/viewHeight;
        mBodyCoords[2] = 0;
        mBodyCoords[3] = BODY_TEX_COORDS[0];
        mBodyCoords[4] = BODY_TEX_COORDS[1];

        mBodyCoords[5] = mBarRect.left/viewWidth;
        mBodyCoords[6] = (bot+mBarRect.height())/viewHeight;
        mBodyCoords[7] = 0;
        mBodyCoords[8] = BODY_TEX_COORDS[2];
        mBodyCoords[9] = BODY_TEX_COORDS[3];

        mBodyCoords[10] = (mBarRect.left+mBarRect.width())/viewWidth;
        mBodyCoords[11] = bot/viewHeight;
        mBodyCoords[12] = 0;
        mBodyCoords[13] = BODY_TEX_COORDS[4];
        mBodyCoords[14] = BODY_TEX_COORDS[5];

        mBodyCoords[15] = (mBarRect.left+mBarRect.width())/viewWidth;
        mBodyCoords[16] = (bot+mBarRect.height())/viewHeight;
        mBodyCoords[17] = 0;
        mBodyCoords[18] = BODY_TEX_COORDS[6];
        mBodyCoords[19] = BODY_TEX_COORDS[7];

        
        FloatBuffer coordBuffer = context.coordBuffer;
        int positionHandle = mPositionHandle;
        int textureHandle = mTextureHandle;

        
        
        coordBuffer.position(0);
        coordBuffer.put(mBodyCoords);

        
        GLES20.glBindBuffer(GLES20.GL_ARRAY_BUFFER, 0);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                coordBuffer);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        
        coordBuffer.position(0);

        if (mVertical) {
            
            
            mCap[0] = mBarRect.left/viewWidth;
            mCap[1] = top/viewHeight;
            mCap[2] = 0;
            mCap[3] = TOP_CAP_TEX_COORDS[0];
            mCap[4] = TOP_CAP_TEX_COORDS[1];

            mCap[5] = mBarRect.left/viewWidth;
            mCap[6] = (top+CAP_RADIUS)/viewHeight;
            mCap[7] = 0;
            mCap[8] = TOP_CAP_TEX_COORDS[2];
            mCap[9] = TOP_CAP_TEX_COORDS[3];

            mCap[10] = (mBarRect.left+BAR_SIZE)/viewWidth;
            mCap[11] = top/viewHeight;
            mCap[12] = 0;
            mCap[13] = TOP_CAP_TEX_COORDS[4];
            mCap[14] = TOP_CAP_TEX_COORDS[5];

            mCap[15] = (mBarRect.left+BAR_SIZE)/viewWidth;
            mCap[16] = (top+CAP_RADIUS)/viewHeight;
            mCap[17] = 0;
            mCap[18] = TOP_CAP_TEX_COORDS[6];
            mCap[19] = TOP_CAP_TEX_COORDS[7];

            coordBuffer.put(mCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            
            
            coordBuffer.position(0);

            
            
            mCap[0] = mBarRect.left/viewWidth;
            mCap[1] = (bot-CAP_RADIUS)/viewHeight;
            mCap[2] = 0;
            mCap[3] = BOT_CAP_TEX_COORDS[0];
            mCap[4] = BOT_CAP_TEX_COORDS[1];

            mCap[5] = mBarRect.left/viewWidth;
            mCap[6] = (bot)/viewHeight;
            mCap[7] = 0;
            mCap[8] = BOT_CAP_TEX_COORDS[2];
            mCap[9] = BOT_CAP_TEX_COORDS[3];

            mCap[10] = (mBarRect.left+BAR_SIZE)/viewWidth;
            mCap[11] = (bot-CAP_RADIUS)/viewHeight;
            mCap[12] = 0;
            mCap[13] = BOT_CAP_TEX_COORDS[4];
            mCap[14] = BOT_CAP_TEX_COORDS[5];

            mCap[15] = (mBarRect.left+BAR_SIZE)/viewWidth;
            mCap[16] = (bot)/viewHeight;
            mCap[17] = 0;
            mCap[18] = BOT_CAP_TEX_COORDS[6];
            mCap[19] = BOT_CAP_TEX_COORDS[7];

            coordBuffer.put(mCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            
            
            coordBuffer.position(0);
        } else {
            
            
            mCap[0] = (mBarRect.left-CAP_RADIUS)/viewWidth;
            mCap[1] = bot/viewHeight;
            mCap[2] = 0;
            mCap[3] = LEFT_CAP_TEX_COORDS[0];
            mCap[4] = LEFT_CAP_TEX_COORDS[1];

            mCap[5] = (mBarRect.left-CAP_RADIUS)/viewWidth;
            mCap[6] = (bot+BAR_SIZE)/viewHeight;
            mCap[7] = 0;
            mCap[8] = LEFT_CAP_TEX_COORDS[2];
            mCap[9] = LEFT_CAP_TEX_COORDS[3];

            mCap[10] = (mBarRect.left)/viewWidth;
            mCap[11] = bot/viewHeight;
            mCap[12] = 0;
            mCap[13] = LEFT_CAP_TEX_COORDS[4];
            mCap[14] = LEFT_CAP_TEX_COORDS[5];

            mCap[15] = (mBarRect.left)/viewWidth;
            mCap[16] = (bot+BAR_SIZE)/viewHeight;
            mCap[17] = 0;
            mCap[18] = LEFT_CAP_TEX_COORDS[6];
            mCap[19] = LEFT_CAP_TEX_COORDS[7];

            coordBuffer.put(mCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            
            
            coordBuffer.position(0);

            
            
            mCap[0] = mBarRect.right/viewWidth;
            mCap[1] = (bot)/viewHeight;
            mCap[2] = 0;
            mCap[3] = RIGHT_CAP_TEX_COORDS[0];
            mCap[4] = RIGHT_CAP_TEX_COORDS[1];

            mCap[5] = mBarRect.right/viewWidth;
            mCap[6] = (bot+BAR_SIZE)/viewHeight;
            mCap[7] = 0;
            mCap[8] = RIGHT_CAP_TEX_COORDS[2];
            mCap[9] = RIGHT_CAP_TEX_COORDS[3];

            mCap[10] = (mBarRect.right+CAP_RADIUS)/viewWidth;
            mCap[11] = (bot)/viewHeight;
            mCap[12] = 0;
            mCap[13] = RIGHT_CAP_TEX_COORDS[4];
            mCap[14] = RIGHT_CAP_TEX_COORDS[5];

            mCap[15] = (mBarRect.right+CAP_RADIUS)/viewWidth;
            mCap[16] = (bot+BAR_SIZE)/viewHeight;
            mCap[17] = 0;
            mCap[18] = RIGHT_CAP_TEX_COORDS[6];
            mCap[19] = RIGHT_CAP_TEX_COORDS[7];

            coordBuffer.put(mCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
        }

        
        deactivateProgram();
        mRenderer.activateDefaultProgram();
    }

    private void getVerticalRect(RenderContext context, RectF dest) {
        RectF viewport = context.viewport;
        RectF pageRect = context.pageRect;
        float barStart = ((viewport.top - pageRect.top) * (viewport.height() / pageRect.height())) + CAP_RADIUS;
        float barEnd = ((viewport.bottom - pageRect.top) * (viewport.height() / pageRect.height())) - CAP_RADIUS;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        float right = viewport.width() - PADDING;
        dest.set(right - BAR_SIZE, barStart, right, barEnd);
    }

    private void getHorizontalRect(RenderContext context, RectF dest) {
        RectF viewport = context.viewport;
        RectF pageRect = context.pageRect;
        float barStart = ((viewport.left - pageRect.left) * (viewport.width() / pageRect.width())) + CAP_RADIUS;
        float barEnd = ((viewport.right - pageRect.left) * (viewport.width() / pageRect.width())) - CAP_RADIUS;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        float bottom = viewport.height() - PADDING;
        dest.set(barStart, bottom - BAR_SIZE, barEnd, bottom);
    }
}
