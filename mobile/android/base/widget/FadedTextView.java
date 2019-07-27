




package org.mozilla.gecko.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.text.Layout;
import android.util.AttributeSet;

import org.mozilla.gecko.R;





public abstract class FadedTextView extends ThemedTextView {
    
    protected final int fadeWidth;

    public FadedTextView(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        setSingleLine(true);
        setEllipsize(null);

        final TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.FadedTextView);
        fadeWidth = a.getDimensionPixelSize(R.styleable.FadedTextView_fadeWidth, 0);
        a.recycle();
    }

    protected int getAvailableWidth() {
        return getWidth() - getCompoundPaddingLeft() - getCompoundPaddingRight();
    }

    protected boolean needsEllipsis() {
        final int width = getAvailableWidth();
        if (width <= 0) {
            return false;
        }

        final Layout layout = getLayout();
        return (layout != null && layout.getLineWidth(0) > width);
    }
}
