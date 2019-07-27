



package org.mozilla.gecko.tabs;

import org.mozilla.gecko.R;
import org.mozilla.gecko.fxa.FirefoxAccounts;
import org.mozilla.gecko.fxa.login.State;
import org.mozilla.gecko.sync.setup.SyncAccounts;
import org.mozilla.gecko.tabs.TabsPanel.PanelView;

import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;






class RemoteTabsPanel extends FrameLayout implements PanelView {
    private enum RemotePanelType {
        SETUP,
        VERIFICATION,
        CONTAINER
    }

    private PanelView currentPanel;
    private RemotePanelType currentPanelType;

    private TabsPanel tabsPanel;

    public RemoteTabsPanel(Context context, AttributeSet attrs) {
        super(context, attrs);
        updateCurrentPanel();
    }

    @Override
    public void setTabsPanel(TabsPanel panel) {
        tabsPanel = panel;
        currentPanel.setTabsPanel(panel);
    }

    @Override
    public void show() {
        updateCurrentPanel();
        currentPanel.show();
        setVisibility(View.VISIBLE);
    }

    @Override
    public void hide() {
        setVisibility(View.GONE);
        currentPanel.hide();
    }

    @Override
    public boolean shouldExpand() {
        return currentPanel.shouldExpand();
    }

    private void updateCurrentPanel() {
        final RemotePanelType newPanelType = getPanelTypeFromAccountState();
        if (newPanelType != currentPanelType) {
            
            if (currentPanel != null) {
                currentPanel.hide();
            }
            removeAllViews();

            currentPanelType = newPanelType;
            currentPanel = inflatePanel(currentPanelType);
            currentPanel.setTabsPanel(tabsPanel);
            addView((View) currentPanel);
        }
    }

    private RemotePanelType getPanelTypeFromAccountState() {
        final Context context = getContext();
        final State accountState = FirefoxAccounts.getFirefoxAccountState(context);
        if (accountState == null) {
            
            
            if (SyncAccounts.syncAccountsExist(context)) {
                return RemotePanelType.CONTAINER;
            } else {
                return RemotePanelType.SETUP;
            }
        }

        if (accountState.getNeededAction() == State.Action.NeedsVerification) {
            return RemotePanelType.VERIFICATION;
        }

        return RemotePanelType.CONTAINER;
    }

    private PanelView inflatePanel(final RemotePanelType panelType) {
        final PanelView view;
        switch (panelType) {
            case SETUP:
                view = new RemoteTabsSetupPanel(getContext());
                break;

            case VERIFICATION:
                view = new RemoteTabsVerificationPanel(getContext());
                break;

            case CONTAINER:
                final LayoutInflater inflater = LayoutInflater.from(getContext());
                view = (PanelView) inflater.inflate(R.layout.remote_tabs_container_panel, null);
                break;

            default:
                throw new IllegalArgumentException("Unknown panelType, " + panelType);
        }

        return view;
    }
}
