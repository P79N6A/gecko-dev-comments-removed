




package org.mozilla.gecko.home;

import org.mozilla.gecko.R;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;
import org.mozilla.gecko.home.PanelLayout.DatasetBacked;

import android.content.Context;
import android.database.Cursor;
import android.support.v4.widget.CursorAdapter;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

public class PanelListView extends HomeListView
                           implements DatasetBacked {

    private static final String LOGTAG = "GeckoPanelListView";

    private final PanelListAdapter mAdapter;
    private final ViewConfig mViewConfig;

    public PanelListView(Context context, ViewConfig viewConfig) {
        super(context);
        mViewConfig = viewConfig;
        mAdapter = new PanelListAdapter(context);
        setAdapter(mAdapter);
    }

    @Override
    public void setDataset(Cursor cursor) {
        Log.d(LOGTAG, "Setting dataset: " + mViewConfig.getDatasetId());
        mAdapter.swapCursor(cursor);
    }

    private class PanelListAdapter extends CursorAdapter {
        public PanelListAdapter(Context context) {
            super(context, null, 0);
        }

        @Override
        public void bindView(View view, Context context, Cursor cursor) {
            final PanelListRow row = (PanelListRow) view;
            row.updateFromCursor(cursor);
        }

        @Override
        public View newView(Context context, Cursor cursor, ViewGroup parent) {
            return LayoutInflater.from(parent.getContext()).inflate(R.layout.panel_list_row, parent, false);
        }
    }
}