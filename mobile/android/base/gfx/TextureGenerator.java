




































package org.mozilla.gecko.gfx;

import android.util.Log;
import android.opengl.GLES20;
import java.util.concurrent.ArrayBlockingQueue;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLContext;

public class TextureGenerator {
    private static final String LOGTAG = "TextureGenerator";
    private static final int POOL_SIZE = 5;

    private static TextureGenerator sSharedInstance;

    private ArrayBlockingQueue<Integer> mTextureIds;
    private EGLContext mContext;

    private TextureGenerator() { mTextureIds = new ArrayBlockingQueue<Integer>(POOL_SIZE); }

    public static TextureGenerator get() {
        if (sSharedInstance == null)
            sSharedInstance = new TextureGenerator();
        return sSharedInstance;
    }

    public synchronized int take() {
        try {
            
            return (int)mTextureIds.take();
        } catch (InterruptedException e) {
            return 0;
        }
    }

    private void evictTextures() {
        int[] textures = new int[1];
        Integer texture;
        while ((texture = mTextureIds.poll()) != null) {
            textures[0] = texture;
            GLES20.glDeleteTextures(1, textures, 0);
        }
    }

    public synchronized void fill() {
        EGL10 egl = (EGL10)EGLContext.getEGL();
        EGLContext context = egl.eglGetCurrentContext();

        if (mContext != null && mContext != context) {
            evictTextures();
        }

        mContext = context;

        int numNeeded = mTextureIds.remainingCapacity();
        if (numNeeded == 0)
            return;

        
        int error;
        while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
            Log.w(LOGTAG, String.format("Clearing GL error: %#x", error));
        }

        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);

        int[] textures = new int[numNeeded];
        GLES20.glGenTextures(numNeeded, textures, 0);

        error = GLES20.glGetError();
        if (error != GLES20.GL_NO_ERROR) {
            Log.e(LOGTAG, String.format("Failed to generate textures: %#x", error), new Exception());
            return;
        }
        
        for (int i = 0; i < numNeeded; i++) {
            mTextureIds.offer(textures[i]);
        }
    }
}


