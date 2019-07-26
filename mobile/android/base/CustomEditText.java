




package org.mozilla.gecko;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.view.View;

public class CustomEditText extends GeckoEditText {
    private OnKeyPreImeListener mOnKeyPreImeListener;
    private OnSelectionChangedListener mOnSelectionChangedListener;
    private OnWindowFocusChangeListener mOnWindowFocusChangeListener;

    public CustomEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
        mOnKeyPreImeListener = null;
    }

    public interface OnKeyPreImeListener {
        public boolean onKeyPreIme(View v, int keyCode, KeyEvent event);
    }

    public void setOnKeyPreImeListener(OnKeyPreImeListener listener) {
        mOnKeyPreImeListener = listener;
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (mOnKeyPreImeListener != null)
            return mOnKeyPreImeListener.onKeyPreIme(this, keyCode, event);

        return false;
    }

    public interface OnSelectionChangedListener {
        public void onSelectionChanged(int selStart, int selEnd);
    }

    public void setOnSelectionChangedListener(OnSelectionChangedListener listener) {
        mOnSelectionChangedListener = listener;
    }

    @Override
    protected void onSelectionChanged(int selStart, int selEnd) {
        if (mOnSelectionChangedListener != null)
            mOnSelectionChangedListener.onSelectionChanged(selStart, selEnd);

        super.onSelectionChanged(selStart, selEnd);
    }

    public interface OnWindowFocusChangeListener {
        public void onWindowFocusChanged(boolean hasFocus);
    }

    public void setOnWindowFocusChangeListener(OnWindowFocusChangeListener listener) {
        mOnWindowFocusChangeListener = listener;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (mOnWindowFocusChangeListener != null)
            mOnWindowFocusChangeListener.onWindowFocusChanged(hasFocus);
    }

    @Override
    public void setPrivateMode(boolean isPrivate) {
        super.setPrivateMode(isPrivate);

        
        int colorId = isPrivate ? R.color.url_bar_text_highlight_pb : R.color.url_bar_text_highlight;
        setHighlightColor(getContext().getResources().getColor(colorId));
    }
}
