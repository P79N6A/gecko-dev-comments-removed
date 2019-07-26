




package org.mozilla.gecko.home;

import java.util.EnumSet;

import org.mozilla.gecko.R;
import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomeConfig.ItemHandler;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PanelLayout.DatasetBacked;
import org.mozilla.gecko.home.PanelLayout.FilterManager;
import org.mozilla.gecko.home.PanelLayout.PanelView;

import android.content.Context;
import android.database.Cursor;
import android.view.View;
import android.widget.AdapterView;
import android.widget.GridView;

public class PanelGridView extends GridView
                           implements DatasetBacked, PanelView {
    private static final String LOGTAG = "GeckoPanelGridView";

    private final ViewConfig mViewConfig;
    private final PanelViewAdapter mAdapter;
    private PanelViewUrlHandler mUrlHandler;

    public PanelGridView(Context context, ViewConfig viewConfig) {
        super(context, null, R.attr.panelGridViewStyle);

        mViewConfig = viewConfig;
        mUrlHandler = new PanelViewUrlHandler(viewConfig);

        mAdapter = new PanelViewAdapter(context, viewConfig.getItemType());
        setAdapter(mAdapter);

        setOnItemClickListener(new PanelGridItemClickListener());
    }

    @Override
    public void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        mUrlHandler.setOnUrlOpenListener(null);
    }

    @Override
    public void setDataset(Cursor cursor) {
        mAdapter.swapCursor(cursor);
    }

    @Override
    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlHandler.setOnUrlOpenListener(listener);
    }

    @Override
    public void setFilterManager(FilterManager filterManager) {
        mAdapter.setFilterManager(filterManager);
        mUrlHandler.setFilterManager(filterManager);
    }

    private class PanelGridItemClickListener implements AdapterView.OnItemClickListener {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            mUrlHandler.openUrlAtPosition(mAdapter.getCursor(), position);
        }
    }
}
