




package org.mozilla.gecko;

import java.util.Timer;
import java.util.TimerTask;

import android.os.Handler;
import android.view.animation.Interpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.View;
import android.widget.RelativeLayout;

public class PropertyAnimator extends TimerTask {
    private static final String LOGTAG = "GeckoPropertyAnimator";

    private Timer mTimer;
    private TimerTask mShowTask;
    private Interpolator mInterpolator;

    public static enum Property {
        MARGIN_LEFT,
        MARGIN_RIGHT,
        MARGIN_TOP,
        MARGIN_BOTTOM
    }

    private View mView;
    private Property mProperty;
    private int mDuration;
    private int mFrom;
    private int mTo;

    private int mCount;

    
    private static final int sInterval = 10;

    public PropertyAnimator(View view, Property property, int from, int to, int duration) {
        mView = view;
        mProperty = property;
        mDuration = duration;
        mFrom = from;
        mTo = to;

        mTimer = new Timer();
        mInterpolator = new DecelerateInterpolator();
    }

    @Override
    public void run() {
        float interpolation = mInterpolator.getInterpolation((float) (mCount * sInterval) / (float) mDuration);
        int delta;
        if (mFrom < mTo)
            delta = mFrom + (int) ((mTo - mFrom) * interpolation);
        else
            delta = mFrom - (int) ((mFrom - mTo) * interpolation);

        invalidate(delta);

        mCount++;

        if (mCount * sInterval >= mDuration)
            stop();
    }

    public void start() {
        mCount = 0;
        mTimer.scheduleAtFixedRate(this, 0, sInterval);
    }

    public void stop() {
        cancel();
        mTimer.cancel();
        mTimer.purge();

        
        invalidate(mTo);
    }

    private void invalidate(final int delta) {
        
        mView.getHandler().post(new Runnable() {
            @Override
            public void run() {
                RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(mView.getLayoutParams());
                switch(mProperty) {
                    case MARGIN_TOP:
                        params.setMargins(0, delta, 0, 0);
                        break;
                 
                    case MARGIN_BOTTOM:
                        params.setMargins(0, 0, 0, delta);
                        break;
                 
                    case MARGIN_LEFT:
                        params.setMargins(delta, 0, 0, 0);
                        break;
                 
                    case MARGIN_RIGHT:
                        params.setMargins(0, 0, delta, 0);
                        break;
                }
                    
                mView.setLayoutParams(params);
                mView.requestLayout();
            }
        });
    }
}
