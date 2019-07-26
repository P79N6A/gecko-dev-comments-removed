




package org.mozilla.gecko;

import android.graphics.Rect;
import android.view.TouchDelegate;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;






public class TailTouchDelegate extends TouchDelegate {
    
    private List<TouchDelegate> mDelegates;

    





    public TailTouchDelegate(Rect bounds, View delegateView) {
        super(bounds, delegateView);
        mDelegates = new ArrayList<TouchDelegate>();
        mDelegates.add(new TouchDelegate(bounds, delegateView));
    }

    





    public void add(Rect bounds, View delegateView) {
        mDelegates.add(new TouchDelegate(bounds, delegateView));
    }

    @Override 
    public boolean onTouchEvent(MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                
                for (TouchDelegate delegate : mDelegates) {
                    if (delegate.onTouchEvent(event))
                        return true;

                    MotionEvent cancelEvent = MotionEvent.obtain(event);
                    cancelEvent.setAction(MotionEvent.ACTION_CANCEL);
                    delegate.onTouchEvent(cancelEvent);
                }
                return false;
            default:
                return super.onTouchEvent(event);
        }
    }
}
