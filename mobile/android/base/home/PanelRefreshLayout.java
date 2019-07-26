




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.home.PanelLayout.DatasetBacked;
import org.mozilla.gecko.home.PanelLayout.FilterManager;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout.OnRefreshListener;

import org.json.JSONException;
import org.json.JSONObject;

import android.content.Context;
import android.database.Cursor;
import android.util.Log;
import android.view.View;








class PanelRefreshLayout extends GeckoSwipeRefreshLayout implements DatasetBacked {
    private static final String LOGTAG = "GeckoPanelRefreshLayout";

    private static final String JSON_KEY_PANEL_ID = "panelId";
    private static final String JSON_KEY_VIEW_INDEX = "viewIndex";

    private final String panelId;
    private final int viewIndex;
    private final DatasetBacked datasetBacked;

    





    public PanelRefreshLayout(Context context, View childView, String panelId, int viewIndex) {
        super(context);

        if (!(childView instanceof DatasetBacked)) {
            throw new IllegalArgumentException("View must implement DatasetBacked to be refreshed");
        }

        this.panelId = panelId;
        this.viewIndex = viewIndex;
        this.datasetBacked = (DatasetBacked) childView;

        setOnRefreshListener(new RefreshListener());
        addView(childView);

        
        setColorScheme(R.color.swipe_refresh_orange, R.color.swipe_refresh_white,
                       R.color.swipe_refresh_orange, R.color.swipe_refresh_white);
    }

    @Override
    public void setDataset(Cursor cursor) {
        datasetBacked.setDataset(cursor);
        setRefreshing(false);
    }

    @Override
    public void setFilterManager(FilterManager manager) {
        datasetBacked.setFilterManager(manager);
    }

    private class RefreshListener implements OnRefreshListener {
        @Override
        public void onRefresh() {
            final JSONObject response = new JSONObject();
            try {
                response.put(JSON_KEY_PANEL_ID, panelId);
                response.put(JSON_KEY_VIEW_INDEX, viewIndex);
            } catch (JSONException e) {
                Log.e(LOGTAG, "Could not create refresh message", e);
                return;
            }

            final GeckoEvent event =
                GeckoEvent.createBroadcastEvent("HomePanels:RefreshView", response.toString());
            GeckoAppShell.sendEventToGecko(event);
        }
    }
}
