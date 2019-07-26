package org.mozilla.gecko.db;

import android.content.ContentResolver;
import android.database.CharArrayBuffer;
import android.database.ContentObserver;
import android.database.Cursor;
import android.database.DataSetObserver;
import android.net.Uri;
import android.os.Bundle;
import android.util.SparseBooleanArray;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserDB.URLColumns;







public class TopSitesCursorWrapper implements Cursor {

    private enum RowType {
        UNKNOWN,
        TOP,
        PINNED
    }

    
    private RowType currentRowType;

    
    private final Cursor topCursor;

    
    private Cursor pinnedCursor;

    
    private SparseBooleanArray pinnedPositions;

    
    private int currentPosition = -1;

    
    private int count;

    
    private final int minSize;

    public TopSitesCursorWrapper(Cursor pinnedCursor, Cursor topCursor, int minSize) {
        currentRowType = RowType.UNKNOWN;

        this.minSize = minSize;
        this.topCursor = topCursor;
        this.pinnedCursor = pinnedCursor;

        updatePinnedPositions();
        updateCount();
    }

    private void updatePinnedPositions() {
        if (pinnedPositions == null) {
            pinnedPositions = new SparseBooleanArray();
        } else {
            pinnedPositions.clear();
        }

        pinnedCursor.moveToPosition(-1);
        while (pinnedCursor.moveToNext()) {
            int pos = pinnedCursor.getInt(pinnedCursor.getColumnIndex(Bookmarks.POSITION));
            pinnedPositions.put(pos, true);
        };
    }

    private void updateCount() {
        count = Math.max(minSize, pinnedCursor.getCount() + topCursor.getCount());
    }

    public boolean isPinned() {
        return (currentRowType == RowType.PINNED);
    }

    private void updateRowType() {
        if (!pinnedCursor.isBeforeFirst() && !pinnedCursor.isAfterLast()) {
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
            if (pinnedPositions.get(i)) {
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

        if (pinnedPositions.get(position)) {
            pinnedCursor.moveToPosition(pinnedPositions.indexOfKey(position));
        } else {
            pinnedCursor.moveToPosition(-1);
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
                if (columnIndex == topCursor.getColumnIndex(URLColumns.URL)) {
                    return pinnedCursor.getString(pinnedCursor.getColumnIndex(URLColumns.URL));
                } else if (columnIndex == topCursor.getColumnIndex(URLColumns.TITLE)) {
                    return pinnedCursor.getString(pinnedCursor.getColumnIndex(URLColumns.TITLE));
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
        boolean result = topCursor.requery() && pinnedCursor.requery();

        updatePinnedPositions();
        updateCount();

        return result;
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
        pinnedCursor.registerContentObserver(observer);
    }

    @Override
    public void unregisterContentObserver(ContentObserver observer) {
        topCursor.unregisterContentObserver(observer);
        pinnedCursor.unregisterContentObserver(observer);
    }

    @Override
    public void registerDataSetObserver(DataSetObserver observer) {
        topCursor.registerDataSetObserver(observer);
        pinnedCursor.registerDataSetObserver(observer);
    }

    @Override
    public void unregisterDataSetObserver(DataSetObserver observer) {
        topCursor.unregisterDataSetObserver(observer);
        pinnedCursor.unregisterDataSetObserver(observer);
    }

    @Override
    public void deactivate() {
        topCursor.deactivate();
        pinnedCursor.deactivate();
    }

    @Override
    public boolean isClosed() {
        return topCursor.isClosed() && pinnedCursor.isClosed();
    }

    @Override
    public void close() {
        topCursor.close();
        pinnedCursor.close();
        pinnedPositions = null;
    }
}
