




package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Shader;
import android.util.AttributeSet;










public class FadedMultiColorTextView extends FadedTextView {
    private final ColorStateList fadeBackgroundColorList;

    private final Paint fadePaint;
    private FadedTextGradient backgroundGradient;

    public FadedMultiColorTextView(Context context, AttributeSet attrs) {
        super(context, attrs);

        fadePaint = new Paint();

        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.FadedMultiColorTextView);
        fadeBackgroundColorList =
                a.getColorStateList(R.styleable.FadedMultiColorTextView_fadeBackgroundColor);
        a.recycle();
    }

    @Override
    public void onDraw(Canvas canvas) {
        super.onDraw(canvas);

        final boolean needsEllipsis = needsEllipsis();
        if (needsEllipsis) {
            final int right = getWidth() - getCompoundPaddingRight();
            final float left = right - fadeWidth;

            updateGradientShader(needsEllipsis, right);

            final float center = getHeight() / 2;
            final float top = center - getTextSize();
            final float bottom = center + getTextSize();

            canvas.drawRect(left, top, right, bottom, fadePaint);
        }
    }

    private void updateGradientShader(final boolean needsEllipsis, final int gradientEndRight) {
        final int backgroundColor =
                fadeBackgroundColorList.getColorForState(getDrawableState(), Color.RED);

        final boolean needsNewGradient = (backgroundGradient == null ||
                                          backgroundGradient.getBackgroundColor() != backgroundColor ||
                                          backgroundGradient.getEndRight() != gradientEndRight);

        if (needsEllipsis && needsNewGradient) {
            backgroundGradient = new FadedTextGradient(gradientEndRight, fadeWidth, backgroundColor);
            fadePaint.setShader(backgroundGradient);
        }
    }

    private static class FadedTextGradient extends LinearGradient {
        private final int endRight;
        private final int backgroundColor;

        public FadedTextGradient(final int gradientEndRight, final int fadeWidth,
                final int backgroundColor) {
            super(gradientEndRight - fadeWidth, 0, gradientEndRight, 0,
                  getColorWithZeroedAlpha(backgroundColor), backgroundColor, Shader.TileMode.CLAMP);

            this.endRight = gradientEndRight;
            this.backgroundColor = backgroundColor;
        }

        private static int getColorWithZeroedAlpha(final int color) {
            return Color.argb(0, Color.red(color), Color.green(color), Color.blue(color));
        }

        public int getEndRight() {
            return endRight;
        }

        public int getBackgroundColor() {
            return backgroundColor;
        }
    }
}
