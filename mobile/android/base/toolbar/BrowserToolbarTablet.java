




package org.mozilla.gecko.toolbar;

import android.graphics.Rect;
import android.view.TouchDelegate;
import android.view.ViewTreeObserver;
import org.mozilla.gecko.R;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;




class BrowserToolbarTablet extends BrowserToolbarTabletBase {

    private static final int FORWARD_ANIMATION_DURATION = 450;

    private enum ForwardButtonState {
        HIDDEN,
        DISPLAYED,
        TRANSITIONING,
    }

    private final int forwardButtonTranslationWidth;

    private ForwardButtonState forwardButtonState;

    private boolean backButtonWasEnabledOnStartEditing;

    public BrowserToolbarTablet(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        forwardButtonTranslationWidth =
                getResources().getDimensionPixelOffset(R.dimen.new_tablet_nav_button_width);

        
        
        
        ViewHelper.setTranslationX(forwardButton, -forwardButtonTranslationWidth);

        
        
        
        
        setButtonEnabled(forwardButton, true);

        updateForwardButtonState(ForwardButtonState.HIDDEN);

        getViewTreeObserver().addOnPreDrawListener(new ViewTreeObserver.OnPreDrawListener() {
            @Override
            public boolean onPreDraw() {
                
                
                getViewTreeObserver().removeOnPreDrawListener(this);

                final Rect r = new Rect();
                r.left = menuButton.getLeft();
                r.right = getWidth();
                r.top = 0;
                r.bottom = getHeight();

                
                
                setTouchDelegate(new TouchDelegate(r, menuButton));

                return true;
            }
        });
    }

    private void updateForwardButtonState(final ForwardButtonState state) {
        forwardButtonState = state;
        forwardButton.setEnabled(forwardButtonState == ForwardButtonState.DISPLAYED);
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
        final boolean willShowForward = (animation == ForwardButtonAnimation.SHOW);
        if ((forwardButtonState != ForwardButtonState.HIDDEN && willShowForward) ||
                (forwardButtonState != ForwardButtonState.DISPLAYED && !willShowForward)) {
            return;
        }
        updateForwardButtonState(ForwardButtonState.TRANSITIONING);

        
        final PropertyAnimator forwardAnim =
                new PropertyAnimator(isSwitchingTabs ? 10 : FORWARD_ANIMATION_DURATION);

        forwardAnim.addPropertyAnimationListener(new PropertyAnimator.PropertyAnimationListener() {
            @Override
            public void onPropertyAnimationStart() {
                if (!willShowForward) {
                    
                    
                    MarginLayoutParams layoutParams =
                        (MarginLayoutParams) urlDisplayLayout.getLayoutParams();
                    layoutParams.leftMargin = 0;

                    
                    layoutParams = (MarginLayoutParams) urlEditLayout.getLayoutParams();
                    layoutParams.leftMargin = 0;

                    requestLayout();
                    
                    
                    
                }
            }

            @Override
            public void onPropertyAnimationEnd() {
                final ForwardButtonState newForwardButtonState;
                if (willShowForward) {
                    
                    MarginLayoutParams layoutParams =
                        (MarginLayoutParams) urlDisplayLayout.getLayoutParams();
                    layoutParams.leftMargin = forwardButtonTranslationWidth;

                    layoutParams = (MarginLayoutParams) urlEditLayout.getLayoutParams();
                    layoutParams.leftMargin = forwardButtonTranslationWidth;

                    newForwardButtonState = ForwardButtonState.DISPLAYED;
                } else {
                    newForwardButtonState = ForwardButtonState.HIDDEN;
                }

                urlDisplayLayout.finishForwardAnimation();
                updateForwardButtonState(newForwardButtonState);

                requestLayout();
            }
        });

        prepareForwardAnimation(forwardAnim, animation, forwardButtonTranslationWidth);
        forwardAnim.start();
    }

    private void prepareForwardAnimation(PropertyAnimator anim, ForwardButtonAnimation animation, int width) {
        if (animation == ForwardButtonAnimation.HIDE) {
            anim.attach(forwardButton,
                      PropertyAnimator.Property.TRANSLATION_X,
                      -width);
            anim.attach(forwardButton,
                      PropertyAnimator.Property.ALPHA,
                      0);

        } else {
            anim.attach(forwardButton,
                      PropertyAnimator.Property.TRANSLATION_X,
                      0);
            anim.attach(forwardButton,
                      PropertyAnimator.Property.ALPHA,
                      1);
        }

        urlDisplayLayout.prepareForwardAnimation(anim, animation, width);
    }

    @Override
    public void triggerTabsPanelTransition(final PropertyAnimator animator, final boolean areTabsShown) {
        
    }

    @Override
    public void setToolBarButtonsAlpha(float alpha) {
        
    }


    @Override
    public void startEditing(final String url, final PropertyAnimator animator) {
        
        backButtonWasEnabledOnStartEditing = backButton.isEnabled();

        setButtonEnabled(backButton, false);
        setButtonEnabled(forwardButton, false);

        super.startEditing(url, animator);
    }

    @Override
    public String commitEdit() {
        stopEditingNewTablet();
        return super.commitEdit();
    }

    @Override
    public String cancelEdit() {
        
        
        if (isEditing()) {
            stopEditingNewTablet();

            setButtonEnabled(backButton, backButtonWasEnabledOnStartEditing);
            updateForwardButtonState(forwardButtonState);
        }

        return super.cancelEdit();
    }

    private void stopEditingNewTablet() {
        
        
        
        setButtonEnabled(forwardButton, true);
    }

    @Override
    protected Drawable getLWTDefaultStateSetDrawable() {
        return BrowserToolbar.getLightweightThemeDrawable(this, getResources(), getTheme(),
                R.color.background_normal);
    }
}
