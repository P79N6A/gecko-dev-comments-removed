




package org.mozilla.gecko.gfx;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.gfx.Layer.RenderContext;
import org.mozilla.gecko.gfx.RenderTask;
import org.mozilla.gecko.mozglue.DirectBufferAllocator;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLES20;
import android.os.SystemClock;
import android.util.Log;

import org.mozilla.gecko.mozglue.JNITarget;
import org.mozilla.gecko.util.ThreadUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.ArrayList;
import java.util.List;

import javax.microedition.khronos.egl.EGLConfig;




public class LayerRenderer implements Tabs.OnTabsChangedListener {
    private static final String LOGTAG = "GeckoLayerRenderer";
    private static final String PROFTAG = "GeckoLayerRendererProf";

    



    private static final int MAX_FRAME_TIME = 16;   

    private static final int FRAME_RATE_METER_WIDTH = 128;
    private static final int FRAME_RATE_METER_HEIGHT = 32;

    private static final long NANOS_PER_MS = 1000000;
    private static final int NANOS_PER_SECOND = 1000000000;

    private static final int MAX_SCROLL_SPEED_TO_REQUEST_ZOOM_RENDER = 5;

    private final LayerView mView;
    private final ScrollbarLayer mHorizScrollLayer;
    private final ScrollbarLayer mVertScrollLayer;
    private final FadeRunnable mFadeRunnable;
    private ByteBuffer mCoordByteBuffer;
    private FloatBuffer mCoordBuffer;
    private RenderContext mLastPageContext;
    private int mMaxTextureSize;
    private int mBackgroundColor;
    private int mOverscrollColor;

    private long mLastFrameTime;
    private final CopyOnWriteArrayList<RenderTask> mTasks;

    private final CopyOnWriteArrayList<Layer> mExtraLayers = new CopyOnWriteArrayList<Layer>();

    
    private final int[] mFrameTimings;
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

    private List<LayerView.ZoomedViewListener> mZoomedViewListeners;
    private float mLastViewLeft;
    private float mLastViewTop;

    
    
    
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
        setOverscrollColor(R.color.toolbar_grey);

        Bitmap scrollbarImage = view.getScrollbarImage();
        IntSize size = new IntSize(scrollbarImage.getWidth(), scrollbarImage.getHeight());
        scrollbarImage = expandCanvasToPowerOfTwo(scrollbarImage, size);

        mTasks = new CopyOnWriteArrayList<RenderTask>();
        mLastFrameTime = System.nanoTime();

        mVertScrollLayer = new ScrollbarLayer(this, scrollbarImage, size, true);
        mHorizScrollLayer = new ScrollbarLayer(this, diagonalFlip(scrollbarImage), new IntSize(size.height, size.width), false);
        mFadeRunnable = new FadeRunnable();

        mFrameTimings = new int[60];
        mCurrentFrame = mFrameTimingsSum = mDroppedFrames = 0;

        
        
        mCoordByteBuffer = DirectBufferAllocator.allocate(COORD_BUFFER_SIZE * 4);
        mCoordByteBuffer.order(ByteOrder.nativeOrder());
        mCoordBuffer = mCoordByteBuffer.asFloatBuffer();

        Tabs.registerOnTabsChangedListener(this);
        mZoomedViewListeners = new ArrayList<LayerView.ZoomedViewListener>();
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
        mHorizScrollLayer.destroy();
        mVertScrollLayer.destroy();
        Tabs.unregisterOnTabsChangedListener(this);
        mZoomedViewListeners.clear();
    }

    void onSurfaceCreated(EGLConfig config) {
        checkMonitoringEnabled();
        createDefaultProgram();
        activateDefaultProgram();
    }

    void setOverscrollColor(int colorId) {
        try {
            mOverscrollColor = mView.getContext().getResources().getColor(colorId);
        } catch (Resources.NotFoundException nfe) { mOverscrollColor = Color.BLACK; }
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

    void restoreState(boolean enableScissor, int scissorX, int scissorY, int scissorW, int scissorH) {
        GLES20.glScissor(scissorX, scissorY, scissorW, scissorH);
        if (enableScissor) {
            GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
        } else {
            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        }
    }

    public int getMaxTextureSize() {
        return mMaxTextureSize;
    }

    public void postRenderTask(RenderTask aTask) {
        mTasks.add(aTask);
        mView.requestRender();
    }

    public void removeRenderTask(RenderTask aTask) {
        mTasks.remove(aTask);
    }

    private void runRenderTasks(CopyOnWriteArrayList<RenderTask> tasks, boolean after, long frameStartTime) {
        for (RenderTask task : tasks) {
            if (task.runAfter != after) {
                continue;
            }

            boolean stillRunning = task.run(frameStartTime - mLastFrameTime, frameStartTime);

            
            if (!stillRunning) {
                tasks.remove(task);
            }
        }
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

    private RenderContext createScreenContext(ImmutableViewportMetrics metrics, PointF offset) {
        RectF viewport = new RectF(0.0f, 0.0f, metrics.getWidth(), metrics.getHeight());
        RectF pageRect = metrics.getPageRect();

        return createContext(viewport, pageRect, 1.0f, offset);
    }

    private RenderContext createPageContext(ImmutableViewportMetrics metrics, PointF offset) {
        RectF viewport = metrics.getViewport();
        RectF pageRect = metrics.getPageRect();
        float zoomFactor = metrics.zoomFactor;

        return createContext(new RectF(RectUtils.round(viewport)), pageRect, zoomFactor, offset);
    }

    private RenderContext createContext(RectF viewport, RectF pageRect, float zoomFactor, PointF offset) {
        if (mCoordBuffer == null) {
            throw new IllegalStateException();
        }
        return new RenderContext(viewport, pageRect, zoomFactor, offset, mPositionHandle, mTextureHandle,
                                 mCoordBuffer);
    }

    private void updateDroppedFrames(long frameStartTime) {
        int frameElapsedTime = (int)((System.nanoTime() - frameStartTime) / NANOS_PER_MS);

        
        mFrameTimingsSum -= mFrameTimings[mCurrentFrame];
        mFrameTimingsSum += frameElapsedTime;
        mDroppedFrames -= (mFrameTimings[mCurrentFrame] + 1) / MAX_FRAME_TIME;
        mDroppedFrames += (frameElapsedTime + 1) / MAX_FRAME_TIME;

        mFrameTimings[mCurrentFrame] = frameElapsedTime;
        mCurrentFrame = (mCurrentFrame + 1) % mFrameTimings.length;

        int averageTime = mFrameTimingsSum / mFrameTimings.length;
    }

    void checkMonitoringEnabled() {
        mProfileRender = Log.isLoggable(PROFTAG, Log.DEBUG);
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
        
        private final ImmutableViewportMetrics mFrameMetrics;
        
        private final RenderContext mPageContext, mScreenContext;
        
        private boolean mUpdated;
        private final Rect mPageRect;
        private final Rect mAbsolutePageRect;
        private final PointF mRenderOffset;

        public Frame(ImmutableViewportMetrics metrics) {
            mFrameMetrics = metrics;

            
            Layer rootLayer = mView.getLayerClient().getRoot();
            mRenderOffset = mFrameMetrics.getMarginOffset();
            mPageContext = createPageContext(metrics, mRenderOffset);
            mScreenContext = createScreenContext(metrics, mRenderOffset);

            RectF pageRect = mFrameMetrics.getPageRect();
            mAbsolutePageRect = RectUtils.round(pageRect);

            PointF origin = mFrameMetrics.getOrigin();
            pageRect.offset(-origin.x, -origin.y);
            mPageRect = RectUtils.round(pageRect);
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

            Rect scissorRect = new Rect(left, screenSize.height - bottom, right,
                                        (screenSize.height - bottom) + (bottom - top));
            scissorRect.offset(Math.round(-mRenderOffset.x), Math.round(-mRenderOffset.y));

            return scissorRect;
        }

        
        @JNITarget
        public void beginDrawing() {
            mFrameStartTime = System.nanoTime();

            TextureReaper.get().reap();
            TextureGenerator.get().fill();

            mUpdated = true;

            Layer rootLayer = mView.getLayerClient().getRoot();

            
            runRenderTasks(mTasks, false, mFrameStartTime);

            boolean hideScrollbars = (mView.getFullScreenState() == FullScreenState.NON_ROOT_ELEMENT);
            if (!mPageContext.fuzzyEquals(mLastPageContext) && !hideScrollbars) {
                
                
                
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

            
            if (rootLayer != null) {
                
                mUpdated &= rootLayer.update(mPageContext);
            }

            mUpdated &= mVertScrollLayer.update(mPageContext);  
            mUpdated &= mHorizScrollLayer.update(mPageContext); 

            for (Layer layer : mExtraLayers) {
                mUpdated &= layer.update(mPageContext); 
            }
        }

        private void clear(int color) {
            GLES20.glClearColor(((color >> 16) & 0xFF) / 255.0f,
                                ((color >> 8) & 0xFF) / 255.0f,
                                (color & 0xFF) / 255.0f,
                                0.0f);
            
            
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT |
                           GLES20.GL_DEPTH_BUFFER_BIT);
        }

        
        @JNITarget
        public void drawBackground() {
            
            

            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);

            
            clear(mOverscrollColor);

            
            mBackgroundColor = mView.getBackgroundColor();

            
            setScissorRect();
            clear(mBackgroundColor);
            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        }

        @JNITarget
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

                if (mFrameStartTime - mProfileOutputTime > NANOS_PER_SECOND) {
                    mProfileOutputTime = mFrameStartTime;
                    printCheckerboardStats();
                }
            }

            runRenderTasks(mTasks, true, mFrameStartTime);

        }

        private void maybeRequestZoomedViewRender(RenderContext context) {
            
            
            if (mZoomedViewListeners.size() == 0) {
                return;
            }

            
            
            
            final float viewLeft = context.viewport.left - context.offset.x;
            final float viewTop = context.viewport.top - context.offset.y;
            boolean shouldWaitToRender = false;

            if (Math.abs(mLastViewLeft - viewLeft) > MAX_SCROLL_SPEED_TO_REQUEST_ZOOM_RENDER ||
                Math.abs(mLastViewTop - viewTop) > MAX_SCROLL_SPEED_TO_REQUEST_ZOOM_RENDER) {
                shouldWaitToRender = true;
            }

            mLastViewLeft = viewLeft;
            mLastViewTop = viewTop;

            if (shouldWaitToRender) {
                return;
            }

            ThreadUtils.postToUiThread(new Runnable() {
                @Override
                public void run() {
                    for (LayerView.ZoomedViewListener listener : mZoomedViewListeners) {
                        listener.requestZoomedViewRender();
                    }
                }
            });
        }

        
        @JNITarget
        public void endDrawing() {
            
            if (!mUpdated)
                mView.requestRender();

            PanningPerfAPI.recordFrameTime();

            maybeRequestZoomedViewRender(mPageContext);

            
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
            mLastFrameTime = mFrameStartTime;
        }
    }

    @Override
    public void onTabChanged(final Tab tab, Tabs.TabEvents msg, Object data) {
        
        
        
        
        if (msg == Tabs.TabEvents.SELECTED) {
            if (mView != null) {
                final int overscrollColor =
                        (tab.isPrivate() ? R.color.private_toolbar_grey : R.color.toolbar_grey);
                setOverscrollColor(overscrollColor);

                if (mView.getChildAt(0) != null) {
                    mView.getChildAt(0).setBackgroundColor(tab.getBackgroundColor());
                }
                mView.setPaintState(LayerView.PAINT_START);
            }
        }
    }

    public void updateZoomedView(final ByteBuffer data) {
        ThreadUtils.postToUiThread(new Runnable() {
            @Override
            public void run() {
                for (LayerView.ZoomedViewListener listener : mZoomedViewListeners) {
                    data.position(0);
                    listener.updateView(data);
                }
            }
        });
    }

    public void addZoomedViewListener(LayerView.ZoomedViewListener listener) {
        ThreadUtils.assertOnUiThread();
        mZoomedViewListeners.add(listener);
    }

    public void removeZoomedViewListener(LayerView.ZoomedViewListener listener) {
        ThreadUtils.assertOnUiThread();
        mZoomedViewListeners.remove(listener);
    }
}
