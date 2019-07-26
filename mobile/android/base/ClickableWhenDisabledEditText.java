




package org.mozilla.gecko;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.EditText;

public class ClickableWhenDisabledEditText extends EditText {
    public ClickableWhenDisabledEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (!isEnabled() && event.getAction() == MotionEvent.ACTION_UP) {
            return performClick();
        }
        return super.onTouchEvent(event);
    }
}
