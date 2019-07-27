















package com.nineoldandroids.view;

import java.util.WeakHashMap;
import android.os.Build;
import android.view.View;
import android.view.animation.Interpolator;
import com.nineoldandroids.animation.Animator;






















public abstract class ViewPropertyAnimator {
    private static final WeakHashMap<View, ViewPropertyAnimator> ANIMATORS =
            new WeakHashMap<View, ViewPropertyAnimator>(0);

    






    public static ViewPropertyAnimator animate(View view) {
        ViewPropertyAnimator animator = ANIMATORS.get(view);
        if (animator == null) {
            final int version = Integer.valueOf(Build.VERSION.SDK);
            if (version >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
                animator = new ViewPropertyAnimatorICS(view);
            } else if (version >= Build.VERSION_CODES.HONEYCOMB) {
                animator = new ViewPropertyAnimatorHC(view);
            } else {
                animator = new ViewPropertyAnimatorPreHC(view);
            }
            ANIMATORS.put(view, animator);
        }
        return animator;
    }


    







    public abstract ViewPropertyAnimator setDuration(long duration);

    







    public abstract long getDuration();

    







    public abstract long getStartDelay();

    







    public abstract ViewPropertyAnimator setStartDelay(long startDelay);

    







    public abstract ViewPropertyAnimator setInterpolator(Interpolator interpolator);

    






    public abstract ViewPropertyAnimator setListener(Animator.AnimatorListener listener);

    






    public abstract void start();

    


    public abstract void cancel();

    







    public abstract ViewPropertyAnimator x(float value);

    







    public abstract ViewPropertyAnimator xBy(float value);

    







    public abstract ViewPropertyAnimator y(float value);

    







    public abstract ViewPropertyAnimator yBy(float value);

    







    public abstract ViewPropertyAnimator rotation(float value);

    







    public abstract ViewPropertyAnimator rotationBy(float value);

    







    public abstract ViewPropertyAnimator rotationX(float value);

    







    public abstract ViewPropertyAnimator rotationXBy(float value);

    







    public abstract ViewPropertyAnimator rotationY(float value);

    







    public abstract ViewPropertyAnimator rotationYBy(float value);

    







    public abstract ViewPropertyAnimator translationX(float value);

    







    public abstract ViewPropertyAnimator translationXBy(float value);

    







    public abstract ViewPropertyAnimator translationY(float value);

    







    public abstract ViewPropertyAnimator translationYBy(float value);

    







    public abstract ViewPropertyAnimator scaleX(float value);

    







    public abstract ViewPropertyAnimator scaleXBy(float value);

    







    public abstract ViewPropertyAnimator scaleY(float value);

    







    public abstract ViewPropertyAnimator scaleYBy(float value);

    







    public abstract ViewPropertyAnimator alpha(float value);

    







    public abstract ViewPropertyAnimator alphaBy(float value);
}
