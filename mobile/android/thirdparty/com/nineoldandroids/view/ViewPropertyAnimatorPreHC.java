















package com.nineoldandroids.view;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;
import android.view.View;
import android.view.animation.Interpolator;
import com.nineoldandroids.animation.Animator;
import com.nineoldandroids.animation.ValueAnimator;
import com.nineoldandroids.view.animation.AnimatorProxy;

class ViewPropertyAnimatorPreHC extends ViewPropertyAnimator {
    



    private final AnimatorProxy mProxy;

    



    private final WeakReference<View> mView;

    




    private long mDuration;

    



    private boolean mDurationSet = false;

    




    private long mStartDelay = 0;

    



    private boolean mStartDelaySet = false;

    




    private Interpolator mInterpolator;

    



    private boolean mInterpolatorSet = false;

    


    private Animator.AnimatorListener mListener = null;

    




    private AnimatorEventListener mAnimatorEventListener = new AnimatorEventListener();

    







    ArrayList<NameValuesHolder> mPendingAnimations = new ArrayList<NameValuesHolder>();

    



    private static final int NONE           = 0x0000;
    private static final int TRANSLATION_X  = 0x0001;
    private static final int TRANSLATION_Y  = 0x0002;
    private static final int SCALE_X        = 0x0004;
    private static final int SCALE_Y        = 0x0008;
    private static final int ROTATION       = 0x0010;
    private static final int ROTATION_X     = 0x0020;
    private static final int ROTATION_Y     = 0x0040;
    private static final int X              = 0x0080;
    private static final int Y              = 0x0100;
    private static final int ALPHA          = 0x0200;

    private static final int TRANSFORM_MASK = TRANSLATION_X | TRANSLATION_Y | SCALE_X | SCALE_Y |
            ROTATION | ROTATION_X | ROTATION_Y | X | Y;

    







    private Runnable mAnimationStarter = new Runnable() {
        @Override
        public void run() {
            startAnimation();
        }
    };

    




    private static class PropertyBundle {
        int mPropertyMask;
        ArrayList<NameValuesHolder> mNameValuesHolder;

        PropertyBundle(int propertyMask, ArrayList<NameValuesHolder> nameValuesHolder) {
            mPropertyMask = propertyMask;
            mNameValuesHolder = nameValuesHolder;
        }

        









        boolean cancel(int propertyConstant) {
            if ((mPropertyMask & propertyConstant) != 0 && mNameValuesHolder != null) {
                int count = mNameValuesHolder.size();
                for (int i = 0; i < count; ++i) {
                    NameValuesHolder nameValuesHolder = mNameValuesHolder.get(i);
                    if (nameValuesHolder.mNameConstant == propertyConstant) {
                        mNameValuesHolder.remove(i);
                        mPropertyMask &= ~propertyConstant;
                        return true;
                    }
                }
            }
            return false;
        }
    }

    








    private HashMap<Animator, PropertyBundle> mAnimatorMap =
            new HashMap<Animator, PropertyBundle>();

    





    private static class NameValuesHolder {
        int mNameConstant;
        float mFromValue;
        float mDeltaValue;
        NameValuesHolder(int nameConstant, float fromValue, float deltaValue) {
            mNameConstant = nameConstant;
            mFromValue = fromValue;
            mDeltaValue = deltaValue;
        }
    }

    





    ViewPropertyAnimatorPreHC(View view) {
        mView = new WeakReference<View>(view);
        mProxy = AnimatorProxy.wrap(view);
    }

    







    public ViewPropertyAnimator setDuration(long duration) {
        if (duration < 0) {
            throw new IllegalArgumentException("Animators cannot have negative duration: " +
                    duration);
        }
        mDurationSet = true;
        mDuration = duration;
        return this;
    }

    







    public long getDuration() {
        if (mDurationSet) {
            return mDuration;
        } else {
            
            
            return new ValueAnimator().getDuration();
        }
    }

    @Override
    public long getStartDelay() {
        if (mStartDelaySet) {
            return mStartDelay;
        } else {
            
            
            return 0;
        }
    }

    @Override
    public ViewPropertyAnimator setStartDelay(long startDelay) {
        if (startDelay < 0) {
            throw new IllegalArgumentException("Animators cannot have negative duration: " +
                    startDelay);
        }
        mStartDelaySet = true;
        mStartDelay = startDelay;
        return this;
    }

    @Override
    public ViewPropertyAnimator setInterpolator(Interpolator interpolator) {
        mInterpolatorSet = true;
        mInterpolator = interpolator;
        return this;
    }

    @Override
    public ViewPropertyAnimator setListener(Animator.AnimatorListener listener) {
        mListener = listener;
        return this;
    }

    @Override
    public void start() {
        startAnimation();
    }

    @Override
    public void cancel() {
        if (mAnimatorMap.size() > 0) {
            HashMap<Animator, PropertyBundle> mAnimatorMapCopy =
                    (HashMap<Animator, PropertyBundle>)mAnimatorMap.clone();
            Set<Animator> animatorSet = mAnimatorMapCopy.keySet();
            for (Animator runningAnim : animatorSet) {
                runningAnim.cancel();
            }
        }
        mPendingAnimations.clear();
        View v = mView.get();
        if (v != null) {
            v.removeCallbacks(mAnimationStarter);
        }
    }

    @Override
    public ViewPropertyAnimator x(float value) {
        animateProperty(X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator xBy(float value) {
        animatePropertyBy(X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator y(float value) {
        animateProperty(Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator yBy(float value) {
        animatePropertyBy(Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator rotation(float value) {
        animateProperty(ROTATION, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator rotationBy(float value) {
        animatePropertyBy(ROTATION, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator rotationX(float value) {
        animateProperty(ROTATION_X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator rotationXBy(float value) {
        animatePropertyBy(ROTATION_X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator rotationY(float value) {
        animateProperty(ROTATION_Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator rotationYBy(float value) {
        animatePropertyBy(ROTATION_Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator translationX(float value) {
        animateProperty(TRANSLATION_X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator translationXBy(float value) {
        animatePropertyBy(TRANSLATION_X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator translationY(float value) {
        animateProperty(TRANSLATION_Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator translationYBy(float value) {
        animatePropertyBy(TRANSLATION_Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator scaleX(float value) {
        animateProperty(SCALE_X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator scaleXBy(float value) {
        animatePropertyBy(SCALE_X, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator scaleY(float value) {
        animateProperty(SCALE_Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator scaleYBy(float value) {
        animatePropertyBy(SCALE_Y, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator alpha(float value) {
        animateProperty(ALPHA, value);
        return this;
    }

    @Override
    public ViewPropertyAnimator alphaBy(float value) {
        animatePropertyBy(ALPHA, value);
        return this;
    }

    




    private void startAnimation() {
        ValueAnimator animator = ValueAnimator.ofFloat(1.0f);
        ArrayList<NameValuesHolder> nameValueList =
                (ArrayList<NameValuesHolder>) mPendingAnimations.clone();
        mPendingAnimations.clear();
        int propertyMask = 0;
        int propertyCount = nameValueList.size();
        for (int i = 0; i < propertyCount; ++i) {
            NameValuesHolder nameValuesHolder = nameValueList.get(i);
            propertyMask |= nameValuesHolder.mNameConstant;
        }
        mAnimatorMap.put(animator, new PropertyBundle(propertyMask, nameValueList));
        animator.addUpdateListener(mAnimatorEventListener);
        animator.addListener(mAnimatorEventListener);
        if (mStartDelaySet) {
            animator.setStartDelay(mStartDelay);
        }
        if (mDurationSet) {
            animator.setDuration(mDuration);
        }
        if (mInterpolatorSet) {
            animator.setInterpolator(mInterpolator);
        }
        animator.start();
    }

    











    private void animateProperty(int constantName, float toValue) {
        float fromValue = getValue(constantName);
        float deltaValue = toValue - fromValue;
        animatePropertyBy(constantName, fromValue, deltaValue);
    }

    







    private void animatePropertyBy(int constantName, float byValue) {
        float fromValue = getValue(constantName);
        animatePropertyBy(constantName, fromValue, byValue);
    }

    







    private void animatePropertyBy(int constantName, float startValue, float byValue) {
        
        if (mAnimatorMap.size() > 0) {
            Animator animatorToCancel = null;
            Set<Animator> animatorSet = mAnimatorMap.keySet();
            for (Animator runningAnim : animatorSet) {
                PropertyBundle bundle = mAnimatorMap.get(runningAnim);
                if (bundle.cancel(constantName)) {
                    
                    
                    
                    
                    if (bundle.mPropertyMask == NONE) {
                        
                        animatorToCancel = runningAnim;
                        break;
                    }
                }
            }
            if (animatorToCancel != null) {
                animatorToCancel.cancel();
            }
        }

        NameValuesHolder nameValuePair = new NameValuesHolder(constantName, startValue, byValue);
        mPendingAnimations.add(nameValuePair);
        View v = mView.get();
        if (v != null) {
            v.removeCallbacks(mAnimationStarter);
            v.post(mAnimationStarter);
        }
    }

    







    private void setValue(int propertyConstant, float value) {
        
        switch (propertyConstant) {
            case TRANSLATION_X:
                
                mProxy.setTranslationX(value);
                break;
            case TRANSLATION_Y:
                
                mProxy.setTranslationY(value);
                break;
            case ROTATION:
                
                mProxy.setRotation(value);
                break;
            case ROTATION_X:
                
                mProxy.setRotationX(value);
                break;
            case ROTATION_Y:
                
                mProxy.setRotationY(value);
                break;
            case SCALE_X:
                
                mProxy.setScaleX(value);
                break;
            case SCALE_Y:
                
                mProxy.setScaleY(value);
                break;
            case X:
                
                mProxy.setX(value);
                break;
            case Y:
                
                mProxy.setY(value);
                break;
            case ALPHA:
                
                mProxy.setAlpha(value);
                break;
        }
    }

    





    private float getValue(int propertyConstant) {
        
        switch (propertyConstant) {
            case TRANSLATION_X:
                
                return mProxy.getTranslationX();
            case TRANSLATION_Y:
                
                return mProxy.getTranslationY();
            case ROTATION:
                
                return mProxy.getRotation();
            case ROTATION_X:
                
                return mProxy.getRotationX();
            case ROTATION_Y:
                
                return mProxy.getRotationY();
            case SCALE_X:
                
                return mProxy.getScaleX();
            case SCALE_Y:
                
                return mProxy.getScaleY();
            case X:
                
                return mProxy.getX();
            case Y:
                
                return mProxy.getY();
            case ALPHA:
                
                return mProxy.getAlpha();
        }
        return 0;
    }

    





    private class AnimatorEventListener
            implements Animator.AnimatorListener, ValueAnimator.AnimatorUpdateListener {
        @Override
        public void onAnimationStart(Animator animation) {
            if (mListener != null) {
                mListener.onAnimationStart(animation);
            }
        }

        @Override
        public void onAnimationCancel(Animator animation) {
            if (mListener != null) {
                mListener.onAnimationCancel(animation);
            }
        }

        @Override
        public void onAnimationRepeat(Animator animation) {
            if (mListener != null) {
                mListener.onAnimationRepeat(animation);
            }
        }

        @Override
        public void onAnimationEnd(Animator animation) {
            if (mListener != null) {
                mListener.onAnimationEnd(animation);
            }
            mAnimatorMap.remove(animation);
            
            
            
            if (mAnimatorMap.isEmpty()) {
                mListener = null;
            }
        }

        







        @Override
        public void onAnimationUpdate(ValueAnimator animation) {
            
            
            
            
            
            
            
            float fraction = animation.getAnimatedFraction();
            PropertyBundle propertyBundle = mAnimatorMap.get(animation);
            int propertyMask = propertyBundle.mPropertyMask;
            if ((propertyMask & TRANSFORM_MASK) != 0) {
                View v = mView.get();
                if (v != null) {
                    v.invalidate();
                }
            }
            ArrayList<NameValuesHolder> valueList = propertyBundle.mNameValuesHolder;
            if (valueList != null) {
                int count = valueList.size();
                for (int i = 0; i < count; ++i) {
                    NameValuesHolder values = valueList.get(i);
                    float value = values.mFromValue + fraction * values.mDeltaValue;
                    
                    
                    
                        setValue(values.mNameConstant, value);
                    
                }
            }
            



            
            
            View v = mView.get();
            if (v != null) {
                v.invalidate();
            }
        }
    }
}
