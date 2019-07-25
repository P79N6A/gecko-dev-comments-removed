




































package org.mozilla.gecko.gfx.layers;

import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.util.Log;
import android.view.View;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.gfx.*;
import org.mozilla.gecko.GeckoInputConnection;

public class OGLSurfaceView implements AbstractLayerView {
    private static final String LOGTAG = "OGLSurfaceView";

    
    private static LayerController mLayerController;

    private Context mContext;
    private LayerController mController;
    private InputConnectionHandler mInputConnectionHandler;

    public OGLSurfaceView(Context context, LayerController controller) {
        
        mContext = context;
        mController = controller;
        System.out.println("construct");
    }

    public SurfaceHolder getHolder() {
        System.out.println("Get Holder");
        return null;
    }

    
    public static void registerLayerController(LayerController layerController) {
        System.out.println("register layer controller");
        synchronized (OGLSurfaceView.class) {
            mLayerController = layerController;
            OGLSurfaceView.class.notifyAll();
        }
    }

    
    public static void registerCompositor() {
        System.out.println("register layer comp");
        synchronized (OGLSurfaceView.class) {
            
            
            while (mLayerController == null) {
                try {
                    OGLSurfaceView.class.wait();
                } catch (InterruptedException e) {}
            }
            final LayerController controller = mLayerController;

            GeckoApp.mAppContext.runOnUiThread(new Runnable() {
                public void run() {
                    synchronized (OGLSurfaceView.class) {
                        OGLSurfaceView surfaceView =
                            new OGLSurfaceView(controller.getContext(), controller);
                        OGLSurfaceView.class.notifyAll();
                    }
                }
            });

            
            
            try {
                OGLSurfaceView.class.wait();
            } catch (InterruptedException e) {}
        }

    }

    public static native void setSurfaceView(SurfaceView sv);

    public LayerController getController() {
        return mController; 
    }

    public GeckoInputConnection setInputConnectionHandler() {
        GeckoInputConnection geckoInputConnection = GeckoInputConnection.create(getAndroidView());
        mInputConnectionHandler = geckoInputConnection;
        return geckoInputConnection;
    }

    public View getAndroidView() {
        return null;
    }

    
    public void setViewportSize(IntSize size) {}
    public void requestRender() {}
    public boolean post(Runnable action) { return false; }
    public boolean postDelayed(Runnable action, long delayMillis) { return false; }
    public Context getContext() { return mContext; }
    public int getMaxTextureSize() { return 1024; }
    public void clearEventQueue() {}
    public void processEventQueue() {}

    private class InternalSurfaceView extends SurfaceView {
        public InternalSurfaceView() {
            super(OGLSurfaceView.this.mContext);
        }
    }
}
