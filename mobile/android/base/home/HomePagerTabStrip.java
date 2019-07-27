




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.BounceAnimator;
import org.mozilla.gecko.animation.BounceAnimator.Attributes;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.v4.view.PagerTabStrip;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewTreeObserver;

import com.nineoldandroids.animation.AnimatorSet;
import com.nineoldandroids.animation.ObjectAnimator;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.ViewHelper;






class HomePagerTabStrip extends PagerTabStrip {

    private static final String LOGTAG = "PagerTabStrip";
    private static final int ANIMATION_DELAY_MS = 250;
    private static final int ALPHA_MS = 10;
    private static final int BOUNCE1_MS = 350;
    private static final int BOUNCE2_MS = 200;
    private static final int BOUNCE3_MS = 100;
    private static final int BOUNCE4_MS = 100;
    private static final int INIT_OFFSET = 100;

    public HomePagerTabStrip(Context context) {
        super(context);
    }

    public HomePagerTabStrip(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.HomePagerTabStrip);
        int color = a.getColor(R.styleable.HomePagerTabStrip_tabIndicatorColor, 0x00);
        a.recycle();

        setTabIndicatorColor(color);

        getViewTreeObserver().addOnPreDrawListener(new PreDrawListener());
    }

    @Override
    public int getPaddingBottom() {
        
        
        
        return 0;
    }

    private void animateTitles() {
        final View prevTextView = getChildAt(0);
        final View nextTextView = getChildAt(getChildCount() - 1);

        if (prevTextView == null || nextTextView == null) {
            return;
        }

        
        ViewHelper.setTranslationX(prevTextView, -INIT_OFFSET);
        ViewHelper.setAlpha(prevTextView, 0);
        ViewHelper.setTranslationX(nextTextView, INIT_OFFSET);
        ViewHelper.setAlpha(nextTextView, 0);

        
        final ValueAnimator alpha1 = ObjectAnimator.ofFloat(prevTextView, "alpha", 1);
        final ValueAnimator alpha2 = ObjectAnimator.ofFloat(nextTextView, "alpha", 1);

        final AnimatorSet alphaAnimatorSet = new AnimatorSet();
        alphaAnimatorSet.playTogether(alpha1, alpha2);
        alphaAnimatorSet.setStartDelay(ANIMATION_DELAY_MS);
        alphaAnimatorSet.setDuration(ALPHA_MS);

        
        final float bounceDistance = getWidth()/100f; 

        final BounceAnimator prevBounceAnimator = new BounceAnimator(prevTextView, "translationX");
        prevBounceAnimator.queue(new Attributes(bounceDistance, BOUNCE1_MS));
        prevBounceAnimator.queue(new Attributes(-bounceDistance/4, BOUNCE2_MS));
        prevBounceAnimator.queue(new Attributes(0, BOUNCE4_MS));
        prevBounceAnimator.setStartDelay(ANIMATION_DELAY_MS);

        final BounceAnimator nextBounceAnimator = new BounceAnimator(nextTextView, "translationX");
        nextBounceAnimator.queue(new Attributes(-bounceDistance, BOUNCE1_MS));
        nextBounceAnimator.queue(new Attributes(bounceDistance/4, BOUNCE2_MS));
        nextBounceAnimator.queue(new Attributes(0, BOUNCE4_MS));
        nextBounceAnimator.setStartDelay(ANIMATION_DELAY_MS);

        
        alphaAnimatorSet.start();
        prevBounceAnimator.start();
        nextBounceAnimator.start();
    }

    private class PreDrawListener implements ViewTreeObserver.OnPreDrawListener {
        @Override
        public boolean onPreDraw() {
            animateTitles();
            getViewTreeObserver().removeOnPreDrawListener(this);
            return true;
        }
    }
}
