




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomeConfig.ItemHandler;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PanelLayout.FilterManager;

import android.database.Cursor;

import java.util.EnumSet;

class PanelViewUrlHandler {
    private final ViewConfig mViewConfig;
    private OnUrlOpenListener mUrlOpenListener;
    private FilterManager mFilterManager;

    public PanelViewUrlHandler(ViewConfig viewConfig) {
        mViewConfig = viewConfig;
    }

    public void setOnUrlOpenListener(OnUrlOpenListener listener) {
        mUrlOpenListener = listener;
    }

    public void setFilterManager(FilterManager manager) {
        mFilterManager = manager;
    }

    




    public void openUrlAtPosition(Cursor cursor, int position) {
        if (mFilterManager != null && mFilterManager.canGoBack()) {
            if (position == 0) {
                mFilterManager.goBack();
                return;
            }

            position--;
        }

        if (cursor == null || !cursor.moveToPosition(position)) {
            throw new IllegalStateException("Couldn't move cursor to position " + position);
        }

        int urlIndex = cursor.getColumnIndexOrThrow(HomeItems.URL);
        final String url = cursor.getString(urlIndex);

        EnumSet<OnUrlOpenListener.Flags> flags = EnumSet.noneOf(OnUrlOpenListener.Flags.class);
        if (mViewConfig.getItemHandler() == ItemHandler.INTENT) {
            flags.add(OnUrlOpenListener.Flags.OPEN_WITH_INTENT);
        }

        if (mUrlOpenListener != null) {
            mUrlOpenListener.onUrlOpen(url, flags);
        }
    }
}
