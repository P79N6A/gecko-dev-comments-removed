





































package org.mozilla.gecko.gfx;

import org.mozilla.gecko.gfx.BufferedCairoImage;
import org.mozilla.gecko.gfx.IntSize;
import org.mozilla.gecko.gfx.LayerController;
import org.mozilla.gecko.gfx.LayerView;
import org.mozilla.gecko.gfx.NinePatchTileLayer;
import org.mozilla.gecko.gfx.SingleTileLayer;
import org.mozilla.gecko.gfx.TextureReaper;
import org.mozilla.gecko.gfx.TextLayer;
import org.mozilla.gecko.gfx.TileLayer;
import android.content.Context;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.opengl.GLSurfaceView;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import java.nio.ByteBuffer;




public class LayerRenderer implements GLSurfaceView.Renderer {
    private static final float BACKGROUND_COLOR_R = 0.81f;
    private static final float BACKGROUND_COLOR_G = 0.81f;
    private static final float BACKGROUND_COLOR_B = 0.81f;

    private LayerView mView;
    private SingleTileLayer mCheckerboardLayer;
    private NinePatchTileLayer mShadowLayer;
    private TextLayer mFPSLayer;
    private ScrollbarLayer mHorizScrollLayer;
    private ScrollbarLayer mVertScrollLayer;

    
    private long mFrameCountTimestamp;
    private int mFrameCount;            

    public LayerRenderer(LayerView view) {
        mView = view;

        
        LayerController controller = view.getController();
        CairoImage checkerboardImage = new BufferedCairoImage(controller.getCheckerboardPattern());
        mCheckerboardLayer = new SingleTileLayer(true, checkerboardImage);
        CairoImage shadowImage = new BufferedCairoImage(controller.getShadowPattern());
        mShadowLayer = new NinePatchTileLayer(controller, shadowImage);
        mFPSLayer = TextLayer.create(new IntSize(64, 32), "-- FPS");
        mHorizScrollLayer = ScrollbarLayer.create();
        mVertScrollLayer = ScrollbarLayer.create();

        mFrameCountTimestamp = System.currentTimeMillis();
        mFrameCount = 0;
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        gl.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        gl.glClearDepthf(1.0f);             
        gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT, GL10.GL_FASTEST);
        gl.glShadeModel(GL10.GL_SMOOTH);    
        gl.glDisable(GL10.GL_DITHER);
        gl.glEnable(GL10.GL_TEXTURE_2D);
    }

    public void onDrawFrame(GL10 gl) {
        checkFPS();
        TextureReaper.get().reap(gl);

        LayerController controller = mView.getController();
        Layer rootLayer = controller.getRoot();

        
        if (rootLayer != null) rootLayer.update(gl);
        mShadowLayer.update(gl);
        mCheckerboardLayer.update(gl);
        mFPSLayer.update(gl);
        mVertScrollLayer.update(gl);
        mHorizScrollLayer.update(gl);

        
        gl.glClearColor(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 1.0f);
        gl.glClear(GL10.GL_COLOR_BUFFER_BIT);

        
        setupPageTransform(gl, false);
        mShadowLayer.draw(gl);

        
        Rect pageRect = getPageRect();
        Rect scissorRect = transformToScissorRect(pageRect);
        gl.glEnable(GL10.GL_SCISSOR_TEST);
        gl.glScissor(scissorRect.left, scissorRect.top,
                     scissorRect.width(), scissorRect.height());

        gl.glLoadIdentity();
        mCheckerboardLayer.draw(gl);

        
        if (rootLayer != null) {
            gl.glLoadIdentity();
            setupPageTransform(gl, true);
            rootLayer.transform(gl);
            rootLayer.draw(gl);
        }

        gl.glDisable(GL10.GL_SCISSOR_TEST);

        gl.glEnable(GL10.GL_BLEND);

        
        IntSize screenSize = new IntSize(controller.getViewportSize());
        if (pageRect.height() > screenSize.height) {
            mVertScrollLayer.drawVertical(gl, screenSize, pageRect);
        }

        
        if (pageRect.width() > screenSize.width) {
            mHorizScrollLayer.drawHorizontal(gl, screenSize, pageRect);
        }

        
        gl.glLoadIdentity();
        mFPSLayer.draw(gl);

        gl.glDisable(GL10.GL_BLEND);
    }

    private void setupPageTransform(GL10 gl, boolean scale) {
        LayerController controller = mView.getController();

        PointF origin = controller.getOrigin();
        gl.glTranslatef(Math.round(-origin.x), Math.round(-origin.y), 0.0f);

        if (scale) {
            float zoomFactor = controller.getZoomFactor();
            gl.glScalef(zoomFactor, zoomFactor, 1.0f);
        }
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
        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glOrthof(0.0f, (float)width, (float)height, 0.0f, -10.0f, 10.0f);
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glLoadIdentity();

        
        
        mView.post(new Runnable() {
            public void run() {
                mView.setViewportSize(new IntSize(width, height));
            }
        });

        
    }

    private void checkFPS() {
        if (System.currentTimeMillis() >= mFrameCountTimestamp + 1000) {
            mFrameCountTimestamp = System.currentTimeMillis();

            mFPSLayer.beginTransaction();
            try {
                mFPSLayer.setText(mFrameCount + " FPS");
            } finally {
                mFPSLayer.endTransaction();
            }

            mFrameCount = 0;
        } else {
            mFrameCount++;
        }
    }
}

