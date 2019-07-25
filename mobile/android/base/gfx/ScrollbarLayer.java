





































package org.mozilla.gecko.gfx;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.util.Log;
import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import org.mozilla.gecko.FloatUtils;
import org.mozilla.gecko.GeckoAppShell;




public class ScrollbarLayer extends TileLayer {
    public static final long FADE_DELAY = 500; 
    private static final float FADE_AMOUNT = 0.03f; 

    private static final int PADDING = 1;   
    private static final int BAR_SIZE = 6;
    private static final int CAP_RADIUS = (BAR_SIZE / 2);

    private final boolean mVertical;
    private final ByteBuffer mBuffer;
    private final Bitmap mBitmap;
    private final Canvas mCanvas;
    private float mOpacity;
    private boolean mFinalized = false;

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
        "    gl_FragColor = texture2D(sTexture, vec2(vTexCoord.x, 1.0 - vTexCoord.y));\n" +
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
        super(false, image);
        mVertical = vertical;
        mBuffer = buffer;
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

        mBitmap.copyPixelsToBuffer(mBuffer.asIntBuffer());
    }

    protected void finalize() throws Throwable {
        try {
            if (!mFinalized && mBuffer != null)
                GeckoAppShell.freeDirectBuffer(mBuffer);
            mFinalized = true;
        } finally {
            super.finalize();
        }
    }

    public static ScrollbarLayer create(LayerRenderer renderer, boolean vertical) {
        
        
        int imageSize = IntSize.nextPowerOfTwo(BAR_SIZE);
        ByteBuffer buffer = GeckoAppShell.allocateDirectBuffer(imageSize * imageSize * 4);
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
        try {
            mOpacity = Math.max(mOpacity - FADE_AMOUNT, 0.0f);
            invalidate();
        } finally {
            endTransaction();
        }
        return true;
    }

    




    public boolean unfade() {
        if (FloatUtils.fuzzyEquals(mOpacity, 1.0f)) {
            return false;
        }
        beginTransaction(); 
        try {
            mOpacity = 1.0f;
            invalidate();
        } finally {
            endTransaction();
        }
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

        Rect rect = RectUtils.round(mVertical
                ? getVerticalRect(context)
                : getHorizontalRect(context));
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, getTextureID());

        float viewWidth = context.viewport.width();
        float viewHeight = context.viewport.height();

        float top = viewHeight - rect.top;
        float bot = viewHeight - rect.bottom;

        
        float[] bodyCoords = {
            
            rect.left/viewWidth, bot/viewHeight, 0,
            BODY_TEX_COORDS[0], BODY_TEX_COORDS[1],

            rect.left/viewWidth, (bot+rect.height())/viewHeight, 0,
            BODY_TEX_COORDS[2], BODY_TEX_COORDS[3],

            (rect.left+rect.width())/viewWidth, bot/viewHeight, 0,
            BODY_TEX_COORDS[4], BODY_TEX_COORDS[5],

            (rect.left+rect.width())/viewWidth, (bot+rect.height())/viewHeight, 0,
            BODY_TEX_COORDS[6], BODY_TEX_COORDS[7]
        };

        
        FloatBuffer coordBuffer = context.coordBuffer;
        int positionHandle = mPositionHandle;
        int textureHandle = mTextureHandle;

        
        
        coordBuffer.position(0);
        coordBuffer.put(bodyCoords);

        
        coordBuffer.position(0);
        GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                coordBuffer);

        
        coordBuffer.position(3);
        GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                coordBuffer);

        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        
        coordBuffer.position(0);

        if (mVertical) {
            
            float[] topCap = {
                
                rect.left/viewWidth, top/viewHeight, 0,
                TOP_CAP_TEX_COORDS[0], TOP_CAP_TEX_COORDS[1],

                rect.left/viewWidth, (top+CAP_RADIUS)/viewHeight, 0,
                TOP_CAP_TEX_COORDS[2], TOP_CAP_TEX_COORDS[3],

                (rect.left+BAR_SIZE)/viewWidth, top/viewHeight, 0,
                TOP_CAP_TEX_COORDS[4], TOP_CAP_TEX_COORDS[5],

                (rect.left+BAR_SIZE)/viewWidth, (top+CAP_RADIUS)/viewHeight, 0,
                TOP_CAP_TEX_COORDS[6], TOP_CAP_TEX_COORDS[7]
            };

            coordBuffer.put(topCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            
            
            coordBuffer.position(0);

            
            float[] botCap = {
                
                rect.left/viewWidth, (bot-CAP_RADIUS)/viewHeight, 0,
                BOT_CAP_TEX_COORDS[0], BOT_CAP_TEX_COORDS[1],

                rect.left/viewWidth, (bot)/viewHeight, 0,
                BOT_CAP_TEX_COORDS[2], BOT_CAP_TEX_COORDS[3],

                (rect.left+BAR_SIZE)/viewWidth, (bot-CAP_RADIUS)/viewHeight, 0,
                BOT_CAP_TEX_COORDS[4], BOT_CAP_TEX_COORDS[5],

                (rect.left+BAR_SIZE)/viewWidth, (bot)/viewHeight, 0,
                BOT_CAP_TEX_COORDS[6], BOT_CAP_TEX_COORDS[7]
            };

            coordBuffer.put(botCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            
            
            coordBuffer.position(0);
        } else {
            
            float[] leftCap = {
                
                (rect.left-CAP_RADIUS)/viewWidth, bot/viewHeight, 0,
                LEFT_CAP_TEX_COORDS[0], LEFT_CAP_TEX_COORDS[1],
                (rect.left-CAP_RADIUS)/viewWidth, (bot+BAR_SIZE)/viewHeight, 0,
                LEFT_CAP_TEX_COORDS[2], LEFT_CAP_TEX_COORDS[3],
                (rect.left)/viewWidth, bot/viewHeight, 0, LEFT_CAP_TEX_COORDS[4],
                LEFT_CAP_TEX_COORDS[5],
                (rect.left)/viewWidth, (bot+BAR_SIZE)/viewHeight, 0,
                LEFT_CAP_TEX_COORDS[6], LEFT_CAP_TEX_COORDS[7]
            };

            coordBuffer.put(leftCap);

            
            coordBuffer.position(0);
            GLES20.glVertexAttribPointer(positionHandle, 3, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            
            
            coordBuffer.position(3);
            GLES20.glVertexAttribPointer(textureHandle, 2, GLES20.GL_FLOAT, false, 20,
                    coordBuffer);

            GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

            
            
            coordBuffer.position(0);

            
            float[] rightCap = {
                
                rect.right/viewWidth, (bot)/viewHeight, 0,
                RIGHT_CAP_TEX_COORDS[0], RIGHT_CAP_TEX_COORDS[1],

                rect.right/viewWidth, (bot+BAR_SIZE)/viewHeight, 0,
                RIGHT_CAP_TEX_COORDS[2], RIGHT_CAP_TEX_COORDS[3],

                (rect.right+CAP_RADIUS)/viewWidth, (bot)/viewHeight, 0,
                RIGHT_CAP_TEX_COORDS[4], RIGHT_CAP_TEX_COORDS[5],

                (rect.right+CAP_RADIUS)/viewWidth, (bot+BAR_SIZE)/viewHeight, 0,
                RIGHT_CAP_TEX_COORDS[6], RIGHT_CAP_TEX_COORDS[7]
            };

            coordBuffer.put(rightCap);

            
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

    private RectF getVerticalRect(RenderContext context) {
        RectF viewport = context.viewport;
        FloatSize pageSize = context.pageSize;
        float barStart = (viewport.height() * viewport.top / pageSize.height) + CAP_RADIUS;
        float barEnd = (viewport.height() * viewport.bottom / pageSize.height) - CAP_RADIUS;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        float right = viewport.width() - PADDING;
        return new RectF(right - BAR_SIZE, barStart, right, barEnd);
    }

    private RectF getHorizontalRect(RenderContext context) {
        RectF viewport = context.viewport;
        FloatSize pageSize = context.pageSize;
        float barStart = (viewport.width() * viewport.left / pageSize.width) + CAP_RADIUS;
        float barEnd = (viewport.width() * viewport.right / pageSize.width) - CAP_RADIUS;
        if (barStart > barEnd) {
            float middle = (barStart + barEnd) / 2.0f;
            barStart = barEnd = middle;
        }
        float bottom = viewport.height() - PADDING;
        return new RectF(barStart, bottom - BAR_SIZE, barEnd, bottom);
    }
}
