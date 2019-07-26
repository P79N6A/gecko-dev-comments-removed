















package org.mozilla.gecko;

import android.view.animation.Animation;
import android.view.animation.Transformation;

import android.graphics.Camera;
import android.graphics.Matrix;





public class Rotate3DAnimation extends Animation {
    private final float mFromDegrees;
    private final float mToDegrees;

    private final float mCenterX;
    private final float mCenterY;

    private final float mDepthZ;
    private final boolean mReverse;
    private Camera mCamera;

    private int mWidth = 1;
    private int mHeight = 1;

    














    public Rotate3DAnimation(float fromDegrees, float toDegrees,
            float centerX, float centerY, float depthZ, boolean reverse) {
        mFromDegrees = fromDegrees;
        mToDegrees = toDegrees;
        mCenterX = centerX;
        mCenterY = centerY;
        mDepthZ = depthZ;
        mReverse = reverse;
    }

   @Override
    public void initialize(int width, int height, int parentWidth, int parentHeight) {
        super.initialize(width, height, parentWidth, parentHeight);
        mCamera = new Camera();
        mWidth = width;
        mHeight = height;
    }

    @Override
    protected void applyTransformation(float interpolatedTime, Transformation t) {
        final float fromDegrees = mFromDegrees;
        float degrees = fromDegrees + ((mToDegrees - fromDegrees) * interpolatedTime);

        final Camera camera = mCamera;
        final Matrix matrix = t.getMatrix();

        camera.save();
        if (mReverse) {
            camera.translate(0.0f, 0.0f, mDepthZ * interpolatedTime);
        } else {
            camera.translate(0.0f, 0.0f, mDepthZ * (1.0f - interpolatedTime));
        }
        camera.rotateX(degrees);
        camera.getMatrix(matrix);
        camera.restore();

        matrix.preTranslate(-mCenterX * mWidth, -mCenterY * mHeight);
        matrix.postTranslate(mCenterX * mWidth, mCenterY * mHeight);
    }
}
