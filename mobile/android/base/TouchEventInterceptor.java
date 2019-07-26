




package org.mozilla.gecko;

import android.view.MotionEvent;
import android.view.View;

public interface TouchEventInterceptor extends View.OnTouchListener {
    
    public boolean onInterceptTouchEvent(View view, MotionEvent event);
}
