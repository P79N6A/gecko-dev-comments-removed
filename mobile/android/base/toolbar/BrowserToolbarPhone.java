




package org.mozilla.gecko.toolbar;

import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.PropertyAnimator.PropertyAnimationListener;
import org.mozilla.gecko.util.HardwareUtils;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;




class BrowserToolbarPhone extends BrowserToolbarPhoneBase {

    private final PropertyAnimationListener showEditingListener;
    private final PropertyAnimationListener stopEditingListener;

    protected boolean isAnimatingEntry;

    protected BrowserToolbarPhone(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        
        
        showEditingListener = new PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {  }

            @Override
            public void onPropertyAnimationEnd() {
                isAnimatingEntry = false;
            }
        };

        stopEditingListener = new PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {  }

            @Override
            public void onPropertyAnimationEnd() {
                urlBarTranslatingEdge.setVisibility(View.INVISIBLE);

                final PropertyAnimator buttonsAnimator = new PropertyAnimator(300);
                urlDisplayLayout.prepareStopEditingAnimation(buttonsAnimator);
                buttonsAnimator.start();

                isAnimatingEntry = false;

                
                
                updateTabCountAndAnimate(Tabs.getInstance().getDisplayCount());
            }
        };
    }

    @Override
    public boolean isAnimating() {
        return isAnimatingEntry;
    }

    @Override
    protected void triggerStartEditingTransition(final PropertyAnimator animator) {
        if (isAnimatingEntry) {
            return;
        }

        
        
        urlEditLayout.clearFocus();

        urlDisplayLayout.prepareStartEditingAnimation();
        addAnimationsForEditing(animator, true);
        showUrlEditLayout(animator);
        urlBarTranslatingEdge.setVisibility(View.VISIBLE);
        animator.addPropertyAnimationListener(showEditingListener);

        isAnimatingEntry = true; 
    }

    @Override
    protected void triggerStopEditingTransition() {
        final PropertyAnimator animator = new PropertyAnimator(250);
        animator.setUseHardwareLayer(false);

        addAnimationsForEditing(animator, false);
        hideUrlEditLayout(animator);
        animator.addPropertyAnimationListener(stopEditingListener);

        isAnimatingEntry = true;
        animator.start();
    }

    private void addAnimationsForEditing(final PropertyAnimator animator, final boolean isEditing) {
        final int curveTranslation;
        final int entryTranslation;
        if (isEditing) {
            curveTranslation = getUrlBarCurveTranslation();
            entryTranslation = getUrlBarEntryTranslation();
        } else {
            curveTranslation = 0;
            entryTranslation = 0;
        }

        
        animator.attach(urlBarTranslatingEdge,
                        PropertyAnimator.Property.TRANSLATION_X,
                        entryTranslation);
        animator.attach(tabsButton,
                        PropertyAnimator.Property.TRANSLATION_X,
                        curveTranslation);
        animator.attach(tabsCounter,
                        PropertyAnimator.Property.TRANSLATION_X,
                        curveTranslation);

        if (!HardwareUtils.hasMenuButton()) {
            animator.attach(menuButton,
                            PropertyAnimator.Property.TRANSLATION_X,
                            curveTranslation);

            animator.attach(menuIcon,
                            PropertyAnimator.Property.TRANSLATION_X,
                            curveTranslation);
        }
    }
}
