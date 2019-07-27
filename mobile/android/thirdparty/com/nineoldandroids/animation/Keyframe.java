















package com.nineoldandroids.animation;

import android.view.animation.Interpolator;


















public abstract class Keyframe implements Cloneable {
    


    float mFraction;

    



    Class mValueType;

    



    private Interpolator mInterpolator = null;

    




    boolean mHasValue = false;

    











    public static Keyframe ofInt(float fraction, int value) {
        return new IntKeyframe(fraction, value);
    }

    











    public static Keyframe ofInt(float fraction) {
        return new IntKeyframe(fraction);
    }

    











    public static Keyframe ofFloat(float fraction, float value) {
        return new FloatKeyframe(fraction, value);
    }

    











    public static Keyframe ofFloat(float fraction) {
        return new FloatKeyframe(fraction);
    }

    











    public static Keyframe ofObject(float fraction, Object value) {
        return new ObjectKeyframe(fraction, value);
    }

    











    public static Keyframe ofObject(float fraction) {
        return new ObjectKeyframe(fraction, null);
    }

    






    public boolean hasValue() {
        return mHasValue;
    }

    




    public abstract Object getValue();

    




    public abstract void setValue(Object value);

    





    public float getFraction() {
        return mFraction;
    }

    





    public void setFraction(float fraction) {
        mFraction = fraction;
    }

    





    public Interpolator getInterpolator() {
        return mInterpolator;
    }

    





    public void setInterpolator(Interpolator interpolator) {
        mInterpolator = interpolator;
    }

    






    public Class getType() {
        return mValueType;
    }

    @Override
    public abstract Keyframe clone();

    


    static class ObjectKeyframe extends Keyframe {

        


        Object mValue;

        ObjectKeyframe(float fraction, Object value) {
            mFraction = fraction;
            mValue = value;
            mHasValue = (value != null);
            mValueType = mHasValue ? value.getClass() : Object.class;
        }

        public Object getValue() {
            return mValue;
        }

        public void setValue(Object value) {
            mValue = value;
            mHasValue = (value != null);
        }

        @Override
        public ObjectKeyframe clone() {
            ObjectKeyframe kfClone = new ObjectKeyframe(getFraction(), mValue);
            kfClone.setInterpolator(getInterpolator());
            return kfClone;
        }
    }

    


    static class IntKeyframe extends Keyframe {

        


        int mValue;

        IntKeyframe(float fraction, int value) {
            mFraction = fraction;
            mValue = value;
            mValueType = int.class;
            mHasValue = true;
        }

        IntKeyframe(float fraction) {
            mFraction = fraction;
            mValueType = int.class;
        }

        public int getIntValue() {
            return mValue;
        }

        public Object getValue() {
            return mValue;
        }

        public void setValue(Object value) {
            if (value != null && value.getClass() == Integer.class) {
                mValue = ((Integer)value).intValue();
                mHasValue = true;
            }
        }

        @Override
        public IntKeyframe clone() {
            IntKeyframe kfClone = new IntKeyframe(getFraction(), mValue);
            kfClone.setInterpolator(getInterpolator());
            return kfClone;
        }
    }

    


    static class FloatKeyframe extends Keyframe {
        


        float mValue;

        FloatKeyframe(float fraction, float value) {
            mFraction = fraction;
            mValue = value;
            mValueType = float.class;
            mHasValue = true;
        }

        FloatKeyframe(float fraction) {
            mFraction = fraction;
            mValueType = float.class;
        }

        public float getFloatValue() {
            return mValue;
        }

        public Object getValue() {
            return mValue;
        }

        public void setValue(Object value) {
            if (value != null && value.getClass() == Float.class) {
                mValue = ((Float)value).floatValue();
                mHasValue = true;
            }
        }

        @Override
        public FloatKeyframe clone() {
            FloatKeyframe kfClone = new FloatKeyframe(getFraction(), mValue);
            kfClone.setInterpolator(getInterpolator());
            return kfClone;
        }
    }
}
