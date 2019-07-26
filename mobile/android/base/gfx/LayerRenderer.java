




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.gfx.Layer.RenderContext;
import org.mozilla.gecko.mozglue.DirectBufferAllocator;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.os.SystemClock;
import android.util.Log;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.concurrent.CopyOnWriteArrayList;

import javax.microedition.khronos.egl.EGLConfig;




public class LayerRenderer implements Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoLayerRenderer";
    private static final String PROFTAG = "GeckoLayerRendererProf";

    



    private static final int MAX_FRAME_TIME = 16;   

    private static final int FRAME_RATE_METER_WIDTH = 128;
    private static final int FRAME_RATE_METER_HEIGHT = 32;

    private final LayerView mView;
    private final NinePatchTileLayer mShadowLayer;
    private TextLayer mFrameRateLayer;
    private final ScrollbarLayer mHorizScrollLayer;
    private final ScrollbarLayer mVertScrollLayer;
    private final FadeRunnable mFadeRunnable;
    private ByteBuffer mCoordByteBuffer;
    private FloatBuffer mCoordBuffer;
    private RenderContext mLastPageContext;
    private int mMaxTextureSize;
    private int mBackgroundColor;
    private int mOverscrollColor;

    private CopyOnWriteArrayList<Layer> mExtraLayers = new CopyOnWriteArrayList<Layer>();

    
    private int[] mFrameTimings;
    private int mCurrentFrame, mFrameTimingsSum, mDroppedFrames;

    
    private int mFramesRendered;
    private float mCompleteFramesRendered;
    private boolean mProfileRender;
    private long mProfileOutputTime;

    
    private IntBuffer mPixelBuffer;

    
    private int mProgram;
    private int mPositionHandle;
    private int mTextureHandle;
    private int mSampleHandle;
    private int mTMatrixHandle;

    
    
    
    public static final float[] DEFAULT_TEXTURE_MATRIX = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    };

    private static final int COORD_BUFFER_SIZE = 20;

    
    

    
    
    

    public static final String DEFAULT_VERTEX_SHADER =
        "uniform mat4 uTMatrix;\n" +
        "attribute vec4 vPosition;\n" +
        "attribute vec2 aTexCoord;\n" +
        "varying vec2 vTexCoord;\n" +
        "void main() {\n" +
        "    gl_Position = uTMatrix * vPosition;\n" +
        "    vTexCoord.x = aTexCoord.x;\n" +
        "    vTexCoord.y = 1.0 - aTexCoord.y;\n" +
        "}\n";

    
    
    
    
    
    public static final String DEFAULT_FRAGMENT_SHADER =
        "precision highp float;\n" +
        "varying vec2 vTexCoord;\n" +
        "uniform sampler2D sTexture;\n" +
        "void main() {\n" +
        "    gl_FragColor = texture2D(sTexture, vTexCoord);\n" +
        "}\n";

    public LayerRenderer(LayerView view) {
        mView = view;
        mOverscrollColor = view.getContext().getResources().getColor(R.color.background_normal);

        CairoImage shadowImage = new BufferedCairoImage(view.getShadowPattern());
        mShadowLayer = new NinePatchTileLayer(shadowImage);

        Bitmap scrollbarImage = view.getScrollbarImage();
        IntSize size = new IntSize(scrollbarImage.getWidth(), scrollbarImage.getHeight());
        scrollbarImage = expandCanvasToPowerOfTwo(scrollbarImage, size);
        mVertScrollLayer = new ScrollbarLayer(this, scrollbarImage, size, true);
        mHorizScrollLayer = new ScrollbarLayer(this, diagonalFlip(scrollbarImage), new IntSize(size.height, size.width), false);
        mFadeRunnable = new FadeRunnable();

        mFrameTimings = new int[60];
        mCurrentFrame = mFrameTimingsSum = mDroppedFrames = 0;

        
        
        mCoordByteBuffer = DirectBufferAllocator.allocate(COORD_BUFFER_SIZE * 4);
        mCoordByteBuffer.order(ByteOrder.nativeOrder());
        mCoordBuffer = mCoordByteBuffer.asFloatBuffer();

        Tabs.registerOnTabsChangedListener(this);
    }

    private Bitmap expandCanvasToPowerOfTwo(Bitmap image, IntSize size) {
        IntSize potSize = size.nextPowerOfTwo();
        if (size.equals(potSize)) {
            return image;
        }
        
        Bitmap potImage = Bitmap.createBitmap(potSize.width, potSize.height, image.getConfig());
        new Canvas(potImage).drawBitmap(image, new Matrix(), null);
        return potImage;
    }

    private Bitmap diagonalFlip(Bitmap image) {
        Matrix rotation = new Matrix();
        rotation.setValues(new float[] { 0, 1, 0, 1, 0, 0, 0, 0, 1 }); 
        Bitmap rotated = Bitmap.createBitmap(image, 0, 0, image.getWidth(), image.getHeight(), rotation, true);
        return rotated;
    }

    public void destroy() {
        DirectBufferAllocator.free(mCoordByteBuffer);
        mCoordByteBuffer = null;
        mCoordBuffer = null;
        mShadowLayer.destroy();
        mHorizScrollLayer.destroy();
        mVertScrollLayer.destroy();
        if (mFrameRateLayer != null) {
            mFrameRateLayer.destroy();
        }
        Tabs.unregisterOnTabsChangedListener(this);
    }

    void onSurfaceCreated(EGLConfig config) {
        checkMonitoringEnabled();
        createDefaultProgram();
        activateDefaultProgram();
    }

    public void createDefaultProgram() {
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, DEFAULT_VERTEX_SHADER);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, DEFAULT_FRAGMENT_SHADER);

        mProgram = GLES20.glCreateProgram();
        GLES20.glAttachShader(mProgram, vertexShader);   
        GLES20.glAttachShader(mProgram, fragmentShader); 
        GLES20.glLinkProgram(mProgram);                  

        
        mPositionHandle = GLES20.glGetAttribLocation(mProgram, "vPosition");
        mTextureHandle = GLES20.glGetAttribLocation(mProgram, "aTexCoord");
        mSampleHandle = GLES20.glGetUniformLocation(mProgram, "sTexture");
        mTMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uTMatrix");

        int maxTextureSizeResult[] = new int[1];
        GLES20.glGetIntegerv(GLES20.GL_MAX_TEXTURE_SIZE, maxTextureSizeResult, 0);
        mMaxTextureSize = maxTextureSizeResult[0];
    }

    
    public void activateDefaultProgram() {
        
        GLES20.glUseProgram(mProgram);

        
        GLES20.glUniformMatrix4fv(mTMatrixHandle, 1, false, DEFAULT_TEXTURE_MATRIX, 0);

        
        GLES20.glEnableVertexAttribArray(mPositionHandle);
        GLES20.glEnableVertexAttribArray(mTextureHandle);

        GLES20.glUniform1i(mSampleHandle, 0);

        
        
    }

    
    
    public void deactivateDefaultProgram() {
        GLES20.glDisableVertexAttribArray(mTextureHandle);
        GLES20.glDisableVertexAttribArray(mPositionHandle);
        GLES20.glUseProgram(0);
    }

    public int getMaxTextureSize() {
        return mMaxTextureSize;
    }

    public void addLayer(Layer layer) {
        synchronized (mExtraLayers) {
            if (mExtraLayers.contains(layer)) {
                mExtraLayers.remove(layer);
            }

            mExtraLayers.add(layer);
        }
    }

    public void removeLayer(Layer layer) {
        synchronized (mExtraLayers) {
            mExtraLayers.remove(layer);
        }
    }

    private void printCheckerboardStats() {
        Log.d(PROFTAG, "Frames rendered over last 1000ms: " + mCompleteFramesRendered + "/" + mFramesRendered);
        mFramesRendered = 0;
        mCompleteFramesRendered = 0;
    }

    
    IntBuffer getPixels() {
        IntBuffer pixelBuffer = IntBuffer.allocate(mView.getWidth() * mView.getHeight());
        synchronized (pixelBuffer) {
            mPixelBuffer = pixelBuffer;
            mView.requestRender();
            try {
                pixelBuffer.wait();
            } catch (InterruptedException ie) {
            }
            mPixelBuffer = null;
        }
        return pixelBuffer;
    }

    private RenderContext createScreenContext(ImmutableViewportMetrics metrics) {
        RectF viewport = new RectF(0.0f, 0.0f, metrics.getWidth(), metrics.getHeight());
        RectF pageRect = new RectF(metrics.getPageRect());
        return createContext(viewport, pageRect, 1.0f);
    }

    private RenderContext createPageContext(ImmutableViewportMetrics metrics) {
        Rect viewport = RectUtils.round(metrics.getViewport());
        RectF pageRect = metrics.getPageRect();
        float zoomFactor = metrics.zoomFactor;
        return createContext(new RectF(viewport), pageRect, zoomFactor);
    }

    private RenderContext createContext(RectF viewport, RectF pageRect, float zoomFactor) {
        return new RenderContext(viewport, pageRect, zoomFactor, mPositionHandle, mTextureHandle,
                                 mCoordBuffer);
    }

    private void updateDroppedFrames(long frameStartTime) {
        int frameElapsedTime = (int)(SystemClock.uptimeMillis() - frameStartTime);

        
        mFrameTimingsSum -= mFrameTimings[mCurrentFrame];
        mFrameTimingsSum += frameElapsedTime;
        mDroppedFrames -= (mFrameTimings[mCurrentFrame] + 1) / MAX_FRAME_TIME;
        mDroppedFrames += (frameElapsedTime + 1) / MAX_FRAME_TIME;

        mFrameTimings[mCurrentFrame] = frameElapsedTime;
        mCurrentFrame = (mCurrentFrame + 1) % mFrameTimings.length;

        int averageTime = mFrameTimingsSum / mFrameTimings.length;
        mFrameRateLayer.beginTransaction();     
        try {
            mFrameRateLayer.setText(averageTime + " ms/" + mDroppedFrames);
        } finally {
            mFrameRateLayer.endTransaction();
        }
    }

    
    private void moveFrameRateLayer(int width, int height) {
        mFrameRateLayer.beginTransaction();     
        try {
            Rect position = new Rect(width - FRAME_RATE_METER_WIDTH - 8,
                                    height - FRAME_RATE_METER_HEIGHT + 8,
                                    width - 8,
                                    height + 8);
            mFrameRateLayer.setPosition(position);
        } finally {
            mFrameRateLayer.endTransaction();
        }
    }

    void checkMonitoringEnabled() {
        
        new Thread(new Runnable() {
            @Override
            public void run() {
                Context context = mView.getContext();
                SharedPreferences preferences = context.getSharedPreferences("GeckoApp", 0);
                if (preferences.getBoolean("showFrameRate", false)) {
                    IntSize frameRateLayerSize = new IntSize(FRAME_RATE_METER_WIDTH, FRAME_RATE_METER_HEIGHT);
                    mFrameRateLayer = TextLayer.create(frameRateLayerSize, "-- ms/--");
                    moveFrameRateLayer(mView.getWidth(), mView.getHeight());
                }
                mProfileRender = Log.isLoggable(PROFTAG, Log.DEBUG);
            }
        }).start();
    }

    



    public static int loadShader(int type, String shaderCode) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);
        return shader;
    }

    public Frame createFrame(ImmutableViewportMetrics metrics) {
        return new Frame(metrics);
    }

    class FadeRunnable implements Runnable {
        private boolean mStarted;
        private long mRunAt;

        void scheduleStartFade(long delay) {
            mRunAt = SystemClock.elapsedRealtime() + delay;
            if (!mStarted) {
                mView.postDelayed(this, delay);
                mStarted = true;
            }
        }

        void scheduleNextFadeFrame() {
            if (mStarted) {
                Log.e(LOGTAG, "scheduleNextFadeFrame() called while scheduled for starting fade");
            }
            mView.postDelayed(this, 1000L / 60L); 
        }

        boolean timeToFade() {
            return !mStarted;
        }

        @Override
        public void run() {
            long timeDelta = mRunAt - SystemClock.elapsedRealtime();
            if (timeDelta > 0) {
                
                mView.postDelayed(this, timeDelta);
            } else {
                
                mStarted = false;
                mView.requestRender();
            }
        }
    }

    public class Frame {
        
        private long mFrameStartTime;
        
        private ImmutableViewportMetrics mFrameMetrics;
        
        private RenderContext mPageContext, mScreenContext;
        
        private boolean mUpdated;
        private final Rect mPageRect;
        private final Rect mAbsolutePageRect;

        public Frame(ImmutableViewportMetrics metrics) {
            mFrameMetrics = metrics;
            mPageContext = createPageContext(metrics);
            mScreenContext = createScreenContext(metrics);

            Point origin = PointUtils.round(mFrameMetrics.getOrigin());
            Rect pageRect = RectUtils.round(mFrameMetrics.getPageRect());
            mAbsolutePageRect = new Rect(pageRect);
            pageRect.offset(-origin.x, -origin.y);
            mPageRect = pageRect;
        }

        private void setScissorRect() {
            Rect scissorRect = transformToScissorRect(mPageRect);
            GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
            GLES20.glScissor(scissorRect.left, scissorRect.top,
                             scissorRect.width(), scissorRect.height());
        }

        private Rect transformToScissorRect(Rect rect) {
            IntSize screenSize = new IntSize(mFrameMetrics.getSize());

            int left = Math.max(0, rect.left);
            int top = Math.max(0, rect.top);
            int right = Math.min(screenSize.width, rect.right);
            int bottom = Math.min(screenSize.height, rect.bottom);

            return new Rect(left, screenSize.height - bottom, right,
                            (screenSize.height - bottom) + (bottom - top));
        }

        
        public void beginDrawing() {
            mFrameStartTime = SystemClock.uptimeMillis();

            TextureReaper.get().reap();
            TextureGenerator.get().fill();

            mUpdated = true;

            Layer rootLayer = mView.getLayerClient().getRoot();

            if (!mPageContext.fuzzyEquals(mLastPageContext) && !mView.isFullScreen()) {
                
                
                mVertScrollLayer.unfade();
                mHorizScrollLayer.unfade();
                mFadeRunnable.scheduleStartFade(ScrollbarLayer.FADE_DELAY);
            } else if (mFadeRunnable.timeToFade()) {
                boolean stillFading = mVertScrollLayer.fade() | mHorizScrollLayer.fade();
                if (stillFading) {
                    mFadeRunnable.scheduleNextFadeFrame();
                }
            }
            mLastPageContext = mPageContext;

            
            if (rootLayer != null) mUpdated &= rootLayer.update(mPageContext);  
            mUpdated &= mShadowLayer.update(mPageContext);  
            if (mFrameRateLayer != null) mUpdated &= mFrameRateLayer.update(mScreenContext); 
            mUpdated &= mVertScrollLayer.update(mPageContext);  
            mUpdated &= mHorizScrollLayer.update(mPageContext); 

            for (Layer layer : mExtraLayers)
                mUpdated &= layer.update(mPageContext); 
        }

        





        private Rect getMaskForLayer(Layer layer) {
            if (layer == null) {
                return null;
            }

            RectF bounds = RectUtils.contract(layer.getBounds(mPageContext), 1.0f, 1.0f);
            Rect mask = RectUtils.roundIn(bounds);

            
            
            
            if (mask.top <= 2) {
                mask.top = -1;
            }
            if (mask.left <= 2) {
                mask.left = -1;
            }

            
            
            int pageRight = mPageRect.width();
            int pageBottom = mPageRect.height();

            if (mask.right >= pageRight - 2) {
                mask.right = pageRight + 1;
            }
            if (mask.bottom >= pageBottom - 2) {
                mask.bottom = pageBottom + 1;
            }

            return mask;
        }

        private void clear(int color) {
            GLES20.glClearColor(((color >> 16) & 0xFF) / 255.0f,
                                ((color >> 8) & 0xFF) / 255.0f,
                                (color & 0xFF) / 255.0f,
                                0.0f);
            
            
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT |
                           GLES20.GL_DEPTH_BUFFER_BIT);
        }

        
        public void drawBackground() {
            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);

            
            clear(mOverscrollColor);

            
            mBackgroundColor = mView.getBackgroundColor();

            
            setScissorRect();
            clear(mBackgroundColor);
            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);

            
            if (!new RectF(mAbsolutePageRect).contains(mFrameMetrics.getViewport()))
                mShadowLayer.draw(mPageContext);
        }

        
        void drawRootLayer() {
            Layer rootLayer = mView.getLayerClient().getRoot();
            if (rootLayer == null) {
                return;
            }

            rootLayer.draw(mPageContext);
        }

        
        public void drawForeground() {
            
            if (mExtraLayers.size() > 0) {
                for (Layer layer : mExtraLayers) {
                    layer.draw(mPageContext);
                }
            }

            
            if (mPageRect.height() > mFrameMetrics.getHeight())
                mVertScrollLayer.draw(mPageContext);

            
            if (mPageRect.width() > mFrameMetrics.getWidth())
                mHorizScrollLayer.draw(mPageContext);

            
            Layer rootLayer = mView.getLayerClient().getRoot();
            if ((rootLayer != null) &&
                (mProfileRender || PanningPerfAPI.isRecordingCheckerboard())) {
                
                float checkerboard =  1.0f - GeckoAppShell.computeRenderIntegrity();

                PanningPerfAPI.recordCheckerboard(checkerboard);
                if (checkerboard < 0.0f || checkerboard > 1.0f) {
                    Log.e(LOGTAG, "Checkerboard value out of bounds: " + checkerboard);
                }

                mCompleteFramesRendered += 1.0f - checkerboard;
                mFramesRendered ++;

                if (mFrameStartTime - mProfileOutputTime > 1000) {
                    mProfileOutputTime = mFrameStartTime;
                    printCheckerboardStats();
                }
            }

            
            if (mFrameRateLayer != null) {
                updateDroppedFrames(mFrameStartTime);

                GLES20.glEnable(GLES20.GL_BLEND);
                GLES20.glBlendFunc(GLES20.GL_SRC_ALPHA, GLES20.GL_ONE_MINUS_SRC_ALPHA);
                mFrameRateLayer.draw(mScreenContext);
            }
        }

        
        public void endDrawing() {
            
            if (!mUpdated)
                mView.requestRender();

            PanningPerfAPI.recordFrameTime();

            
            IntBuffer pixelBuffer = mPixelBuffer;
            if (mUpdated && pixelBuffer != null) {
                synchronized (pixelBuffer) {
                    pixelBuffer.position(0);
                    GLES20.glReadPixels(0, 0, (int)mScreenContext.viewport.width(),
                                        (int)mScreenContext.viewport.height(), GLES20.GL_RGBA,
                                        GLES20.GL_UNSIGNED_BYTE, pixelBuffer);
                    pixelBuffer.notify();
                }
            }

            
            
            
            if (mView.getPaintState() == LayerView.PAINT_BEFORE_FIRST) {
                mView.post(new Runnable() {
                    @Override
                    public void run() {
                        mView.getChildAt(0).setBackgroundColor(Color.TRANSPARENT);
                    }
                });
                mView.setPaintState(LayerView.PAINT_AFTER_FIRST);
            }
        }
    }

    @Override
    public void onTabChanged(final Tab tab, Tabs.TabEvents msg, Object data) {
        
        
        
        
        if (msg == Tabs.TabEvents.SELECTED) {
            mView.getChildAt(0).setBackgroundColor(tab.getBackgroundColor());
            mView.setPaintState(LayerView.PAINT_START);
        }
    }
}
