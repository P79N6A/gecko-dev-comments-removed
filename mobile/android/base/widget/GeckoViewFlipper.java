




package org.mozilla.gecko.widget;

import org.mozilla.gecko.AppConstants.Versions;
import org.mozilla.gecko.animation.ViewHelper;

import android.content.Context;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.widget.ViewFlipper;




public class GeckoViewFlipper extends ViewFlipper {
    private Rect mRect = new Rect();

    public GeckoViewFlipper(Context context) {
        super(context);
    }

    public GeckoViewFlipper(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean dispatchTouchEvent(MotionEvent ev) {
        if (Versions.preHC) {
            
            getHitRect(mRect);
            mRect.offset((int) ViewHelper.getTranslationX(this), (int) ViewHelper.getTranslationY(this));

            if (!mRect.contains((int) ev.getX(), (int) ev.getY())) {
                return false;
            }
        }

        return super.dispatchTouchEvent(ev);
    }
}
