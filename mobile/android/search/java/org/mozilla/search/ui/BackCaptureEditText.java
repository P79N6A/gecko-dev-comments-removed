



package org.mozilla.search.ui;

import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.EditText;





public class BackCaptureEditText extends EditText {
    public BackCaptureEditText(Context context) {
        super(context);
    }

    public BackCaptureEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public BackCaptureEditText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_UP && keyCode == KeyEvent.KEYCODE_BACK) {
            clearFocus();
        }
        return super.onKeyPreIme(keyCode, event);
    }
}
