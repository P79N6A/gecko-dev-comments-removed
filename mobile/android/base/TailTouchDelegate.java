




package org.mozilla.gecko;

import android.graphics.Rect;
import android.view.TouchDelegate;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;






public class TailTouchDelegate extends TouchDelegate {
    
    private TouchDelegate mDelegate;

    
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
                    if (delegate.onTouchEvent(event)) {
                        mDelegate = delegate;
                        return true;
                    }

                    MotionEvent cancelEvent = MotionEvent.obtain(event);
                    cancelEvent.setAction(MotionEvent.ACTION_CANCEL);
                    delegate.onTouchEvent(cancelEvent);
                    mDelegate = null;
                }
                return false;
            default:
                if (mDelegate != null)
                    return mDelegate.onTouchEvent(event);
                else
                    return false;
        }
    }
}
