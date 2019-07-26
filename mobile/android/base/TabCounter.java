




package org.mozilla.gecko;

import org.mozilla.gecko.animation.Rotate3DAnimation;

import android.content.Context;
import android.os.Build;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.AlphaAnimation;
import android.view.LayoutInflater;
import android.view.View;
import android.util.AttributeSet;
import android.widget.ViewSwitcher;

public class TabCounter extends GeckoTextSwitcher
                        implements ViewSwitcher.ViewFactory {

    private static final float CENTER_X = 0.5f;
    private static final float CENTER_Y = 1.25f;
    private static final int DURATION = 500;
    private static final float Z_DISTANCE = 200;

    private final AnimationSet mFlipInForward;
    private final AnimationSet mFlipInBackward;
    private final AnimationSet mFlipOutForward;
    private final AnimationSet mFlipOutBackward;
    private final LayoutInflater mInflater;

    private int mCount = 0;

    private enum FadeMode {
        FADE_IN,
        FADE_OUT
    }

    public TabCounter(Context context, AttributeSet attrs) {
        super(context, attrs);
        mInflater = LayoutInflater.from(context);

        mFlipInForward = createAnimation(-90, 0, FadeMode.FADE_IN, -1 * Z_DISTANCE, false);
        mFlipInBackward = createAnimation(90, 0, FadeMode.FADE_IN, Z_DISTANCE, false);
        mFlipOutForward = createAnimation(0, -90, FadeMode.FADE_OUT, -1 * Z_DISTANCE, true);
        mFlipOutBackward = createAnimation(0, 90, FadeMode.FADE_OUT, Z_DISTANCE, true);

        removeAllViews();
        setFactory(this);

        if (Build.VERSION.SDK_INT >= 16) {
            
            
            
            
            
            setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
            setAccessibilityDelegate(new View.AccessibilityDelegate() {
                    @Override
                    public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {}
                });
        }
    }

    public void setCountWithAnimation(int count) {
        
        if (mCount == 0) {
            setCount(count);
            return;
        }

        if (mCount == count) {
            return;
        }

        if (count < mCount) {
            setInAnimation(mFlipInBackward);
            setOutAnimation(mFlipOutForward);
        } else {
            setInAnimation(mFlipInForward);
            setOutAnimation(mFlipOutBackward);
        }

        
        
        setDisplayedChild(0);

        
        setCurrentText(String.valueOf(mCount));
        setText(String.valueOf(count));

        mCount = count;
    }

    public void setCount(int count) {
        setCurrentText(String.valueOf(count));
        mCount = count;
    }

    private AnimationSet createAnimation(float startAngle, float endAngle,
                                         FadeMode fadeMode,
                                         float zEnd, boolean reverse) {
        final Context context = getContext();
        AnimationSet set = new AnimationSet(context, null);
        set.addAnimation(new Rotate3DAnimation(startAngle, endAngle, CENTER_X, CENTER_Y, zEnd, reverse));
        set.addAnimation(fadeMode == FadeMode.FADE_IN ? new AlphaAnimation(0.0f, 1.0f) :
                                                        new AlphaAnimation(1.0f, 0.0f));
        set.setDuration(DURATION);
        set.setInterpolator(context, android.R.anim.accelerate_interpolator);
        return set;
    }

    @Override
    public View makeView() {
        return mInflater.inflate(R.layout.tabs_counter, null);
    }

}
