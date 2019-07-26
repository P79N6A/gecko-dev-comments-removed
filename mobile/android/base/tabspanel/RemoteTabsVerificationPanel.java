



package org.mozilla.gecko.tabspanel;

import org.mozilla.gecko.R;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.tabspanel.TabsPanel.PanelView;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;






class RemoteTabsVerificationPanel extends LinearLayout implements PanelView {
    private static final String LOG_TAG = RemoteTabsVerificationPanel.class.getSimpleName();

    private TabsPanel tabsPanel;

    public RemoteTabsVerificationPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        final View resendLink = findViewById(R.id.remote_tabs_confirm_resend);
        resendLink.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                final State accountState = FirefoxAccounts.getFirefoxAccountState(getContext());
                final State.Action neededAction = accountState.getNeededAction();
                if (accountState.getNeededAction() != State.Action.NeedsVerification) {
                    autoHideTabsPanelOnUnexpectedState("Account expected to need verification " +
                            "on resend, but action was " + neededAction + " instead.");
                    return;
                }

                if (!FirefoxAccounts.resendVerificationEmail(getContext())) {
                    autoHideTabsPanelOnUnexpectedState("Account DNE when resending verification email.");
                    return;
                }
            }
        });
    }

    private void refresh() {
        final TextView verificationView =
                (TextView) findViewById(R.id.remote_tabs_confirm_verification);
        final String email = FirefoxAccounts.getFirefoxAccountEmail(getContext());
        if (email == null) {
            autoHideTabsPanelOnUnexpectedState("Account email DNE on View refresh.");
            return;
        }

        final String text = getResources().getString(
                R.string.fxaccount_confirm_account_verification_link, email);
        verificationView.setText(text);
    }

    













    private void autoHideTabsPanelOnUnexpectedState(final String log) {
        Log.w(LOG_TAG, "Unexpected state: " + log + " Closing the tabs panel.");

        if (tabsPanel != null) {
            tabsPanel.autoHidePanel();
        }
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        tabsPanel = panel;
    }

    @Override
    public void show() {
        refresh();
        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
    }

    @Override
    public boolean shouldExpand() {
        return getOrientation() == LinearLayout.VERTICAL;
    }
}
