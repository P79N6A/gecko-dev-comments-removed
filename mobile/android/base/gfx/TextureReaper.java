




package org.mozilla.gecko.gfx;

import android.opengl.GLES20;

import java.util.ArrayList;


public class TextureReaper {
    private static TextureReaper sSharedInstance;
    private ArrayList<Integer> mDeadTextureIDs;

    private TextureReaper() { mDeadTextureIDs = new ArrayList<Integer>(); }

    public static TextureReaper get() {
        if (sSharedInstance == null)
            sSharedInstance = new TextureReaper();
        return sSharedInstance;
    }

    public void add(int[] textureIDs) {
        for (int textureID : textureIDs)
            add(textureID);
    }

    public void add(int textureID) {
        mDeadTextureIDs.add(textureID);
    }

    public void reap() {
        int numTextures = mDeadTextureIDs.size();
        
        
        if (numTextures == 0)
            return;

        int[] deadTextureIDs = new int[numTextures];
        for (int i = 0; i < numTextures; i++) {
            deadTextureIDs[i] = mDeadTextureIDs.get(i);
        }
        mDeadTextureIDs.clear();

        GLES20.glDeleteTextures(deadTextureIDs.length, deadTextureIDs, 0);
    }
}


