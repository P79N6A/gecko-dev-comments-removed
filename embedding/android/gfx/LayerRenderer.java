




































package org.mozilla.fennec.gfx;

import org.mozilla.fennec.gfx.BufferedCairoImage;
import org.mozilla.fennec.gfx.IntRect;
import org.mozilla.fennec.gfx.IntSize;
import org.mozilla.fennec.gfx.LayerController;
import org.mozilla.fennec.gfx.LayerView;
import org.mozilla.fennec.gfx.NinePatchTileLayer;
import org.mozilla.fennec.gfx.SingleTileLayer;
import org.mozilla.fennec.gfx.TextureReaper;
import org.mozilla.fennec.gfx.TextLayer;
import org.mozilla.fennec.gfx.TileLayer;
import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import java.nio.ByteBuffer;




public class LayerRenderer implements GLSurfaceView.Renderer {
    private LayerView mView;
    private SingleTileLayer mBackgroundLayer;
    private SingleTileLayer mCheckerboardLayer;
    private NinePatchTileLayer mShadowLayer;
    private TextLayer mFPSLayer;

    
    private long mFrameCountTimestamp;
    private int mFrameCount;            

    public LayerRenderer(LayerView view) {
        mView = view;

        
        LayerController controller = view.getController();
        mBackgroundLayer = new SingleTileLayer(true);
        mBackgroundLayer.paintImage(new BufferedCairoImage(controller.getBackgroundPattern()));
        mCheckerboardLayer = new SingleTileLayer(true);
        mCheckerboardLayer.paintImage(new BufferedCairoImage(controller.getCheckerboardPattern()));
        mShadowLayer = new NinePatchTileLayer(controller);
        mShadowLayer.paintImage(new BufferedCairoImage(controller.getShadowPattern()));
        mFPSLayer = new TextLayer(new IntSize(64, 32));
        mFPSLayer.setText("-- FPS");

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

        

        
        gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);

        
        gl.glLoadIdentity();
        mBackgroundLayer.draw(gl);

        
        setupPageTransform(gl);
        mShadowLayer.draw(gl);

        
        IntRect pageRect = clampToScreen(getPageRect());
        IntSize screenSize = controller.getScreenSize();
        gl.glEnable(GL10.GL_SCISSOR_TEST);
        gl.glScissor(pageRect.x, screenSize.height - (pageRect.y + pageRect.height),
                     pageRect.width, pageRect.height);

        gl.glLoadIdentity();
        mCheckerboardLayer.draw(gl);

        
        setupPageTransform(gl);

        Layer rootLayer = controller.getRoot();
        if (rootLayer != null)
            rootLayer.draw(gl);

        gl.glDisable(GL10.GL_SCISSOR_TEST);

        
        gl.glLoadIdentity();
        gl.glEnable(GL10.GL_BLEND);
        mFPSLayer.draw(gl);
        gl.glDisable(GL10.GL_BLEND);
    }

    public void pageSizeChanged() {
        mShadowLayer.recreateVertexBuffers();
    }

    private void setupPageTransform(GL10 gl) {
        LayerController controller = mView.getController();
        IntRect visibleRect = controller.getVisibleRect();
        float zoomFactor = controller.getZoomFactor();

        gl.glLoadIdentity();
        gl.glScalef(zoomFactor, zoomFactor, 1.0f);
        gl.glTranslatef(-visibleRect.x, -visibleRect.y, 0.0f);
    }

    private IntRect getPageRect() {
        LayerController controller = mView.getController();
        float zoomFactor = controller.getZoomFactor();
        IntRect visibleRect = controller.getVisibleRect();
        IntSize pageSize = controller.getPageSize(); 

        return new IntRect((int)Math.round(-zoomFactor * visibleRect.x),
                           (int)Math.round(-zoomFactor * visibleRect.y),
                           (int)Math.round(zoomFactor * pageSize.width),
                           (int)Math.round(zoomFactor * pageSize.height));
    }

    private IntRect clampToScreen(IntRect rect) {
        LayerController controller = mView.getController();
        IntSize screenSize = controller.getScreenSize();

        int left = Math.max(0, rect.x);
        int top = Math.max(0, rect.y);
        int right = Math.min(screenSize.width, rect.getRight());
        int bottom = Math.min(screenSize.height, rect.getBottom());
        return new IntRect(left, top, right - left, bottom - top);
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        gl.glViewport(0, 0, width, height);
        gl.glMatrixMode(GL10.GL_PROJECTION);
        gl.glLoadIdentity();
        gl.glOrthof(0.0f, (float)width, (float)height, 0.0f, -10.0f, 10.0f);
        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glLoadIdentity();

        mView.setScreenSize(width, height);

        
    }

    private void checkFPS() {
        if (System.currentTimeMillis() >= mFrameCountTimestamp + 1000) {
            mFrameCountTimestamp = System.currentTimeMillis();
            mFPSLayer.setText(mFrameCount + " FPS");
            mFrameCount = 0;
        } else {
            mFrameCount++;
        }
    }
}

