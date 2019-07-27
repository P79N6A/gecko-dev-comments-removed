




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.util.AttributeSet;





class BrowserToolbarPreHC extends BrowserToolbarPhoneBase {

    public BrowserToolbarPreHC(final Context context, final AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean isAnimating() {
        return false;
    }

    @Override
    protected void triggerStartEditingTransition(final PropertyAnimator animator) {
        showUrlEditLayout();
        updateEditingState(true);
    }

    @Override
    protected void triggerStopEditingTransition() {
        hideUrlEditLayout();
        updateTabCountAndAnimate(Tabs.getInstance().getDisplayCount());
        updateEditingState(false);
    }

    private void updateEditingState(final boolean isEditing) {
        final int curveTranslation;
        final int entryTranslation;
        if (isEditing) {
            curveTranslation = getUrlBarCurveTranslation();
            entryTranslation = getUrlBarEntryTranslation();
        } else {
            curveTranslation = 0;
            entryTranslation = 0;
        }

        
        tabsButton.setEnabled(!isEditing);

        ViewHelper.setTranslationX(urlBarTranslatingEdge, entryTranslation);
        ViewHelper.setTranslationX(tabsButton, curveTranslation);
        ViewHelper.setTranslationX(tabsCounter, curveTranslation);
        ViewHelper.setTranslationX(actionItemBar, curveTranslation);

        if (!HardwareUtils.hasMenuButton()) {
            
            menuButton.setEnabled(!isEditing);

            ViewHelper.setTranslationX(menuButton, curveTranslation);
            ViewHelper.setTranslationX(menuIcon, curveTranslation);
        }
    }
}
