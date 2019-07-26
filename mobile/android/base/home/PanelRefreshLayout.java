




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;

import org.mozilla.gecko.home.PanelLayout.DatasetBacked;
import org.mozilla.gecko.home.PanelLayout.FilterManager;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout;
import org.mozilla.gecko.widget.GeckoSwipeRefreshLayout.OnRefreshListener;

import android.content.Context;
import android.database.Cursor;
import android.view.View;








class PanelRefreshLayout extends GeckoSwipeRefreshLayout implements DatasetBacked {
    private final DatasetBacked datasetBacked;

    



    public PanelRefreshLayout(Context context, View childView) {
        super(context);

        if (!(childView instanceof DatasetBacked)) {
            throw new IllegalArgumentException("View must implement DatasetBacked to be refreshed");
        }

        this.datasetBacked = (DatasetBacked) childView;

        setOnRefreshListener(new RefreshListener());
        addView(childView);

        
        setColorScheme(R.color.swipe_refresh_orange, R.color.swipe_refresh_white,
                       R.color.swipe_refresh_orange, R.color.swipe_refresh_white);
    }

    @Override
    public void setDataset(Cursor cursor) {
        datasetBacked.setDataset(cursor);
    }

    @Override
    public void setFilterManager(FilterManager manager) {
        datasetBacked.setFilterManager(manager);
    }

    private class RefreshListener implements OnRefreshListener {
        @Override
        public void onRefresh() {
            setRefreshing(false);
        }
    }
}
