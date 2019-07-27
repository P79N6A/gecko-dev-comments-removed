




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.animation.PropertyAnimator;

import android.content.Context;
import android.util.AttributeSet;





class BrowserToolbarNewTablet extends BrowserToolbarTabletBase {

    public BrowserToolbarNewTablet(final Context context, final AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean isAnimating() {
        return false;
    }

    @Override
    protected void triggerStartEditingTransition(final PropertyAnimator animator) {
        showUrlEditLayout();
    }

    @Override
    protected void triggerStopEditingTransition() {
        hideUrlEditLayout();
    }

    @Override
    protected void animateForwardButton(final ForwardButtonAnimation animation) {
        
    }
}
