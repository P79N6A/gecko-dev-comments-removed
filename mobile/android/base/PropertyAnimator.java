




package org.mozilla.gecko;

import java.util.List;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import android.view.animation.Interpolator;
import android.view.animation.DecelerateInterpolator;
import android.view.View;
import android.view.ViewGroup;
import android.util.Log;

public class PropertyAnimator extends TimerTask {
    private static final String LOGTAG = "GeckoPropertyAnimator";

    private Timer mTimer;
    private Interpolator mInterpolator;

    public static enum Property {
        SHRINK_LEFT,
        SHRINK_TOP,
        SLIDE_TOP,
        SLIDE_LEFT
    }

    private class ElementHolder {
        View view;
        Property property;
        int from;
        int to;
    }

    public static interface PropertyAnimationListener {
        public void onPropertyAnimationStart();
        public void onPropertyAnimationEnd();
    }

    private int mCount;
    private int mDuration;
    private List<ElementHolder> mElementsList;
    private PropertyAnimationListener mListener;

    
    private static final int INTERVAL = 10;

    public PropertyAnimator(int duration) {
        mDuration = duration;
        mTimer = new Timer();
        mInterpolator = new DecelerateInterpolator();
        mElementsList = new ArrayList<ElementHolder>();
    }

    public void attach(View view, Property property, int to) {
        if (!(view instanceof ViewGroup) && (property == Property.SHRINK_LEFT ||
                                             property == Property.SHRINK_TOP)) {
            Log.i(LOGTAG, "Margin can only be animated on Viewgroups");
            return;
        }

        ElementHolder element = new ElementHolder();

        element.view = view;
        element.property = property;

        
        if (property == Property.SLIDE_TOP || property == Property.SLIDE_LEFT)
            element.to = to * -1;
        else
            element.to = to;

        mElementsList.add(element);
    }

    public void setPropertyAnimationListener(PropertyAnimationListener listener) {
        mListener = listener;
    }

    @Override
    public void run() {
        float interpolation = mInterpolator.getInterpolation((float) (mCount * INTERVAL) / (float) mDuration);

        for (ElementHolder element : mElementsList) { 
            int delta = element.from + (int) ((element.to - element.from) * interpolation);
            invalidate(element, delta);
        }

        mCount++;

        if (mCount * INTERVAL >= mDuration)
            stop();
    }

    public void start() {
        mCount = 0;

        
        for (ElementHolder element : mElementsList) {
            if (element.property == Property.SLIDE_TOP)
                element.from = element.view.getScrollY();
            else if (element.property == Property.SLIDE_LEFT)
                element.from = element.view.getScrollX();
            else {
                ViewGroup.MarginLayoutParams params = ((ViewGroup.MarginLayoutParams) element.view.getLayoutParams());
                if (element.property == Property.SHRINK_TOP)
                    element.from = params.topMargin;
                else if (element.property == Property.SHRINK_LEFT)
                    element.from = params.leftMargin;
            }
        }

        if (mDuration != 0) {
            mTimer.scheduleAtFixedRate(this, 0, INTERVAL);

            if (mListener != null)
                mListener.onPropertyAnimationStart();
        }
    }

    public void stop() {
        cancel();
        mTimer.cancel();
        mTimer.purge();

        
        for (ElementHolder element : mElementsList) { 
            invalidate(element, element.to);
        }

        mElementsList.clear();

        if (mListener != null) {
            mListener.onPropertyAnimationEnd();
            mListener = null;
        }
    }

    private void invalidate(final ElementHolder element, final int delta) {
        
        element.view.getHandler().post(new Runnable() {
            @Override
            public void run() {
                if (element.property == Property.SLIDE_TOP) {
                    element.view.scrollTo(element.view.getScrollX(), delta);
                    return;
                } else if (element.property == Property.SLIDE_LEFT) {
                    element.view.scrollTo(delta, element.view.getScrollY());
                    return;
                }

                ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) element.view.getLayoutParams();

                if (element.property == Property.SHRINK_TOP)
                    params.setMargins(params.leftMargin, delta, params.rightMargin, params.bottomMargin);
                else if (element.property == Property.SHRINK_LEFT)
                    params.setMargins(delta, params.topMargin, params.rightMargin, params.bottomMargin);

                element.view.requestLayout();
            }
        });
    }
}
