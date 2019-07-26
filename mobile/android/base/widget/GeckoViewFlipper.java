




package org.mozilla.gecko.widget;

import org.mozilla.gecko.animation.ViewHelper;

import android.content.Context;
import android.graphics.Rect;
import android.os.Build;
import android.view.MotionEvent;
import android.widget.ViewFlipper;
import android.util.Log;
import android.util.AttributeSet;




public class GeckoViewFlipper extends ViewFlipper {
    private static final String LOGTAG = "GeckoViewFlipper";
    private Rect mRect = new Rect();

    public GeckoViewFlipper(Context context) {
        super(context);
    }

    public GeckoViewFlipper(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (Build.VERSION.SDK_INT < 11) {
            
            getHitRect(mRect);
            mRect.offset((int) ViewHelper.getTranslationX(this), (int) ViewHelper.getTranslationY(this));

            if (!mRect.contains((int) ev.getX(), (int) ev.getY())) {
                return false;
            }
        }

        return super.dispatchTouchEvent(ev);
    }
}
