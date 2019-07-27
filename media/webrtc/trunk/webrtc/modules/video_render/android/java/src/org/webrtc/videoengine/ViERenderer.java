









package org.webrtc.videoengine;

import android.content.Context;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class ViERenderer {
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
}
