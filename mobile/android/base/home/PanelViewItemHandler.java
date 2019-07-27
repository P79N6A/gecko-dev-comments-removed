




package org.mozilla.gecko.home;

import org.mozilla.gecko.db.BrowserContract.HomeItems;
import org.mozilla.gecko.home.HomeConfig.ViewConfig;
import org.mozilla.gecko.home.HomePager.OnUrlOpenListener;
import org.mozilla.gecko.home.PanelLayout.FilterManager;
import org.mozilla.gecko.home.PanelLayout.OnItemOpenListener;

import android.database.Cursor;

import java.util.EnumSet;

class PanelViewItemHandler {
    private OnItemOpenListener mItemOpenListener;
    private FilterManager mFilterManager;

    public void setOnItemOpenListener(OnItemOpenListener listener) {
        mItemOpenListener = listener;
    }

    public void setFilterManager(FilterManager manager) {
        mFilterManager = manager;
    }

    




    public void openItemAtPosition(Cursor cursor, int position) {
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

        int titleIndex = cursor.getColumnIndexOrThrow(HomeItems.TITLE);
        final String title = cursor.getString(titleIndex);

        if (mItemOpenListener != null) {
            mItemOpenListener.onItemOpen(url, title);
        }
    }
}
