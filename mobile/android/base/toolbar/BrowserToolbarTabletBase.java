




package org.mozilla.gecko.toolbar;

import java.util.Arrays;

import org.mozilla.gecko.R;
import org.mozilla.gecko.Tab;
import org.mozilla.gecko.Tabs;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Button;
import android.widget.ImageButton;





abstract class BrowserToolbarTabletBase extends BrowserToolbar {

    protected enum ForwardButtonAnimation {
        SHOW,
        HIDE
    }

    protected final BackButton backButton;
    protected final ForwardButton forwardButton;

    protected abstract void animateForwardButton(ForwardButtonAnimation animation);

    public BrowserToolbarTabletBase(final Context context, final AttributeSet attrs) {
        super(context, attrs);

        backButton = (BackButton) findViewById(R.id.back);
        setButtonEnabled(backButton, false);
        forwardButton = (ForwardButton) findViewById(R.id.forward);
        setButtonEnabled(forwardButton, false);
        initButtonListeners();

        focusOrder.addAll(Arrays.asList(tabsButton, (View) backButton, (View) forwardButton, this));
        focusOrder.addAll(urlDisplayLayout.getFocusOrder());
        focusOrder.addAll(Arrays.asList(actionItemBar, menuButton));
    }

    private void initButtonListeners() {
        backButton.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                Tabs.getInstance().getSelectedTab().doBack();
            }
        });
        backButton.setOnLongClickListener(new Button.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                return Tabs.getInstance().getSelectedTab().showBackHistory();
            }
        });

        forwardButton.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View view) {
                Tabs.getInstance().getSelectedTab().doForward();
            }
        });
        forwardButton.setOnLongClickListener(new Button.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                return Tabs.getInstance().getSelectedTab().showForwardHistory();
            }
        });
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
}
