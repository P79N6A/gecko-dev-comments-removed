




package org.mozilla.gecko.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.text.Layout;
import android.util.AttributeSet;

import org.mozilla.gecko.R;
import org.mozilla.gecko.widget.ThemedTextView;





public class FadedTextView extends ThemedTextView {

    
    private final int mFadeWidth;

    
    private FadedTextGradient mTextGradient;

    public FadedTextView(Context context) {
        this(context, null);
    }

    public FadedTextView(Context context, AttributeSet attrs) {
        super(context, attrs);

        setSingleLine(true);
        setEllipsize(null);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.FadedTextView);
        mFadeWidth = a.getDimensionPixelSize(R.styleable.FadedTextView_fadeWidth, 0);
        a.recycle();
    }

    private int getAvailableWidth() {
        return getWidth() - getCompoundPaddingLeft() - getCompoundPaddingRight();
    }

    private boolean needsEllipsis() {
        final int width = getAvailableWidth();
        if (width <= 0) {
            return false;
        }

        final Layout layout = getLayout();
        return (layout != null && layout.getLineWidth(0) > width);
    }

    private void updateGradientShader() {
        final int color = getCurrentTextColor();
        final int width = getAvailableWidth();

        final boolean needsNewGradient = (mTextGradient == null ||
                                          mTextGradient.getColor() != color ||
                                          mTextGradient.getWidth() != width);

        final boolean needsEllipsis = needsEllipsis();
        if (needsEllipsis && needsNewGradient) {
            mTextGradient = new FadedTextGradient(width, mFadeWidth, color);
        }

        getPaint().setShader(needsEllipsis ? mTextGradient : null);
    }

    @Override
    public void onDraw(Canvas canvas) {
        updateGradientShader();
        super.onDraw(canvas);
    }

    private static class FadedTextGradient extends LinearGradient {
        private final int mWidth;
        private final int mColor;

        public FadedTextGradient(int width, int fadeWidth, int color) {
            super(0, 0, width, 0,
                  new int[] { color, color, 0x0 },
                  new float[] { 0,  ((float) (width - fadeWidth) / (float) width), 1.0f },
                  Shader.TileMode.CLAMP);

            mWidth = width;
            mColor = color;
        }

        public int getWidth() {
            return mWidth;
        }

        public int getColor() {
            return mColor;
        }
    }
}
