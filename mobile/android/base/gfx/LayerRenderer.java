




































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
        Rect pageRect = getPageRect();

        
        gl.glClearColor(BACKGROUND_COLOR_R, BACKGROUND_COLOR_G, BACKGROUND_COLOR_B, 1.0f);
        gl.glClear(GL10.GL_COLOR_BUFFER_BIT);

        
        setupPageTransform(gl);
        mShadowLayer.draw(gl);

        
        Rect clampedPageRect = clampToScreen(pageRect);
        IntSize screenSize = controller.getScreenSize();
        gl.glEnable(GL10.GL_SCISSOR_TEST);
        gl.glScissor(clampedPageRect.left, screenSize.height - clampedPageRect.bottom,
                     clampedPageRect.width(), clampedPageRect.height());

        gl.glLoadIdentity();
        mCheckerboardLayer.draw(gl);

        
        setupPageTransform(gl);

        Layer rootLayer = controller.getRoot();
        if (rootLayer != null)
            rootLayer.draw(gl);

        gl.glDisable(GL10.GL_SCISSOR_TEST);

        gl.glEnable(GL10.GL_BLEND);

        
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

    public void pageSizeChanged() {
        mShadowLayer.recreateVertexBuffers();
    }

    private void setupPageTransform(GL10 gl) {
        LayerController controller = mView.getController();
        RectF visibleRect = controller.getVisibleRect();
        float zoomFactor = controller.getZoomFactor();

        gl.glLoadIdentity();
        gl.glScalef(zoomFactor, zoomFactor, 1.0f);
        gl.glTranslatef(-visibleRect.left, -visibleRect.top, 0.0f);
    }

    private Rect getPageRect() {
        LayerController controller = mView.getController();
        float zoomFactor = controller.getZoomFactor();
        RectF visibleRect = controller.getVisibleRect();
        IntSize pageSize = controller.getPageSize();

        int x = (int)Math.round(-zoomFactor * visibleRect.left);
        int y = (int)Math.round(-zoomFactor * visibleRect.top);
        return new Rect(x, y,
                        x + (int)Math.round(zoomFactor * pageSize.width),
                        y + (int)Math.round(zoomFactor * pageSize.height));
    }

    private Rect clampToScreen(Rect rect) {
        LayerController controller = mView.getController();
        IntSize screenSize = controller.getScreenSize();

        int left = Math.max(0, rect.left);
        int top = Math.max(0, rect.top);
        int right = Math.min(screenSize.width, rect.right);
        int bottom = Math.min(screenSize.height, rect.bottom);
        return new Rect(left, top, right, bottom);
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
                mView.setScreenSize(width, height);
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

