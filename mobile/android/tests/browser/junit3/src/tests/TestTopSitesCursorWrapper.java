


package org.mozilla.gecko.browser.tests;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.text.TextUtils;

import java.util.Arrays;
import java.util.List;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
import org.mozilla.gecko.db.BrowserContract.TopSites;
import org.mozilla.gecko.db.TopSitesCursorWrapper;

public class TestTopSitesCursorWrapper extends BrowserTestCase {

    private String[] TOP_SITES_COLUMNS = new String[] { Combined._ID,
                                                        Combined.URL,
                                                        Combined.TITLE,
                                                        Combined.DISPLAY,
                                                        Combined.BOOKMARK_ID,
                                                        Combined.HISTORY_ID };

    private String[] PINNED_SITES_COLUMNS = new String[] { Bookmarks._ID,
                                                           Bookmarks.URL,
                                                           Bookmarks.TITLE,
                                                           Bookmarks.POSITION };

    private final int MIN_COUNT = 6;

    private final String TOP_PREFIX = "top-";
    private final String PINNED_PREFIX = "pinned-";

    private Cursor createTopSitesCursor(int count) {
        MatrixCursor c = new MatrixCursor(TOP_SITES_COLUMNS);

        for (int i = 0; i < count; i++) {
            RowBuilder row = c.newRow();
            row.add(-1);
            row.add(TOP_PREFIX + "url" + i);
            row.add(TOP_PREFIX + "title" + i);
            row.add(Combined.DISPLAY_NORMAL);
            row.add(i);
            row.add(i);
        }

        return c;
    }

    private Cursor createPinnedSitesCursor(Integer[] positions) {
        MatrixCursor c = new MatrixCursor(PINNED_SITES_COLUMNS);

        if (positions == null) {
            return c;
        }

        for (int i = 0; i < positions.length; i++) {
            int position = positions[i];

            RowBuilder row = c.newRow();
            row.add(-1);
            row.add(PINNED_PREFIX + "url" + i);
            row.add(PINNED_PREFIX + "title" + i);
            row.add(position);
        }

        return c;
    }

    private TopSitesCursorWrapper createTopSitesCursorWrapper(int topSitesCount, Integer[] pinnedPositions) {
        Cursor pinnedSites = createPinnedSitesCursor(pinnedPositions);
        Cursor topSites = createTopSitesCursor(topSitesCount);

        return new TopSitesCursorWrapper(pinnedSites, topSites, MIN_COUNT);
    }

    private void assertUrlAndTitle(Cursor c, String prefix, int index) {
        String url = c.getString(c.getColumnIndex(TopSites.URL));
        String title = c.getString(c.getColumnIndex(TopSites.TITLE));

        assertEquals(prefix + "url" + index, url);
        assertEquals(prefix + "title" + index, title);
    }

    private void assertBlank(Cursor c) {
        String url = c.getString(c.getColumnIndex(TopSites.URL));
        String title = c.getString(c.getColumnIndex(TopSites.TITLE));

        assertTrue(TextUtils.isEmpty(url));
        assertTrue(TextUtils.isEmpty(title));
    }

    private void assertRowType(Cursor c, int position, int rowType) {
        assertTrue(c.moveToPosition(position));
        assertEquals(rowType, getRowType(c));
    }

    private int getRowType(Cursor c) {
        return c.getInt(c.getColumnIndex(TopSites.TYPE));
    }

    public void testCount() {
        
        Cursor c = createTopSitesCursorWrapper(2, new Integer[] { 0, 1 });
        assertEquals(MIN_COUNT, c.getCount());
        c.close();

        
        c = createTopSitesCursorWrapper(0, new Integer[] { 0, 1, 2 });
        assertEquals(MIN_COUNT, c.getCount());
        c.close();

        
        c = createTopSitesCursorWrapper(3, null);
        assertEquals(MIN_COUNT, c.getCount());
        c.close();

        
        c = createTopSitesCursorWrapper(10, new Integer[] { 0, 1, 2 });
        assertEquals(13, c.getCount());
        c.close();
    }

    public void testCloseWrappedCursors() {
        Cursor pinnedSites = createPinnedSitesCursor(new Integer[] { 0, 1 });
        Cursor topSites = createTopSitesCursor(2);
        Cursor wrapper = new TopSitesCursorWrapper(pinnedSites, topSites, MIN_COUNT);

        assertFalse(pinnedSites.isClosed());
        assertFalse(topSites.isClosed());

        wrapper.close();

        
        assertTrue(pinnedSites.isClosed());
        assertTrue(topSites.isClosed());
    }

    public void testIsPinned() {
        Integer[] pinnedPositions = new Integer[] { 0, 1 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        List<Integer> pinnedList = Arrays.asList(pinnedPositions);

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            boolean isPinnedPosition = pinnedList.contains(c.getPosition());
            assertEquals(isPinnedPosition, getRowType(c) == TopSites.TYPE_PINNED);
        }

        c.close();
    }

    public void testBlankPositions() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(0, pinnedPositions);

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            if (getRowType(c) == TopSites.TYPE_BLANK) {
                assertBlank(c);
            }
        }

        c.close();
    }

    public void testIsNull() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            int rowType = getRowType(c);

            if (rowType == TopSites.TYPE_BLANK) {
                assertTrue(c.isNull(c.getColumnIndex(TopSites.URL)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.TITLE)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.DISPLAY)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.BOOKMARK_ID)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.HISTORY_ID)));
            } else if (rowType == TopSites.TYPE_PINNED) {
                assertFalse(c.isNull(c.getColumnIndex(TopSites.URL)));
                assertFalse(c.isNull(c.getColumnIndex(TopSites.TITLE)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.DISPLAY)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.BOOKMARK_ID)));
                assertTrue(c.isNull(c.getColumnIndex(TopSites.HISTORY_ID)));
            } else if (rowType == TopSites.TYPE_TOP) {
                assertFalse(c.isNull(c.getColumnIndex(TopSites.URL)));
                assertFalse(c.isNull(c.getColumnIndex(TopSites.TITLE)));
                assertFalse(c.isNull(c.getColumnIndex(TopSites.DISPLAY)));
                assertFalse(c.isNull(c.getColumnIndex(TopSites.BOOKMARK_ID)));
                assertFalse(c.isNull(c.getColumnIndex(TopSites.HISTORY_ID)));
            } else {
                fail("Invalid row type found in the cursor");
            }
        }

        c.close();
    }

    public void testColumnValues() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        int topIndex = 0;
        int pinnedIndex = 0;

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            int rowType = getRowType(c);

            if (rowType == TopSites.TYPE_BLANK) {
                assertBlank(c);
            } else if (rowType == TopSites.TYPE_PINNED) {
                assertUrlAndTitle(c, PINNED_PREFIX, pinnedIndex++);
            } else if (rowType == TopSites.TYPE_TOP) {
                assertUrlAndTitle(c, TOP_PREFIX, topIndex++);
            } else {
                fail("Invalid row type found in the cursor");
            }
        }

        c.close();
    }

    public void testColumns() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        assertEquals(7, c.getColumnCount());

        String[] columnNames = c.getColumnNames();
        assertEquals(columnNames.length, c.getColumnCount());

        boolean allRowsHaveAllCols = true;
        c.moveToPosition(-1);
        while (c.moveToNext()) {
            for (int i = 0; i < columnNames.length; i++) {
                try {
                    c.getColumnIndexOrThrow(columnNames[i]);
                } catch (Exception e) {
                    allRowsHaveAllCols = false;
                }
            }
        }
        assertTrue(allRowsHaveAllCols);

        c.close();
    }

    public void testRowTypes() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        
        for (int i = 0; i < pinnedPositions.length; i++) {
            assertRowType(c, pinnedPositions[0], TopSites.TYPE_PINNED);
        }

        
        assertRowType(c, 2, TopSites.TYPE_TOP);
        assertRowType(c, 3, TopSites.TYPE_TOP);

        
        assertRowType(c, 5, TopSites.TYPE_BLANK);

        c.close();
    }

    public void testPositionOutOfBounds() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        boolean failed = false;
        try {
            assertFalse(c.moveToPosition(-1));
            c.getInt(0);
        } catch (Exception e) {
            failed = true;
        }
        assertTrue(failed);

        failed = false;
        try {
            assertFalse(c.moveToPosition(c.getCount()));
            c.getInt(0);
        } catch (Exception e) {
            failed = true;
        }
        assertTrue(failed);

        c.close();
    }

    public void testColumnIndexOutOfBounds() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        boolean failed = false;
        try {
            assertTrue(c.moveToPosition(0));
            c.getInt(-1);
        } catch (Exception e) {
            failed = true;
        }
        assertTrue(failed);

        failed = false;
        try {
            assertTrue(c.moveToPosition(0));
            c.getString(c.getColumnCount());
        } catch (Exception e) {
            failed = true;
        }
        assertTrue(failed);

        c.close();
    }
}