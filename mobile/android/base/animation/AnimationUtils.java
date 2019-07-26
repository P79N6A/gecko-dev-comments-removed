





package org.mozilla.gecko.animation;

import android.content.Context;

public class AnimationUtils {
    private static long mShortDuration = -1;

    public static long getShortDuration(Context context) {
        if (mShortDuration < 0) {
            mShortDuration = context.getResources().getInteger(android.R.integer.config_shortAnimTime);
        }
        return mShortDuration;
    }
}

