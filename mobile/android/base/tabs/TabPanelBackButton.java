



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.ImageButton;

public class TabPanelBackButton extends ImageButton {

    private int dividerWidth = 0;

    private final Drawable divider;
    private final int dividerPadding;

    public TabPanelBackButton(Context context, AttributeSet attrs) {
        super(context, attrs);

        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.TabPanelBackButton);
        divider = a.getDrawable(R.styleable.TabPanelBackButton_rightDivider);
        dividerPadding = (int) a.getDimension(R.styleable.TabPanelBackButton_dividerVerticalPadding, 0);
        a.recycle();

        if (divider != null) {
            dividerWidth = divider.getIntrinsicWidth();
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        setMeasuredDimension(getMeasuredWidth() + dividerWidth, getMeasuredHeight());
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        if (divider != null) {
            final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) getLayoutParams();
            final int left = getRight() - lp.rightMargin - dividerWidth;

            divider.setBounds(left, getPaddingTop() + dividerPadding,
                    left + dividerWidth, getHeight() - getPaddingBottom() - dividerPadding);
            divider.draw(canvas);
        }
    }
}
