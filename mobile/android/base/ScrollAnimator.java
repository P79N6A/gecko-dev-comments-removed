




package org.mozilla.gecko;

import java.util.Timer;
import java.util.TimerTask;

import org.mozilla.gecko.util.GamepadUtils;

import android.os.Build;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.View;

public class ScrollAnimator implements View.OnGenericMotionListener {
    private Timer mScrollTimer;
    private int mX;
    private int mY;

    
    static final long MS_PER_FRAME = 1000 / 60;

    
    static final float MAX_SCROLL = 0.075f * GeckoAppShell.getDpi();

    private class ScrollRunnable extends TimerTask {
        private View mView;

        public ScrollRunnable(View view) {
            mView = view;
        }

        @Override
        public final void run() {
            mView.scrollBy(mX, mY);
        }
    }

    @Override
    public boolean onGenericMotion(View view, MotionEvent event) {
        
        if (Build.VERSION.SDK_INT >= 12 &&
                (event.getSource() & InputDevice.SOURCE_CLASS_JOYSTICK) != 0) {
            switch (event.getAction()) {
            case MotionEvent.ACTION_MOVE:
                
                if (GamepadUtils.isValueInDeadZone(event, MotionEvent.AXIS_X) &&
                        GamepadUtils.isValueInDeadZone(event, MotionEvent.AXIS_Y)) {
                    if (mScrollTimer != null) {
                        mScrollTimer.cancel();
                        mScrollTimer = null;
                    }
                    return true;
                }

                
                mX = (int) (event.getAxisValue(MotionEvent.AXIS_X) * MAX_SCROLL);
                mY = (int) (event.getAxisValue(MotionEvent.AXIS_Y) * MAX_SCROLL);

                
                
                if (mScrollTimer == null) {
                    mScrollTimer = new Timer();
                    ScrollRunnable task = new ScrollRunnable(view);
                    mScrollTimer.scheduleAtFixedRate(task, 0, MS_PER_FRAME);
                }

                return true;
            }
        }

        return false;
    }

    


    public void cancel() {
        if (mScrollTimer != null) {
            mScrollTimer.cancel();
            mScrollTimer = null;
        }
    }
}
