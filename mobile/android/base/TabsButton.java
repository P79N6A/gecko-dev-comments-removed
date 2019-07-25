



package org.mozilla.gecko;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PorterDuffXfermode;
import android.graphics.PorterDuff.Mode;
import android.util.AttributeSet;

public class TabsButton extends ShapedButton {
    private Paint mPaint;

    private Path mBackgroundPath;
    private Path mLeftCurve;
    private Path mRightCurve;

    private boolean mCropped;
    private int mFullWidth;

    public TabsButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabsButton);
        mCropped = a.getBoolean(R.styleable.TabsButton_cropped, false);
        a.recycle();

        
        mPaint = new Paint();
        mPaint.setAntiAlias(true);
        mPaint.setColor(0xFF000000);

        
        mPath = new Path();
        mBackgroundPath = new Path();
        mLeftCurve = new Path();
        mRightCurve = new Path();
        mCanvasDelegate = new CanvasDelegate(this, Mode.DST_IN);

        
        mFullWidth = (int) context.getResources().getDimension(R.dimen.tabs_button_full_width);
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        int width = getMeasuredWidth();
        int height = getMeasuredHeight();
        float curve = height * 1.125f;

        
        float left;
        float right;
        float top;
        float bottom;

        if (mSide == CurveTowards.RIGHT) {
            left = 0;
            right = mFullWidth;
            top = 0;
            bottom = height;
        } else {
            left = width - mFullWidth;
            right = width;
            top = height;
            bottom = 0;
        }

        mLeftCurve.reset();
        mLeftCurve.moveTo(left, top);

        if (mCropped && mSide == CurveTowards.LEFT) {
            mLeftCurve.cubicTo(left + curve, top,
                               left, bottom,
                               left + curve, bottom);
        } else {
            mLeftCurve.cubicTo(left + (curve * 0.75f), top,
                               left + (curve * 0.25f), bottom,
                               left + curve, bottom);
        }

        mRightCurve.reset();
        mRightCurve.moveTo(right, bottom);

        if (mCropped && mSide == CurveTowards.RIGHT) {
            mRightCurve.cubicTo(right - curve, bottom,
                                right, top,
                                right - curve, top);
        } else {
            mRightCurve.cubicTo(right - (curve * 0.75f), bottom,
                                right - (curve * 0.25f), top,
                                right - curve, top);
        }

        mPath.reset();

        
        
        Drawable background = getBackground();

        if (!(background.getCurrent() instanceof ColorDrawable)) {
            if (background.getLevel() == 2) {
                mPath.moveTo(left, top);
                mPath.lineTo(left, bottom);
                mPath.lineTo(right, bottom);
                mPath.addPath(mRightCurve);
                mPath.lineTo(left, top);
            } else {
                mPath.moveTo(left, top);
                mPath.addPath(mLeftCurve);
                mPath.lineTo(right, bottom);
                mPath.addPath(mRightCurve);
                mPath.lineTo(left, top);
            }
        }

        if (mCropped) {
            mBackgroundPath.reset();

            if (mSide == CurveTowards.RIGHT) {
                mBackgroundPath.moveTo(right, bottom);
                mBackgroundPath.addPath(mRightCurve);
                mBackgroundPath.lineTo(right, top);
                mBackgroundPath.lineTo(right, bottom);
            } else {
                mBackgroundPath.moveTo(left, top);
                mBackgroundPath.addPath(mLeftCurve);
                mBackgroundPath.lineTo(left, bottom);
                mBackgroundPath.lineTo(left, top);
            }
        }
    }

    @Override
    public void draw(Canvas canvas) {
        mCanvasDelegate.draw(canvas, mPath, getWidth(), getHeight());

        Drawable background = getBackground();
        if (background.getCurrent() instanceof ColorDrawable)
            return;

        
        if (mCropped && background.getLevel() != 2)
            canvas.drawPath(mBackgroundPath, mPaint);
    }
}
