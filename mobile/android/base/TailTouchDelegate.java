




package org.mozilla.gecko;

import android.graphics.Rect;
import android.view.TouchDelegate;
import android.view.MotionEvent;
import android.view.View;






public class TailTouchDelegate extends TouchDelegate {
    





    public TailTouchDelegate(Rect bounds, View delegateView) {
        super(bounds, delegateView);
    }

    @Override 
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                
                if (!super.onTouchEvent(event)) {
                    MotionEvent cancelEvent = MotionEvent.obtain(event);
                    cancelEvent.setAction(MotionEvent.ACTION_CANCEL);
                    super.onTouchEvent(cancelEvent);
                    return false;
                 } else {
                    return true;
                 }
            default:
                return super.onTouchEvent(event);
        }
    }
}
