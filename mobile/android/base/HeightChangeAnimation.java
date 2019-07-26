



package org.mozilla.gecko;

import android.view.View;
import android.view.animation.Animation;
import android.view.animation.Transformation;

public class HeightChangeAnimation extends Animation {
    int mFromHeight;
    int mToHeight;
    View mView;

    public HeightChangeAnimation(View view, int fromHeight, int toHeight) {
        mView = view;
        mFromHeight = fromHeight;
        mToHeight = toHeight;
    }

    @Override
    protected void applyTransformation(float interpolatedTime, Transformation t) {
        mView.getLayoutParams().height = Math.round((mFromHeight * (1 - interpolatedTime)) + (mToHeight * interpolatedTime));
        mView.requestLayout();
    }
}
