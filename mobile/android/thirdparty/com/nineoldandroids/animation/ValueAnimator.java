















package com.nineoldandroids.animation;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.AndroidRuntimeException;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.view.animation.AnimationUtils;
import android.view.animation.Interpolator;
import android.view.animation.LinearInterpolator;

import java.util.ArrayList;
import java.util.HashMap;













public class ValueAnimator extends Animator {

    



    


    private static final long DEFAULT_FRAME_DELAY = 10;

    



    static final int ANIMATION_START = 0;
    static final int ANIMATION_FRAME = 1;

    



    static final int STOPPED    = 0; 
    static final int RUNNING    = 1; 
    static final int SEEKED     = 2; 

    






    
    
    
    long mStartTime;

    



    long mSeekTime = -1;

    
    
    
    

    
    
    private static ThreadLocal<AnimationHandler> sAnimationHandler =
            new ThreadLocal<AnimationHandler>();

    
    private static final ThreadLocal<ArrayList<ValueAnimator>> sAnimations =
            new ThreadLocal<ArrayList<ValueAnimator>>() {
                @Override
                protected ArrayList<ValueAnimator> initialValue() {
                    return new ArrayList<ValueAnimator>();
                }
            };

    
    private static final ThreadLocal<ArrayList<ValueAnimator>> sPendingAnimations =
            new ThreadLocal<ArrayList<ValueAnimator>>() {
                @Override
                protected ArrayList<ValueAnimator> initialValue() {
                    return new ArrayList<ValueAnimator>();
                }
            };

    



    private static final ThreadLocal<ArrayList<ValueAnimator>> sDelayedAnims =
            new ThreadLocal<ArrayList<ValueAnimator>>() {
                @Override
                protected ArrayList<ValueAnimator> initialValue() {
                    return new ArrayList<ValueAnimator>();
                }
            };

    private static final ThreadLocal<ArrayList<ValueAnimator>> sEndingAnims =
            new ThreadLocal<ArrayList<ValueAnimator>>() {
                @Override
                protected ArrayList<ValueAnimator> initialValue() {
                    return new ArrayList<ValueAnimator>();
                }
            };

    private static final ThreadLocal<ArrayList<ValueAnimator>> sReadyAnims =
            new ThreadLocal<ArrayList<ValueAnimator>>() {
                @Override
                protected ArrayList<ValueAnimator> initialValue() {
                    return new ArrayList<ValueAnimator>();
                }
            };

    
    private static final Interpolator sDefaultInterpolator =
            new AccelerateDecelerateInterpolator();

    
    private static final TypeEvaluator sIntEvaluator = new IntEvaluator();
    private static final TypeEvaluator sFloatEvaluator = new FloatEvaluator();

    



    private boolean mPlayingBackwards = false;

    



    private int mCurrentIteration = 0;

    


    private float mCurrentFraction = 0f;

    


    private boolean mStartedDelay = false;

    





    private long mDelayStartTime;

    





    int mPlayingState = STOPPED;

    







    private boolean mRunning = false;

    



    private boolean mStarted = false;

    



    boolean mInitialized = false;

    
    
    

    
    private long mDuration = 300;

    
    private long mStartDelay = 0;

    
    private static long sFrameDelay = DEFAULT_FRAME_DELAY;

    
    
    private int mRepeatCount = 0;

    




    private int mRepeatMode = RESTART;

    




    private Interpolator mInterpolator = sDefaultInterpolator;

    


    private ArrayList<AnimatorUpdateListener> mUpdateListeners = null;

    


    PropertyValuesHolder[] mValues;

    



    HashMap<String, PropertyValuesHolder> mValuesMap;

    



    



    public static final int RESTART = 1;
    



    public static final int REVERSE = 2;
    



    public static final int INFINITE = -1;

    




    public ValueAnimator() {
    }

    










    public static ValueAnimator ofInt(int... values) {
        ValueAnimator anim = new ValueAnimator();
        anim.setIntValues(values);
        return anim;
    }

    










    public static ValueAnimator ofFloat(float... values) {
        ValueAnimator anim = new ValueAnimator();
        anim.setFloatValues(values);
        return anim;
    }

    







    public static ValueAnimator ofPropertyValuesHolder(PropertyValuesHolder... values) {
        ValueAnimator anim = new ValueAnimator();
        anim.setValues(values);
        return anim;
    }
    

















    public static ValueAnimator ofObject(TypeEvaluator evaluator, Object... values) {
        ValueAnimator anim = new ValueAnimator();
        anim.setObjectValues(values);
        anim.setEvaluator(evaluator);
        return anim;
    }

    













    public void setIntValues(int... values) {
        if (values == null || values.length == 0) {
            return;
        }
        if (mValues == null || mValues.length == 0) {
            setValues(new PropertyValuesHolder[]{PropertyValuesHolder.ofInt("", values)});
        } else {
            PropertyValuesHolder valuesHolder = mValues[0];
            valuesHolder.setIntValues(values);
        }
        
        mInitialized = false;
    }

    













    public void setFloatValues(float... values) {
        if (values == null || values.length == 0) {
            return;
        }
        if (mValues == null || mValues.length == 0) {
            setValues(new PropertyValuesHolder[]{PropertyValuesHolder.ofFloat("", values)});
        } else {
            PropertyValuesHolder valuesHolder = mValues[0];
            valuesHolder.setFloatValues(values);
        }
        
        mInitialized = false;
    }

    

















    public void setObjectValues(Object... values) {
        if (values == null || values.length == 0) {
            return;
        }
        if (mValues == null || mValues.length == 0) {
            setValues(new PropertyValuesHolder[]{PropertyValuesHolder.ofObject("",
                    (TypeEvaluator)null, values)});
        } else {
            PropertyValuesHolder valuesHolder = mValues[0];
            valuesHolder.setObjectValues(values);
        }
        
        mInitialized = false;
    }

    







    public void setValues(PropertyValuesHolder... values) {
        int numValues = values.length;
        mValues = values;
        mValuesMap = new HashMap<String, PropertyValuesHolder>(numValues);
        for (int i = 0; i < numValues; ++i) {
            PropertyValuesHolder valuesHolder = (PropertyValuesHolder) values[i];
            mValuesMap.put(valuesHolder.getPropertyName(), valuesHolder);
        }
        
        mInitialized = false;
    }

    







    public PropertyValuesHolder[] getValues() {
        return mValues;
    }

    









    void initAnimation() {
        if (!mInitialized) {
            int numValues = mValues.length;
            for (int i = 0; i < numValues; ++i) {
                mValues[i].init();
            }
            mInitialized = true;
        }
    }


    








    public ValueAnimator setDuration(long duration) {
        if (duration < 0) {
            throw new IllegalArgumentException("Animators cannot have negative duration: " +
                    duration);
        }
        mDuration = duration;
        return this;
    }

    




    public long getDuration() {
        return mDuration;
    }

    









    public void setCurrentPlayTime(long playTime) {
        initAnimation();
        long currentTime = AnimationUtils.currentAnimationTimeMillis();
        if (mPlayingState != RUNNING) {
            mSeekTime = playTime;
            mPlayingState = SEEKED;
        }
        mStartTime = currentTime - playTime;
        animationFrame(currentTime);
    }

    






    public long getCurrentPlayTime() {
        if (!mInitialized || mPlayingState == STOPPED) {
            return 0;
        }
        return AnimationUtils.currentAnimationTimeMillis() - mStartTime;
    }

    







    private static class AnimationHandler extends Handler {
        











        @Override
        public void handleMessage(Message msg) {
            boolean callAgain = true;
            ArrayList<ValueAnimator> animations = sAnimations.get();
            ArrayList<ValueAnimator> delayedAnims = sDelayedAnims.get();
            switch (msg.what) {
                
                
                case ANIMATION_START:
                    ArrayList<ValueAnimator> pendingAnimations = sPendingAnimations.get();
                    if (animations.size() > 0 || delayedAnims.size() > 0) {
                        callAgain = false;
                    }
                    
                    
                    
                    
                    
                    while (pendingAnimations.size() > 0) {
                        ArrayList<ValueAnimator> pendingCopy =
                                (ArrayList<ValueAnimator>) pendingAnimations.clone();
                        pendingAnimations.clear();
                        int count = pendingCopy.size();
                        for (int i = 0; i < count; ++i) {
                            ValueAnimator anim = pendingCopy.get(i);
                            
                            if (anim.mStartDelay == 0) {
                                anim.startAnimation();
                            } else {
                                delayedAnims.add(anim);
                            }
                        }
                    }
                    
                case ANIMATION_FRAME:
                    
                    
                    long currentTime = AnimationUtils.currentAnimationTimeMillis();
                    ArrayList<ValueAnimator> readyAnims = sReadyAnims.get();
                    ArrayList<ValueAnimator> endingAnims = sEndingAnims.get();

                    
                    
                    int numDelayedAnims = delayedAnims.size();
                    for (int i = 0; i < numDelayedAnims; ++i) {
                        ValueAnimator anim = delayedAnims.get(i);
                        if (anim.delayedAnimationFrame(currentTime)) {
                            readyAnims.add(anim);
                        }
                    }
                    int numReadyAnims = readyAnims.size();
                    if (numReadyAnims > 0) {
                        for (int i = 0; i < numReadyAnims; ++i) {
                            ValueAnimator anim = readyAnims.get(i);
                            anim.startAnimation();
                            anim.mRunning = true;
                            delayedAnims.remove(anim);
                        }
                        readyAnims.clear();
                    }

                    
                    
                    int numAnims = animations.size();
                    int i = 0;
                    while (i < numAnims) {
                        ValueAnimator anim = animations.get(i);
                        if (anim.animationFrame(currentTime)) {
                            endingAnims.add(anim);
                        }
                        if (animations.size() == numAnims) {
                            ++i;
                        } else {
                            
                            
                            
                            
                            
                            
                            
                            --numAnims;
                            endingAnims.remove(anim);
                        }
                    }
                    if (endingAnims.size() > 0) {
                        for (i = 0; i < endingAnims.size(); ++i) {
                            endingAnims.get(i).endAnimation();
                        }
                        endingAnims.clear();
                    }

                    
                    
                    if (callAgain && (!animations.isEmpty() || !delayedAnims.isEmpty())) {
                        sendEmptyMessageDelayed(ANIMATION_FRAME, Math.max(0, sFrameDelay -
                            (AnimationUtils.currentAnimationTimeMillis() - currentTime)));
                    }
                    break;
            }
        }
    }

    





    public long getStartDelay() {
        return mStartDelay;
    }

    





    public void setStartDelay(long startDelay) {
        this.mStartDelay = startDelay;
    }

    








    public static long getFrameDelay() {
        return sFrameDelay;
    }

    








    public static void setFrameDelay(long frameDelay) {
        sFrameDelay = frameDelay;
    }

    











    public Object getAnimatedValue() {
        if (mValues != null && mValues.length > 0) {
            return mValues[0].getAnimatedValue();
        }
        
        return null;
    }

    









    public Object getAnimatedValue(String propertyName) {
        PropertyValuesHolder valuesHolder = mValuesMap.get(propertyName);
        if (valuesHolder != null) {
            return valuesHolder.getAnimatedValue();
        } else {
            
            return null;
        }
    }

    







    public void setRepeatCount(int value) {
        mRepeatCount = value;
    }
    





    public int getRepeatCount() {
        return mRepeatCount;
    }

    






    public void setRepeatMode(int value) {
        mRepeatMode = value;
    }

    




    public int getRepeatMode() {
        return mRepeatMode;
    }

    






    public void addUpdateListener(AnimatorUpdateListener listener) {
        if (mUpdateListeners == null) {
            mUpdateListeners = new ArrayList<AnimatorUpdateListener>();
        }
        mUpdateListeners.add(listener);
    }

    


    public void removeAllUpdateListeners() {
        if (mUpdateListeners == null) {
            return;
        }
        mUpdateListeners.clear();
        mUpdateListeners = null;
    }

    





    public void removeUpdateListener(AnimatorUpdateListener listener) {
        if (mUpdateListeners == null) {
            return;
        }
        mUpdateListeners.remove(listener);
        if (mUpdateListeners.size() == 0) {
            mUpdateListeners = null;
        }
    }


    








    @Override
    public void setInterpolator(Interpolator value) {
        if (value != null) {
            mInterpolator = value;
        } else {
            mInterpolator = new LinearInterpolator();
        }
    }

    




    public Interpolator getInterpolator() {
        return mInterpolator;
    }

    















    public void setEvaluator(TypeEvaluator value) {
        if (value != null && mValues != null && mValues.length > 0) {
            mValues[0].setEvaluator(value);
        }
    }

    












    private void start(boolean playBackwards) {
        if (Looper.myLooper() == null) {
            throw new AndroidRuntimeException("Animators may only be run on Looper threads");
        }
        mPlayingBackwards = playBackwards;
        mCurrentIteration = 0;
        mPlayingState = STOPPED;
        mStarted = true;
        mStartedDelay = false;
        sPendingAnimations.get().add(this);
        if (mStartDelay == 0) {
            
            setCurrentPlayTime(getCurrentPlayTime());
            mPlayingState = STOPPED;
            mRunning = true;

            if (mListeners != null) {
                ArrayList<AnimatorListener> tmpListeners =
                        (ArrayList<AnimatorListener>) mListeners.clone();
                int numListeners = tmpListeners.size();
                for (int i = 0; i < numListeners; ++i) {
                    tmpListeners.get(i).onAnimationStart(this);
                }
            }
        }
        AnimationHandler animationHandler = sAnimationHandler.get();
        if (animationHandler == null) {
            animationHandler = new AnimationHandler();
            sAnimationHandler.set(animationHandler);
        }
        animationHandler.sendEmptyMessage(ANIMATION_START);
    }

    @Override
    public void start() {
        start(false);
    }

    @Override
    public void cancel() {
        
        
        if (mPlayingState != STOPPED || sPendingAnimations.get().contains(this) ||
                sDelayedAnims.get().contains(this)) {
            
            if (mRunning && mListeners != null) {
                ArrayList<AnimatorListener> tmpListeners =
                        (ArrayList<AnimatorListener>) mListeners.clone();
                for (AnimatorListener listener : tmpListeners) {
                    listener.onAnimationCancel(this);
                }
            }
            endAnimation();
        }
    }

    @Override
    public void end() {
        if (!sAnimations.get().contains(this) && !sPendingAnimations.get().contains(this)) {
            
            mStartedDelay = false;
            startAnimation();
        } else if (!mInitialized) {
            initAnimation();
        }
        
        
        if (mRepeatCount > 0 && (mRepeatCount & 0x01) == 1) {
            animateValue(0f);
        } else {
            animateValue(1f);
        }
        endAnimation();
    }

    @Override
    public boolean isRunning() {
        return (mPlayingState == RUNNING || mRunning);
    }

    @Override
    public boolean isStarted() {
        return mStarted;
    }

    






    public void reverse() {
        mPlayingBackwards = !mPlayingBackwards;
        if (mPlayingState == RUNNING) {
            long currentTime = AnimationUtils.currentAnimationTimeMillis();
            long currentPlayTime = currentTime - mStartTime;
            long timeLeft = mDuration - currentPlayTime;
            mStartTime = currentTime - timeLeft;
        } else {
            start(true);
        }
    }

    



    private void endAnimation() {
        sAnimations.get().remove(this);
        sPendingAnimations.get().remove(this);
        sDelayedAnims.get().remove(this);
        mPlayingState = STOPPED;
        if (mRunning && mListeners != null) {
            ArrayList<AnimatorListener> tmpListeners =
                    (ArrayList<AnimatorListener>) mListeners.clone();
            int numListeners = tmpListeners.size();
            for (int i = 0; i < numListeners; ++i) {
                tmpListeners.get(i).onAnimationEnd(this);
            }
        }
        mRunning = false;
        mStarted = false;
    }

    



    private void startAnimation() {
        initAnimation();
        sAnimations.get().add(this);
        if (mStartDelay > 0 && mListeners != null) {
            
            
            ArrayList<AnimatorListener> tmpListeners =
                    (ArrayList<AnimatorListener>) mListeners.clone();
            int numListeners = tmpListeners.size();
            for (int i = 0; i < numListeners; ++i) {
                tmpListeners.get(i).onAnimationStart(this);
            }
        }
    }

    









    private boolean delayedAnimationFrame(long currentTime) {
        if (!mStartedDelay) {
            mStartedDelay = true;
            mDelayStartTime = currentTime;
        } else {
            long deltaTime = currentTime - mDelayStartTime;
            if (deltaTime > mStartDelay) {
                
                
                mStartTime = currentTime - (deltaTime - mStartDelay);
                mPlayingState = RUNNING;
                return true;
            }
        }
        return false;
    }

    











    boolean animationFrame(long currentTime) {
        boolean done = false;

        if (mPlayingState == STOPPED) {
            mPlayingState = RUNNING;
            if (mSeekTime < 0) {
                mStartTime = currentTime;
            } else {
                mStartTime = currentTime - mSeekTime;
                
                mSeekTime = -1;
            }
        }
        switch (mPlayingState) {
        case RUNNING:
        case SEEKED:
            float fraction = mDuration > 0 ? (float)(currentTime - mStartTime) / mDuration : 1f;
            if (fraction >= 1f) {
                if (mCurrentIteration < mRepeatCount || mRepeatCount == INFINITE) {
                    
                    if (mListeners != null) {
                        int numListeners = mListeners.size();
                        for (int i = 0; i < numListeners; ++i) {
                            mListeners.get(i).onAnimationRepeat(this);
                        }
                    }
                    if (mRepeatMode == REVERSE) {
                        mPlayingBackwards = mPlayingBackwards ? false : true;
                    }
                    mCurrentIteration += (int)fraction;
                    fraction = fraction % 1f;
                    mStartTime += mDuration;
                } else {
                    done = true;
                    fraction = Math.min(fraction, 1.0f);
                }
            }
            if (mPlayingBackwards) {
                fraction = 1f - fraction;
            }
            animateValue(fraction);
            break;
        }

        return done;
    }

    





    public float getAnimatedFraction() {
        return mCurrentFraction;
    }

    











    void animateValue(float fraction) {
        fraction = mInterpolator.getInterpolation(fraction);
        mCurrentFraction = fraction;
        int numValues = mValues.length;
        for (int i = 0; i < numValues; ++i) {
            mValues[i].calculateValue(fraction);
        }
        if (mUpdateListeners != null) {
            int numListeners = mUpdateListeners.size();
            for (int i = 0; i < numListeners; ++i) {
                mUpdateListeners.get(i).onAnimationUpdate(this);
            }
        }
    }

    @Override
    public ValueAnimator clone() {
        final ValueAnimator anim = (ValueAnimator) super.clone();
        if (mUpdateListeners != null) {
            ArrayList<AnimatorUpdateListener> oldListeners = mUpdateListeners;
            anim.mUpdateListeners = new ArrayList<AnimatorUpdateListener>();
            int numListeners = oldListeners.size();
            for (int i = 0; i < numListeners; ++i) {
                anim.mUpdateListeners.add(oldListeners.get(i));
            }
        }
        anim.mSeekTime = -1;
        anim.mPlayingBackwards = false;
        anim.mCurrentIteration = 0;
        anim.mInitialized = false;
        anim.mPlayingState = STOPPED;
        anim.mStartedDelay = false;
        PropertyValuesHolder[] oldValues = mValues;
        if (oldValues != null) {
            int numValues = oldValues.length;
            anim.mValues = new PropertyValuesHolder[numValues];
            anim.mValuesMap = new HashMap<String, PropertyValuesHolder>(numValues);
            for (int i = 0; i < numValues; ++i) {
                PropertyValuesHolder newValuesHolder = oldValues[i].clone();
                anim.mValues[i] = newValuesHolder;
                anim.mValuesMap.put(newValuesHolder.getPropertyName(), newValuesHolder);
            }
        }
        return anim;
    }

    





    public static interface AnimatorUpdateListener {
        




        void onAnimationUpdate(ValueAnimator animation);

    }

    







    public static int getCurrentAnimationsCount() {
        return sAnimations.get().size();
    }

    





    public static void clearAllAnimations() {
        sAnimations.get().clear();
        sPendingAnimations.get().clear();
        sDelayedAnims.get().clear();
    }

    @Override
    public String toString() {
        String returnVal = "ValueAnimator@" + Integer.toHexString(hashCode());
        if (mValues != null) {
            for (int i = 0; i < mValues.length; ++i) {
                returnVal += "\n    " + mValues[i].toString();
            }
        }
        return returnVal;
    }
}
