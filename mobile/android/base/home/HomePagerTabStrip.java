




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.BounceAnimatorBuilder;
import org.mozilla.gecko.animation.BounceAnimatorBuilder.Attributes;
import org.mozilla.gecko.animation.TransitionsTracker;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
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
    private static final int ANIMATION_DELAY_MS = 50;
    private static final int ALPHA_MS = 10;
    private static final int BOUNCE1_MS = 350;
    private static final int BOUNCE2_MS = 200;
    private static final int BOUNCE3_MS = 100;
    private static final int INIT_OFFSET = 100;

    private final Paint shadowPaint;
    private final int shadowSize;

    public HomePagerTabStrip(Context context) {
        this(context, null);
    }

    public HomePagerTabStrip(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.HomePagerTabStrip);
        int color = a.getColor(R.styleable.HomePagerTabStrip_tabIndicatorColor, 0x00);
        a.recycle();

        setTabIndicatorColor(color);

        final Resources res = getResources();
        shadowSize = res.getDimensionPixelSize(R.dimen.tabs_strip_shadow_size);

        shadowPaint = new Paint();
        shadowPaint.setColor(res.getColor(R.color.url_bar_shadow));
        shadowPaint.setStrokeWidth(0.0f);

        getViewTreeObserver().addOnPreDrawListener(new PreDrawListener());
    }

    @Override
    public int getPaddingBottom() {
        
        
        
        return 0;
    }

    @Override
    public void draw(Canvas canvas) {
        super.draw(canvas);

        final int height = getHeight();
        canvas.drawRect(0, height - shadowSize, getWidth(), height, shadowPaint);
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
        alphaAnimatorSet.setDuration(ALPHA_MS);
        alphaAnimatorSet.setStartDelay(ANIMATION_DELAY_MS);

        
        final float bounceDistance = getWidth()/100f; 

        final BounceAnimatorBuilder prevBounceAnimatorBuilder = new BounceAnimatorBuilder(prevTextView, "translationX");
        prevBounceAnimatorBuilder.queue(new Attributes(bounceDistance, BOUNCE1_MS));
        prevBounceAnimatorBuilder.queue(new Attributes(-bounceDistance/4, BOUNCE2_MS));
        prevBounceAnimatorBuilder.queue(new Attributes(0, BOUNCE3_MS));

        final BounceAnimatorBuilder nextBounceAnimatorBuilder = new BounceAnimatorBuilder(nextTextView, "translationX");
        nextBounceAnimatorBuilder.queue(new Attributes(-bounceDistance, BOUNCE1_MS));
        nextBounceAnimatorBuilder.queue(new Attributes(bounceDistance/4, BOUNCE2_MS));
        nextBounceAnimatorBuilder.queue(new Attributes(0, BOUNCE3_MS));

        final AnimatorSet bounceAnimatorSet = new AnimatorSet();
        bounceAnimatorSet.playTogether(prevBounceAnimatorBuilder.build(), nextBounceAnimatorBuilder.build());

        TransitionsTracker.track(nextBounceAnimatorBuilder);

        final AnimatorSet titlesAnimatorSet = new AnimatorSet();
        titlesAnimatorSet.playTogether(alphaAnimatorSet, bounceAnimatorSet);
        titlesAnimatorSet.setStartDelay(ANIMATION_DELAY_MS);

        
        titlesAnimatorSet.start();
    }

    private class PreDrawListener implements ViewTreeObserver.OnPreDrawListener {
        @Override
        public boolean onPreDraw() {
            if (!TransitionsTracker.areTransitionsRunning()) {
                
                animateTitles();
            }
            getViewTreeObserver().removeOnPreDrawListener(this);
            return true;
        }
    }
}
