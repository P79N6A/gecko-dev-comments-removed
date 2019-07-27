




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.animation.PropertyAnimator;

import android.content.Context;
import android.util.AttributeSet;




class BrowserToolbarTablet extends BrowserToolbarTabletBase {

    public BrowserToolbarTablet(final Context context, final AttributeSet attrs) {
        super(context, attrs);
        
    }

    @Override
    public boolean isAnimating() {
        return false;
    }

    @Override
    protected void triggerStartEditingTransition(final PropertyAnimator animator) {
        
    }

    @Override
    protected void triggerStopEditingTransition() {
        
    }
}
