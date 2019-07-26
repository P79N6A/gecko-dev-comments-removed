package org.mozilla.gecko.db;

import android.database.Cursor;
import android.database.CursorWrapper;
import android.util.SparseArray;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB.URLColumns;







public class TopSitesCursorWrapper extends CursorWrapper {

    private static class PinnedSite {
        public final String title;
        public final String url;

        public PinnedSite(String title, String url) {
            this.title = (title == null ? "" : title);
            this.url = (url == null ? "" : url);
        }
    }

    private enum RowType {
        UNKNOWN,
        TOP,
        PINNED
    }

    
    private RowType currentRowType;

    
    private final Cursor topCursor;

    
    private SparseArray<PinnedSite> pinnedSites;

    
    private int currentPosition = -1;

    
    private final int count;

    public TopSitesCursorWrapper(Cursor pinnedCursor, Cursor topCursor, int minSize) {
        super(topCursor);

        currentRowType = RowType.UNKNOWN;

        setPinnedSites(pinnedCursor);
        this.topCursor = topCursor;

        count = Math.max(minSize, pinnedSites.size() + topCursor.getCount());
    }

    public void setPinnedSites(Cursor c) {
        pinnedSites = new SparseArray<PinnedSite>();

        if (c == null) {
            return;
        }

        try {
            if (c.getCount() <= 0) {
                return;
            }

            c.moveToPosition(0);
            do {
                final int pos = c.getInt(c.getColumnIndex(Bookmarks.POSITION));
                final String url = c.getString(c.getColumnIndex(URLColumns.URL));
                final String title = c.getString(c.getColumnIndex(URLColumns.TITLE));
                pinnedSites.put(pos, new PinnedSite(title, url));
            } while (c.moveToNext());
        } finally {
            c.close();
        }
    }

    public boolean isPinned() {
        return (currentRowType == RowType.PINNED);
    }

    private void updateRowType() {
        if (pinnedSites.get(currentPosition) != null) {
            currentRowType = RowType.PINNED;
        } else if (!super.isBeforeFirst() && !super.isAfterLast()) {
            currentRowType = RowType.TOP;
        } else {
            currentRowType = RowType.UNKNOWN;
        }
    }

    private int getPinnedBefore(int position) {
        int numFound = 0;
        for (int i = 0; i < position; i++) {
            if (pinnedSites.get(i) != null) {
                numFound++;
            }
        }

        return numFound;
    }

    @Override
    public int getPosition() {
        return currentPosition;
    }

    @Override
    public int getCount() {
        return count;
    }

    @Override
    public boolean isAfterLast() {
        return (currentPosition >= count);
    }

    @Override
    public boolean isBeforeFirst() {
        return (currentPosition < 0);
    }

    @Override
    public boolean isLast() {
        return (currentPosition == count - 1);
    }

    @Override
    public boolean moveToNext() {
        return moveToPosition(currentPosition + 1);
    }

    @Override
    public boolean moveToPrevious() {
        return moveToPosition(currentPosition - 1);
    }

    @Override
    public boolean move(int offset) {
        return moveToPosition(currentPosition + offset);
    }

    @Override
    public boolean moveToFirst() {
        return moveToPosition(0);
    }

    @Override
    public boolean moveToLast() {
        return moveToPosition(count - 1);
    }

    @Override
    public boolean moveToPosition(int position) {
        currentPosition = position;

        
        
        
        final int before = getPinnedBefore(position);
        final int p2 = position - before;

        if (p2 <= -1) {
            super.moveToPosition(-1);
        } else if (p2 >= topCursor.getCount()) {
            super.moveToPosition(topCursor.getCount());
        } else {
            super.moveToPosition(p2);
        }

        updateRowType();

        return (!isBeforeFirst() && !isAfterLast());
    }

    @Override
    public long getLong(int columnIndex) {
        if (currentRowType == RowType.TOP) {
            return super.getLong(columnIndex);
        }

        return 0;
    }

    @Override
    public int getInt(int columnIndex) {
        if (currentRowType == RowType.TOP) {
            return super.getInt(columnIndex);
        }

        return 0;
    }

    @Override
    public String getString(int columnIndex) {
        switch (currentRowType) {
            case TOP:
                return super.getString(columnIndex);

            case PINNED:
                final PinnedSite site = pinnedSites.get(currentPosition);
                if (columnIndex == topCursor.getColumnIndex(URLColumns.URL)) {
                    return site.url;
                } else if (columnIndex == topCursor.getColumnIndex(URLColumns.TITLE)) {
                    return site.title;
                }
                break;
        }

        return "";
    }
}
