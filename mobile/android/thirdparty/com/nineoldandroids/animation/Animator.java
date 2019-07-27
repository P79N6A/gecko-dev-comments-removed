















package com.nineoldandroids.animation;

import java.util.ArrayList;

import android.view.animation.Interpolator;





public abstract class Animator implements Cloneable {


    


    ArrayList<AnimatorListener> mListeners = null;

    












    public void start() {
    }

    








    public void cancel() {
    }

    







    public void end() {
    }

    





    public abstract long getStartDelay();

    





    public abstract void setStartDelay(long startDelay);


    




    public abstract Animator setDuration(long duration);

    




    public abstract long getDuration();

    







    public abstract void setInterpolator(Interpolator value);

    





    public abstract boolean isRunning();

    








    public boolean isStarted() {
        
        
        return isRunning();
    }

    





    public void addListener(AnimatorListener listener) {
        if (mListeners == null) {
            mListeners = new ArrayList<AnimatorListener>();
        }
        mListeners.add(listener);
    }

    





    public void removeListener(AnimatorListener listener) {
        if (mListeners == null) {
            return;
        }
        mListeners.remove(listener);
        if (mListeners.size() == 0) {
            mListeners = null;
        }
    }

    





    public ArrayList<AnimatorListener> getListeners() {
        return mListeners;
    }

    




    public void removeAllListeners() {
        if (mListeners != null) {
            mListeners.clear();
            mListeners = null;
        }
    }

    @Override
    public Animator clone() {
        try {
            final Animator anim = (Animator) super.clone();
            if (mListeners != null) {
                ArrayList<AnimatorListener> oldListeners = mListeners;
                anim.mListeners = new ArrayList<AnimatorListener>();
                int numListeners = oldListeners.size();
                for (int i = 0; i < numListeners; ++i) {
                    anim.mListeners.add(oldListeners.get(i));
                }
            }
            return anim;
        } catch (CloneNotSupportedException e) {
           throw new AssertionError();
        }
    }

    








    public void setupStartValues() {
    }

    








    public void setupEndValues() {
    }

    







    public void setTarget(Object target) {
    }

    




    public static interface AnimatorListener {
        




        void onAnimationStart(Animator animation);

        





        void onAnimationEnd(Animator animation);

        





        void onAnimationCancel(Animator animation);

        




        void onAnimationRepeat(Animator animation);
    }
}
