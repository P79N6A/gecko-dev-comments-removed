




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;










public class RemoteTabsPanel extends HomeFragment {
    @SuppressWarnings("unused")
    private static final String LOGTAG = "GeckoRemoteTabsPanel";

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return inflater.inflate(R.layout.home_remote_tabs_panel, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        loadIfVisible();
    }

    @Override
    protected void load() {
    }
}
