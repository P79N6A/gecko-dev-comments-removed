




package org.mozilla.gecko.toolbar;

import java.util.Arrays;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;
import org.mozilla.gecko.Tabs.TabEvents;
import org.mozilla.gecko.animation.PropertyAnimator;
import org.mozilla.gecko.animation.ViewHelper;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.AccelerateInterpolator;
import android.view.animation.Interpolator;
import android.widget.ImageButton;
import android.widget.LinearLayout;





abstract class BrowserToolbarTabletBase extends BrowserToolbar {

    protected enum ForwardButtonAnimation {
        SHOW,
        HIDE
    }

    protected final LinearLayout actionItemBar;

    protected final BackButton backButton;
    private final OnClickListener backButtonOnClickListener;
    private final OnLongClickListener backButtonOnLongClickListener;

    protected final ForwardButton forwardButton;
    private final OnClickListener forwardButtonOnClickListener;
    private final OnLongClickListener forwardButtonOnLongClickListener;

    private final Interpolator buttonsInterpolator = new AccelerateInterpolator();

    protected abstract void animateForwardButton(ForwardButtonAnimation animation);

    public BrowserToolbarTabletBase(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        actionItemBar = (LinearLayout) findViewById(R.id.menu_items);

        backButton = (BackButton) findViewById(R.id.back);
        setButtonEnabled(backButton, false);
        forwardButton = (ForwardButton) findViewById(R.id.forward);
        setButtonEnabled(forwardButton, false);

        backButtonOnClickListener = new BackButtonOnClickListener();
        backButtonOnLongClickListener = new BackButtonOnLongClickListener();
        forwardButtonOnClickListener = new ForwardButtonOnClickListener();
        forwardButtonOnLongClickListener = new ForwardButtonOnLongClickListener();
        setNavigationButtonListeners(true);

        focusOrder.addAll(Arrays.asList(tabsButton, (View) backButton, (View) forwardButton, this));
        focusOrder.addAll(urlDisplayLayout.getFocusOrder());
        focusOrder.addAll(Arrays.asList(actionItemBar, menuButton));
    }

    








    private void setNavigationButtonListeners(final boolean enabled) {
        if (enabled) {
            backButton.setOnClickListener(backButtonOnClickListener);
            backButton.setOnLongClickListener(backButtonOnLongClickListener);

            forwardButton.setOnClickListener(forwardButtonOnClickListener);
            forwardButton.setOnLongClickListener(forwardButtonOnLongClickListener);
        } else {
            backButton.setOnClickListener(null);
            backButton.setOnLongClickListener(null);

            forwardButton.setOnClickListener(null);
            forwardButton.setOnLongClickListener(null);
        }
    }

    @Override
    public void onTabChanged(final Tab tab, final Tabs.TabEvents msg, final Object data) {
        
        
        
        
        
        if (msg == TabEvents.STOP ||
                msg == TabEvents.SELECTED ||
                msg == TabEvents.LOAD_ERROR) {
            setNavigationButtonListeners(true);
        }

        super.onTabChanged(tab, msg, data);
    }

    @Override
    protected boolean isTabsButtonOffscreen() {
        return false;
    }

    @Override
    public boolean addActionItem(final View actionItem) {
        actionItemBar.addView(actionItem);
        return true;
    }

    @Override
    public void removeActionItem(final View actionItem) {
        actionItemBar.removeView(actionItem);
    }

    @Override
    protected void updateNavigationButtons(final Tab tab) {
        setButtonEnabled(backButton, canDoBack(tab));

        final boolean isForwardEnabled = canDoForward(tab);
        if (forwardButton.isEnabled() != isForwardEnabled) {
            
            
            setButtonEnabled(forwardButton, isForwardEnabled);
            animateForwardButton(
                    isForwardEnabled ? ForwardButtonAnimation.SHOW : ForwardButtonAnimation.HIDE);
        }
    }

    @Override
    public void setNextFocusDownId(int nextId) {
        super.setNextFocusDownId(nextId);
        backButton.setNextFocusDownId(nextId);
        forwardButton.setNextFocusDownId(nextId);
    }

    @Override
    public void setPrivateMode(final boolean isPrivate) {
        super.setPrivateMode(isPrivate);
        backButton.setPrivateMode(isPrivate);
        forwardButton.setPrivateMode(isPrivate);
    }

    @Override
    public void triggerTabsPanelTransition(final PropertyAnimator animator, final boolean areTabsShown) {
        if (areTabsShown) {
            ViewHelper.setAlpha(tabsCounter, 0.0f);
            return;
        }

        final PropertyAnimator buttonsAnimator =
                new PropertyAnimator(animator.getDuration(), buttonsInterpolator);

        buttonsAnimator.attach(tabsCounter,
                               PropertyAnimator.Property.ALPHA,
                               1.0f);

        buttonsAnimator.start();
    }

    protected boolean canDoBack(final Tab tab) {
        return (tab.canDoBack() && !isEditing());
    }

    protected boolean canDoForward(final Tab tab) {
        return (tab.canDoForward() && !isEditing());
    }

    protected static void setButtonEnabled(final ImageButton button, final boolean enabled) {
        final Drawable drawable = button.getDrawable();
        if (drawable != null) {
            drawable.setAlpha(enabled ? 255 : 61);
        }

        button.setEnabled(enabled);
    }

    private class BackButtonOnClickListener implements OnClickListener {
        @Override
        public void onClick(final View view) {
            setNavigationButtonListeners(false);
            Tabs.getInstance().getSelectedTab().doBack();
        }
    }

    private class BackButtonOnLongClickListener implements OnLongClickListener {
        @Override
        public boolean onLongClick(final View view) {
            setNavigationButtonListeners(false);
            return Tabs.getInstance().getSelectedTab().showBackHistory();
        }
    }

    private class ForwardButtonOnClickListener implements OnClickListener {
        @Override
        public void onClick(final View view) {
            setNavigationButtonListeners(false);
            Tabs.getInstance().getSelectedTab().doForward();
        }
    }

    private class ForwardButtonOnLongClickListener implements OnLongClickListener {
        @Override
        public boolean onLongClick(final View view) {
            setNavigationButtonListeners(false);
            return Tabs.getInstance().getSelectedTab().showForwardHistory();
        }
    }
}
