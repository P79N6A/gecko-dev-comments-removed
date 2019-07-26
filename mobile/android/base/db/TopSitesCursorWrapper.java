package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.database.CharArrayBuffer;
import android.database.ContentObserver;
import android.database.Cursor;
import android.database.DataSetObserver;
import android.net.Uri;
import android.os.Bundle;
import android.util.SparseArray;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB.URLColumns;







public class TopSitesCursorWrapper implements Cursor {

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
        } else if (!topCursor.isBeforeFirst() && !topCursor.isAfterLast()) {
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
    public boolean isFirst() {
        return (currentPosition == 0);
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
    public boolean moveToFirst() {
        return moveToPosition(0);
    }

    @Override
    public boolean moveToLast() {
        return moveToPosition(count - 1);
    }

    @Override
    public boolean move(int offset) {
        return moveToPosition(currentPosition + offset);
    }

    @Override
    public boolean moveToPosition(int position) {
        currentPosition = position;

        
        
        
        final int before = getPinnedBefore(position);
        final int p2 = position - before;

        if (p2 <= -1) {
            topCursor.moveToPosition(-1);
        } else if (p2 >= topCursor.getCount()) {
            topCursor.moveToPosition(topCursor.getCount());
        } else {
            topCursor.moveToPosition(p2);
        }

        updateRowType();

        return (!isBeforeFirst() && !isAfterLast());
    }

    @Override
    public long getLong(int columnIndex) {
        if (currentRowType == RowType.TOP) {
            return topCursor.getLong(columnIndex);
        }

        return 0;
    }

    @Override
    public int getInt(int columnIndex) {
        if (currentRowType == RowType.TOP) {
            return topCursor.getInt(columnIndex);
        }

        return 0;
    }

    @Override
    public String getString(int columnIndex) {
        switch (currentRowType) {
            case TOP:
                return topCursor.getString(columnIndex);

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

    @Override
    public float getFloat(int columnIndex) {
        throw new UnsupportedOperationException();
    }

    @Override
    public double getDouble(int columnIndex) {
        throw new UnsupportedOperationException();
    }

    @Override
    public short getShort(int columnIndex) {
        throw new UnsupportedOperationException();
    }

    @Override
    public byte[] getBlob(int columnIndex) {
        throw new UnsupportedOperationException();
    }

    @Override
    public void copyStringToBuffer(int columnIndex, CharArrayBuffer buffer) {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isNull(int columnIndex) {
        switch (currentRowType) {
            case TOP:
                return topCursor.isNull(columnIndex);

            case PINNED:
                if (columnIndex == topCursor.getColumnIndex(URLColumns.URL) ||
                    columnIndex == topCursor.getColumnIndex(URLColumns.TITLE)) {
                    return false;
                }
                break;
        }

        return true;
    }

    @Override
    public int getType(int columnIndex) {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getColumnCount() {
        throw new UnsupportedOperationException();
    }

    @Override
    public int getColumnIndex(String columnName) {
        return topCursor.getColumnIndex(columnName);
    }

    @Override
    public int getColumnIndexOrThrow(String columnName)
            throws IllegalArgumentException {
        return topCursor.getColumnIndexOrThrow(columnName);
    }

    @Override
    public String getColumnName(int columnIndex) {
        throw new UnsupportedOperationException();
    }

    @Override
    public String[] getColumnNames() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean requery() {
        return topCursor.requery();
    }

    @Override
    public Bundle respond(Bundle extras) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Bundle getExtras() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean getWantsAllOnMoveCalls() {
        return false;
    }

    @Override
    public void setNotificationUri(ContentResolver cr, Uri uri) {
        
        
        
    }

    @Override
    public void registerContentObserver(ContentObserver observer) {
        topCursor.registerContentObserver(observer);
    }

    @Override
    public void unregisterContentObserver(ContentObserver observer) {
        topCursor.unregisterContentObserver(observer);
    }

    @Override
    public void registerDataSetObserver(DataSetObserver observer) {
        topCursor.registerDataSetObserver(observer);
    }

    @Override
    public void unregisterDataSetObserver(DataSetObserver observer) {
        topCursor.unregisterDataSetObserver(observer);
    }

    @Override
    public void deactivate() {
        topCursor.deactivate();
    }

    @Override
    public boolean isClosed() {
        return topCursor.isClosed();
    }

    @Override
    public void close() {
        topCursor.close();
        pinnedSites = null;
    }
}
