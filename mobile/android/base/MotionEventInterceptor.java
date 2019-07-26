




package org.mozilla.gecko;

import android.view.MotionEvent;
import android.view.View;

public interface MotionEventInterceptor {
    public boolean onInterceptMotionEvent(View view, MotionEvent event);
}
