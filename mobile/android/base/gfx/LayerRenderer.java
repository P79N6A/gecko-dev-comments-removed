





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.Layer.RenderContext;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.NinePatchTileLayer;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.gfx.TextureReaper;
import org.mozilla.gecko.gfx.TextureGenerator;
import org.mozilla.gecko.gfx.TextLayer;
import org.mozilla.gecko.gfx.TileLayer;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.RegionIterator;
import android.opengl.GLSurfaceView;
import android.os.SystemClock;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import java.nio.IntBuffer;
import java.util.ArrayList;




public class LayerRenderer implements GLSurfaceView.Renderer {
    private static final String LOGTAG = "GeckoLayerRenderer";
    private static final String PROFTAG = "GeckoLayerRendererProf";

    



    private static final int MAX_FRAME_TIME = 16;   

    private static final int FRAME_RATE_METER_WIDTH = 64;
    private static final int FRAME_RATE_METER_HEIGHT = 32;

    private final LayerView mView;
    private final SingleTileLayer mBackgroundLayer;
    private final CheckerboardImage mCheckerboardImage;
    private final SingleTileLayer mCheckerboardLayer;
    private final NinePatchTileLayer mShadowLayer;
    private final TextLayer mFrameRateLayer;
    private final ScrollbarLayer mHorizScrollLayer;
    private final ScrollbarLayer mVertScrollLayer;
    private final FadeRunnable mFadeRunnable;
    private RenderContext mLastPageContext;
    private int mMaxTextureSize;

    private ArrayList<Layer> mExtraLayers = new ArrayList<Layer>();

    
    private int[] mFrameTimings;
    private int mCurrentFrame, mFrameTimingsSum, mDroppedFrames;
    private boolean mShowFrameRate;

    
    private int mFramesRendered;
    private float mCompleteFramesRendered;
    private boolean mProfileRender;
    private long mProfileOutputTime;

    
    private IntBuffer mPixelBuffer;

    public LayerRenderer(LayerView view) {
        mView = view;

        LayerController controller = view.getController();

        CairoImage backgroundImage = new BufferedCairoImage(controller.getBackgroundPattern());
        mBackgroundLayer = new SingleTileLayer(true, backgroundImage);

        mCheckerboardImage = new CheckerboardImage();
        mCheckerboardLayer = new SingleTileLayer(true, mCheckerboardImage);

        CairoImage shadowImage = new BufferedCairoImage(controller.getShadowPattern());
        mShadowLayer = new NinePatchTileLayer(shadowImage);

        IntSize frameRateLayerSize = new IntSize(FRAME_RATE_METER_WIDTH, FRAME_RATE_METER_HEIGHT);
        mFrameRateLayer = TextLayer.create(frameRateLayerSize, "-- ms/--");

        mHorizScrollLayer = ScrollbarLayer.create(false);
        mVertScrollLayer = ScrollbarLayer.create(true);
        mFadeRunnable = new FadeRunnable();

        mFrameTimings = new int[60];
        mCurrentFrame = mFrameTimingsSum = mDroppedFrames = 0;
        mShowFrameRate = false;
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        checkMonitoringEnabled();

        gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_FASTEST);
        gl.glDisable(GL10.GL_DITHER);
        gl.glEnable(GL10.GL_TEXTURE_2D);

        int maxTextureSizeResult[] = new int[1];
        gl.glGetIntegerv(GL10.GL_MAX_TEXTURE_SIZE, maxTextureSizeResult, 0);
        mMaxTextureSize = maxTextureSizeResult[0];

        TextureGenerator.get().fill();
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
        long frameStartTime = SystemClock.uptimeMillis();

        TextureReaper.get().reap(gl);
        TextureGenerator.get().fill();

        LayerController controller = mView.getController();
        RenderContext screenContext = createScreenContext();

        boolean updated = true;

        synchronized (controller) {
            Layer rootLayer = controller.getRoot();
            RenderContext pageContext = createPageContext();

            if (!pageContext.fuzzyEquals(mLastPageContext)) {
                
                
                mVertScrollLayer.unfade();
                mHorizScrollLayer.unfade();
                mFadeRunnable.scheduleStartFade(ScrollbarLayer.FADE_DELAY);
            } else if (mFadeRunnable.timeToFade()) {
                boolean stillFading = mVertScrollLayer.fade() | mHorizScrollLayer.fade();
                if (stillFading) {
                    mFadeRunnable.scheduleNextFadeFrame();
                }
            }
            mLastPageContext = pageContext;

            
            if (rootLayer != null) updated &= rootLayer.update(gl, pageContext);
            updated &= mBackgroundLayer.update(gl, screenContext);
            updated &= mShadowLayer.update(gl, pageContext);
            updateCheckerboardLayer(gl, screenContext);
            updated &= mFrameRateLayer.update(gl, screenContext);
            updated &= mVertScrollLayer.update(gl, pageContext);
            updated &= mHorizScrollLayer.update(gl, pageContext);

            for (Layer layer : mExtraLayers)
                updated &= layer.update(gl, pageContext);

            
            mBackgroundLayer.draw(screenContext);

            
            Rect pageRect = getPageRect();
            RectF untransformedPageRect = new RectF(0.0f, 0.0f, pageRect.width(),
                                                    pageRect.height());
            if (!untransformedPageRect.contains(controller.getViewport()))
                mShadowLayer.draw(pageContext);

            
            Rect scissorRect = transformToScissorRect(pageRect);
            gl.glEnable(GL10.GL_SCISSOR_TEST);
            gl.glScissor(scissorRect.left, scissorRect.top,
                         scissorRect.width(), scissorRect.height());

            mCheckerboardLayer.draw(screenContext);

            
            if (rootLayer != null)
                rootLayer.draw(pageContext);

            gl.glDisable(GL10.GL_SCISSOR_TEST);

            
            for (Layer layer : mExtraLayers)
                layer.draw(pageContext);

            
            IntSize screenSize = new IntSize(controller.getViewportSize());
            if (pageRect.height() > screenSize.height)
                mVertScrollLayer.draw(pageContext);

            
            if (pageRect.width() > screenSize.width)
                mHorizScrollLayer.draw(pageContext);

            
            if ((rootLayer != null) &&
                (mProfileRender || PanningPerfAPI.isRecordingCheckerboard())) {
                
                Rect viewport = RectUtils.round(pageContext.viewport);
                Region validRegion = rootLayer.getValidRegion(pageContext);
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

                if (frameStartTime - mProfileOutputTime > 1000) {
                    mProfileOutputTime = frameStartTime;
                    printCheckerboardStats();
                }
            }
        }

        
        if (mShowFrameRate) {
            updateDroppedFrames(frameStartTime);

            try {
                gl.glEnable(GL10.GL_BLEND);
                gl.glBlendFunc(GL10.GL_SRC_ALPHA, GL10.GL_ONE_MINUS_SRC_ALPHA);
                mFrameRateLayer.draw(screenContext);
            } finally {
                gl.glDisable(GL10.GL_BLEND);
            }
        }

        
        if (!updated)
            mView.requestRender();

        PanningPerfAPI.recordFrameTime();

        
        IntBuffer pixelBuffer = mPixelBuffer;
        if (updated && pixelBuffer != null) {
            synchronized (pixelBuffer) {
                pixelBuffer.position(0);
                gl.glReadPixels(0, 0, (int)screenContext.viewport.width(), (int)screenContext.viewport.height(), GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, pixelBuffer);
                pixelBuffer.notify();
            }
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

    private RenderContext createScreenContext() {
        LayerController layerController = mView.getController();
        IntSize viewportSize = new IntSize(layerController.getViewportSize());
        RectF viewport = new RectF(0.0f, 0.0f, viewportSize.width, viewportSize.height);
        FloatSize pageSize = new FloatSize(layerController.getPageSize());
        return new RenderContext(viewport, pageSize, 1.0f);
    }

    private RenderContext createPageContext() {
        LayerController layerController = mView.getController();

        Rect viewport = new Rect();
        layerController.getViewport().round(viewport);

        FloatSize pageSize = new FloatSize(layerController.getPageSize());
        float zoomFactor = layerController.getZoomFactor();
        return new RenderContext(new RectF(viewport), pageSize, zoomFactor);
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
        gl.glViewport(0, 0, width, height);

        
        
        mView.post(new Runnable() {
            public void run() {
                mView.setViewportSize(new IntSize(width, height));
                moveFrameRateLayer(width, height);
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
            Point origin = new Point(width - FRAME_RATE_METER_WIDTH - 8,
                                     height - FRAME_RATE_METER_HEIGHT + 8);
            mFrameRateLayer.setOrigin(origin);
        } finally {
            mFrameRateLayer.endTransaction();
        }
    }

    private void checkMonitoringEnabled() {
        
        new Thread(new Runnable() {
            @Override
            public void run() {
                Context context = mView.getContext();
                SharedPreferences preferences = context.getSharedPreferences("GeckoApp", 0);
                mShowFrameRate = preferences.getBoolean("showFrameRate", false);
                mProfileRender = Log.isLoggable(PROFTAG, Log.DEBUG);
            }
        }).start();
    }

    private void updateCheckerboardLayer(GL10 gl, RenderContext renderContext) {
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

        mCheckerboardLayer.update(gl, renderContext);
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
}
