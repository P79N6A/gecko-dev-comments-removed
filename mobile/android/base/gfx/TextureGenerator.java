




































package org.mozilla.gecko.gfx;

import android.opengl.GLES10;
import java.util.Stack;

public class TextureGenerator {
    private static final int MIN_TEXTURES = 5;

    private static TextureGenerator sSharedInstance;
    private Stack<Integer> mTextureIds;

    private TextureGenerator() { mTextureIds = new Stack<Integer>(); }

    public static TextureGenerator get() {
        if (sSharedInstance == null)
            sSharedInstance = new TextureGenerator();
        return sSharedInstance;
    }

    public synchronized int take() {
        if (mTextureIds.empty())
            return 0;

        return (int)mTextureIds.pop();
    }

    public synchronized void fill() {
        int[] textures = new int[1];
        while (mTextureIds.size() < MIN_TEXTURES) {
            GLES10.glGenTextures(1, textures, 0);
            mTextureIds.push(textures[0]);
        }
    }
}


