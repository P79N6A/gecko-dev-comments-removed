




package org.mozilla.gecko.widget;

import org.mozilla.gecko.R;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.widget.TextView;




public class EllipsisTextView extends TextView {
    private final String ellipsis;

    private int maxLines;
    private CharSequence originalText;

    public EllipsisTextView(Context context) {
        this(context, null);
    }

    public EllipsisTextView(Context context, AttributeSet attrs) {
        this(context, attrs, android.R.attr.textViewStyle);
    }

    public EllipsisTextView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        ellipsis = getResources().getString(R.string.ellipsis);

        TypedArray a = context.getTheme()
            .obtainStyledAttributes(attrs, R.styleable.EllipsisTextView, 0, 0);
        maxLines = a.getInteger(R.styleable.EllipsisTextView_ellipsizeAtLine, 1);
        a.recycle();
    }

    public void setOriginalText(CharSequence text) {
        originalText = text;
        setText(text);
    }

    @Override
    public void onLayout(boolean changed, int left, int top, int right, int bottom) {
        super.onLayout(changed, left, top, right, bottom);

        
        if (getLineCount() < maxLines) {
            setText(originalText);
        }

        
        if (getLineCount() > maxLines) {
            final int endIndex = getLayout().getLineEnd(maxLines - 1) - 1 - ellipsis.length();
            final String text = getText().subSequence(0, endIndex) + ellipsis;
            
            setText(text);
        }
    }
}
