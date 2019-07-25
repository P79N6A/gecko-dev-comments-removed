






































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.Layer.RenderContext;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.NinePatchTileLayer;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.gfx.TextureReaper;
import org.mozilla.gecko.gfx.TextureGenerator;
import org.mozilla.gecko.gfx.TextLayer;
import org.mozilla.gecko.gfx.TileLayer;
import org.mozilla.gecko.GeckoAppShell;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.RegionIterator;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.ArrayList;




public class LayerRenderer implements GLSurfaceView.Renderer {
    private static final String LOGTAG = "GeckoLayerRenderer";
    private static final String PROFTAG = "GeckoLayerRendererProf";

    



    private static final int MAX_FRAME_TIME = 16;   

    private static final int FRAME_RATE_METER_WIDTH = 128;
    private static final int FRAME_RATE_METER_HEIGHT = 32;

    private final LayerView mView;
    private final SingleTileLayer mBackgroundLayer;
    private final CheckerboardImage mCheckerboardImage;
    private final SingleTileLayer mCheckerboardLayer;
    private final NinePatchTileLayer mShadowLayer;
    private TextLayer mFrameRateLayer;
    private final ScrollbarLayer mHorizScrollLayer;
    private final ScrollbarLayer mVertScrollLayer;
    private final FadeRunnable mFadeRunnable;
    private final FloatBuffer mCoordBuffer;
    private RenderContext mLastPageContext;
    private int mMaxTextureSize;

    private ArrayList<Layer> mExtraLayers = new ArrayList<Layer>();

    
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

    
    
    
    private static final float[] TEXTURE_MATRIX = {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f
    };

    private static final int COORD_BUFFER_SIZE = 20;

    
    
    private static final String VERTEX_SHADER =
        "uniform mat4 uTMatrix;\n" +
        "attribute vec4 vPosition;\n" +
        "attribute vec2 aTexCoord;\n" +
        "varying vec2 vTexCoord;\n" +
        "void main() {\n" +
        "    gl_Position = uTMatrix * vPosition;\n" +
        "    vTexCoord = aTexCoord;\n" +
        "}\n";

    
    
    
    private static final String FRAGMENT_SHADER =
        "precision mediump float;\n" +
        "varying vec2 vTexCoord;\n" +
        "uniform sampler2D sTexture;\n" +
        "void main() {\n" +
        "    gl_FragColor = texture2D(sTexture, vec2(vTexCoord.x, 1.0 - vTexCoord.y));\n" +
        "}\n";

    public LayerRenderer(LayerView view) {
        mView = view;

        LayerController controller = view.getController();

        CairoImage backgroundImage = new BufferedCairoImage(controller.getBackgroundPattern());
        mBackgroundLayer = new SingleTileLayer(true, backgroundImage);

        mCheckerboardImage = new CheckerboardImage();
        mCheckerboardLayer = new SingleTileLayer(true, mCheckerboardImage);

        CairoImage shadowImage = new BufferedCairoImage(controller.getShadowPattern());
        mShadowLayer = new NinePatchTileLayer(shadowImage);

        mHorizScrollLayer = ScrollbarLayer.create(false);
        mVertScrollLayer = ScrollbarLayer.create(true);
        mFadeRunnable = new FadeRunnable();

        mFrameTimings = new int[60];
        mCurrentFrame = mFrameTimingsSum = mDroppedFrames = 0;

        
        
        ByteBuffer byteBuffer = GeckoAppShell.allocateDirectBuffer(COORD_BUFFER_SIZE * 4);
        byteBuffer.order(ByteOrder.nativeOrder());
        mCoordBuffer = byteBuffer.asFloatBuffer();
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        checkMonitoringEnabled();
        createProgram();
        activateProgram();
    }

    public void createProgram() {
        int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, VERTEX_SHADER);
        int fragmentShader = loadShader(GLES20.GL_FRAGMENT_SHADER, FRAGMENT_SHADER);

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

    
    public void activateProgram() {
        
        GLES20.glUseProgram(mProgram);

        
        GLES20.glUniformMatrix4fv(mTMatrixHandle, 1, false, TEXTURE_MATRIX, 0);

        Log.e(LOGTAG, "### Position handle is " + mPositionHandle + ", texture handle is " +
              mTextureHandle + ", last error is " + GLES20.glGetError());

        
        GLES20.glEnableVertexAttribArray(mPositionHandle);
        GLES20.glEnableVertexAttribArray(mTextureHandle);

        GLES20.glUniform1i(mSampleHandle, 0);

        TextureGenerator.get().fill();

        
        
    }

    
    
    public void deactivateProgram() {
        GLES20.glDisableVertexAttribArray(mTextureHandle);
        GLES20.glDisableVertexAttribArray(mPositionHandle);
        GLES20.glUseProgram(0);
    }

    public int getMaxTextureSize() {
        return mMaxTextureSize;
    }

    public void addLayer(Layer layer) {
        LayerController controller = mView.getController();

        synchronized (controller) {
            if (mExtraLayers.contains(layer)) {
                mExtraLayers.remove(layer);
            }

            mExtraLayers.add(layer);
        }
    }

    public void removeLayer(Layer layer) {
        LayerController controller = mView.getController();

        synchronized (controller) {
            mExtraLayers.remove(layer);
        }
    }

    


    public void onDrawFrame(GL10 gl) {
        RenderContext pageContext = createPageContext(), screenContext = createScreenContext();
        Frame frame = createFrame(pageContext, screenContext);
        synchronized (mView.getController()) {
            frame.beginDrawing();
            frame.drawBackground();
            frame.drawRootLayer();
            frame.drawForeground();
            frame.endDrawing();
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

    public RenderContext createScreenContext() {
        LayerController layerController = mView.getController();
        IntSize viewportSize = new IntSize(layerController.getViewportSize());
        RectF viewport = new RectF(0.0f, 0.0f, viewportSize.width, viewportSize.height);
        FloatSize pageSize = new FloatSize(layerController.getPageSize());
        return createContext(viewport, pageSize, 1.0f);
    }

    public RenderContext createPageContext() {
        LayerController layerController = mView.getController();

        Rect viewport = new Rect();
        layerController.getViewport().round(viewport);

        FloatSize pageSize = new FloatSize(layerController.getPageSize());
        float zoomFactor = layerController.getZoomFactor();
        return createContext(new RectF(viewport), pageSize, zoomFactor);
    }

    private RenderContext createContext(RectF viewport, FloatSize pageSize, float zoomFactor) {
        return new RenderContext(viewport, pageSize, zoomFactor, mPositionHandle, mTextureHandle,
                                 mCoordBuffer);
    }

    private Rect getPageRect() {
        LayerController controller = mView.getController();

        Point origin = PointUtils.round(controller.getOrigin());
        IntSize pageSize = new IntSize(controller.getPageSize());

        origin.negate();

        return new Rect(origin.x, origin.y,
                        origin.x + pageSize.width, origin.y + pageSize.height);
    }

    private Rect transformToScissorRect(Rect rect) {
        LayerController controller = mView.getController();
        IntSize screenSize = new IntSize(controller.getViewportSize());

        int left = Math.max(0, rect.left);
        int top = Math.max(0, rect.top);
        int right = Math.min(screenSize.width, rect.right);
        int bottom = Math.min(screenSize.height, rect.bottom);

        return new Rect(left, screenSize.height - bottom, right,
                        (screenSize.height - bottom) + (bottom - top));
    }

    public void onSurfaceChanged(GL10 gl, final int width, final int height) {
        GLES20.glViewport(0, 0, width, height);

        if (mFrameRateLayer != null) {
            moveFrameRateLayer(width, height);
        }

        
        
        mView.post(new Runnable() {
            public void run() {
                mView.setViewportSize(new IntSize(width, height));
            }
        });

        
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

    private void updateCheckerboardLayer(RenderContext renderContext) {
        int checkerboardColor = mView.getController().getCheckerboardColor();
        boolean showChecks = mView.getController().checkerboardShouldShowChecks();
        if (checkerboardColor == mCheckerboardImage.getColor() &&
            showChecks == mCheckerboardImage.getShowChecks()) {
            return;
        }

        mCheckerboardLayer.beginTransaction();  
        try {
            mCheckerboardImage.update(showChecks, checkerboardColor);
            mCheckerboardLayer.invalidate();
        } finally {
            mCheckerboardLayer.endTransaction();
        }

        mCheckerboardLayer.update(renderContext);   
    }

    



    private int loadShader(int type, String shaderCode) {
        int shader = GLES20.glCreateShader(type);
        GLES20.glShaderSource(shader, shaderCode);
        GLES20.glCompileShader(shader);
        return shader;
    }

    public Frame createFrame(RenderContext pageContext, RenderContext screenContext) {
        return new Frame(pageContext, screenContext);
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
        
        private RenderContext mPageContext, mScreenContext;
        
        private boolean mUpdated;

        public Frame(RenderContext pageContext, RenderContext screenContext) {
            mPageContext = pageContext;
            mScreenContext = screenContext;
        }

        private void setScissorRect() {
            Rect scissorRect = transformToScissorRect(getPageRect());
            GLES20.glEnable(GLES20.GL_SCISSOR_TEST);
            GLES20.glScissor(scissorRect.left, scissorRect.top,
                             scissorRect.width(), scissorRect.height());
        }

        
        public void beginDrawing() {
            mFrameStartTime = SystemClock.uptimeMillis();

            TextureReaper.get().reap();
            TextureGenerator.get().fill();

            mUpdated = true;

            LayerController controller = mView.getController();
            Layer rootLayer = controller.getRoot();

            if (!mPageContext.fuzzyEquals(mLastPageContext)) {
                
                
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
            mUpdated &= mBackgroundLayer.update(mScreenContext);    
            mUpdated &= mShadowLayer.update(mPageContext);  
            updateCheckerboardLayer(mScreenContext);
            if (mFrameRateLayer != null) mUpdated &= mFrameRateLayer.update(mScreenContext); 
            mUpdated &= mVertScrollLayer.update(mPageContext);  
            mUpdated &= mHorizScrollLayer.update(mPageContext); 

            for (Layer layer : mExtraLayers)
                mUpdated &= layer.update(mPageContext); 

            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);

            
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
        }

        
        public void drawBackground() {
            
            mBackgroundLayer.setMask(getPageRect());
            mBackgroundLayer.draw(mScreenContext);

            
            Rect pageRect = getPageRect();
            RectF untransformedPageRect = new RectF(0.0f, 0.0f, pageRect.width(),
                                                    pageRect.height());
            if (!untransformedPageRect.contains(mView.getController().getViewport()))
                mShadowLayer.draw(mPageContext);

            
            Rect rootMask = null;
            Layer rootLayer = mView.getController().getRoot();
            if (rootLayer != null) {
                RectF rootBounds = rootLayer.getBounds(mPageContext);
                rootBounds.offset(-mPageContext.viewport.left, -mPageContext.viewport.top);
                rootMask = new Rect();
                rootBounds.roundOut(rootMask);
            }

            
            setScissorRect();
            mCheckerboardLayer.setMask(rootMask);
            mCheckerboardLayer.draw(mScreenContext);
            GLES20.glDisable(GLES20.GL_SCISSOR_TEST);
        }

        
        void drawRootLayer() {
            Layer rootLayer = mView.getController().getRoot();
            if (rootLayer == null) {
                return;
            }

            rootLayer.draw(mPageContext);
        }

        
        public void drawForeground() {
            Rect pageRect = getPageRect();
            LayerController controller = mView.getController();

            
            for (Layer layer : mExtraLayers)
                layer.draw(mPageContext);

            
            IntSize screenSize = new IntSize(controller.getViewportSize());
            if (pageRect.height() > screenSize.height)
                mVertScrollLayer.draw(mPageContext);

            
            if (pageRect.width() > screenSize.width)
                mHorizScrollLayer.draw(mPageContext);

            
            Layer rootLayer = controller.getRoot();
            if ((rootLayer != null) &&
                (mProfileRender || PanningPerfAPI.isRecordingCheckerboard())) {
                
                Rect viewport = RectUtils.round(mPageContext.viewport);
                Region validRegion = rootLayer.getValidRegion(mPageContext);
                validRegion.op(viewport, Region.Op.INTERSECT);

                float checkerboard = 0.0f;
                if (!(validRegion.isRect() && validRegion.getBounds().equals(viewport))) {
                    int screenArea = viewport.width() * viewport.height();
                    validRegion.op(viewport, Region.Op.REVERSE_DIFFERENCE);

                    
                    
                    
                    
                    
                    Rect r = new Rect();
                    int checkerboardArea = 0;
                    for (RegionIterator i = new RegionIterator(validRegion); i.next(r);) {
                        checkerboardArea += r.width() * r.height();
                    }

                    checkerboard = checkerboardArea / (float)screenArea;
                }

                PanningPerfAPI.recordCheckerboard(checkerboard);

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
        }
    }
}
