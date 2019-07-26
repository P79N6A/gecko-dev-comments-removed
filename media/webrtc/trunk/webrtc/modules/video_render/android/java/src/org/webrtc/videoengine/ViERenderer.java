









package org.webrtc.videoengine;

import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ViERenderer {

    
    private static SurfaceHolder g_localRenderer;

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

    
    
    
    
    
    
    
    
    
    
    
    
    
    public static SurfaceView CreateLocalRenderer(Context context) {
        SurfaceView localRender = new SurfaceView(context);
        g_localRenderer = localRender.getHolder();
        return localRender;
    }

    public static SurfaceHolder GetLocalRenderer() {
        return g_localRenderer;
    }

}
