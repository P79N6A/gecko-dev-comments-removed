









package org.webrtc.videoengine;

import android.content.Context;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

import org.mozilla.gecko.GeckoApp;
import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.util.ThreadUtils;

public class ViERenderer {
    private final static String TAG = "WEBRTC-ViEREnderer";

    
    private static SurfaceHolder g_localRenderer = null;

    public static SurfaceView CreateRenderer(Context context) {
        return CreateRenderer(context, false);
    }

    public static SurfaceView CreateRenderer(Context context,
            boolean useOpenGLES2) {
        if(useOpenGLES2 == true && ViEAndroidGLES20.IsSupported(context))
            return new ViEAndroidGLES20(context);
        else
            return new SurfaceView(context);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    public static void CreateLocalRenderer() {
        View cameraView = GeckoAppShell.getGeckoInterface().getCameraView();
        if (cameraView != null && (cameraView instanceof SurfaceView)) {
            SurfaceView localRender = (SurfaceView)cameraView;
            g_localRenderer = localRender.getHolder();
        }

        ThreadUtils.getUiHandler().post(new Runnable() {
            @Override
            public void run() {
                try {
                    GeckoAppShell.getGeckoInterface().enableCameraView();
                } catch (Exception e) {
                    Log.e(TAG, "CreateLocalRenderer enableCameraView exception: "
                          + e.getLocalizedMessage());
                }
            }
        });
    }

    public static void DestroyLocalRenderer() {
        if (g_localRenderer != null) {
            g_localRenderer = null;

            ThreadUtils.getUiHandler().post(new Runnable() {
                @Override
                public void run() {
                    try {
                        GeckoAppShell.getGeckoInterface().disableCameraView();
                    } catch (Exception e) {
                        Log.e(TAG,
                              "DestroyLocalRenderer disableCameraView exception: " +
                              e.getLocalizedMessage());
                    }
                }
            });
        }
    }

    public static SurfaceHolder GetLocalRenderer() {
        return g_localRenderer;
    }

}
