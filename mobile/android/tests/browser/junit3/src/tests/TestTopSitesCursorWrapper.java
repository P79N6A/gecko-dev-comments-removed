


package org.mozilla.gecko.browser.tests;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.database.MatrixCursor.RowBuilder;
import android.text.TextUtils;

import java.util.Arrays;
import java.util.List;

import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.Combined;
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
        String url = c.getString(c.getColumnIndex(Combined.URL));
        String title = c.getString(c.getColumnIndex(Combined.TITLE));

        assertEquals(prefix + "url" + index, url);
        assertEquals(prefix + "title" + index, title);
    }

    private void assertBlank(Cursor c) {
        String url = c.getString(c.getColumnIndex(Combined.URL));
        String title = c.getString(c.getColumnIndex(Combined.TITLE));

        assertTrue(TextUtils.isEmpty(url));
        assertTrue(TextUtils.isEmpty(title));
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

    public void testClosedPinnedSites() {
        Cursor pinnedSites = createPinnedSitesCursor(new Integer[] { 0, 1 });
        Cursor topSites = createTopSitesCursor(2);
        Cursor wrapper = new TopSitesCursorWrapper(pinnedSites, topSites, MIN_COUNT);

        
        
        assertTrue(pinnedSites.isClosed());

        
        assertFalse(topSites.isClosed());

        wrapper.close();

        
        assertTrue(topSites.isClosed());
    }

    public void testIsPinned() {
        Integer[] pinnedPositions = new Integer[] { 0, 1 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(2, pinnedPositions);

        List<Integer> pinnedList = Arrays.asList(pinnedPositions);

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            boolean isPinnedPosition = pinnedList.contains(c.getPosition());
            assertEquals(isPinnedPosition, c.isPinned());
        }

        c.close();
    }

    public void testBlankPositions() {
        Integer[] pinnedPositions = new Integer[] { 0, 1, 4 };
        TopSitesCursorWrapper c = createTopSitesCursorWrapper(0, pinnedPositions);

        c.moveToPosition(-1);
        while (c.moveToNext()) {
            if (!c.isPinned()) {
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
            int position = c.getPosition();

            
            if (position == MIN_COUNT - 1) {
                assertTrue(c.isNull(c.getColumnIndex(Combined.URL)));
                assertTrue(c.isNull(c.getColumnIndex(Combined.TITLE)));
                assertTrue(c.isNull(c.getColumnIndex(Combined.DISPLAY)));
                assertTrue(c.isNull(c.getColumnIndex(Combined.BOOKMARK_ID)));
                assertTrue(c.isNull(c.getColumnIndex(Combined.HISTORY_ID)));
            } else {
                if (c.isPinned()) {
                    assertFalse(c.isNull(c.getColumnIndex(Combined.URL)));
                    assertFalse(c.isNull(c.getColumnIndex(Combined.TITLE)));
                    assertTrue(c.isNull(c.getColumnIndex(Combined.DISPLAY)));
                    assertTrue(c.isNull(c.getColumnIndex(Combined.BOOKMARK_ID)));
                    assertTrue(c.isNull(c.getColumnIndex(Combined.HISTORY_ID)));
                } else {
                    assertFalse(c.isNull(c.getColumnIndex(Combined.URL)));
                    assertFalse(c.isNull(c.getColumnIndex(Combined.TITLE)));
                    assertFalse(c.isNull(c.getColumnIndex(Combined.DISPLAY)));
                    assertFalse(c.isNull(c.getColumnIndex(Combined.BOOKMARK_ID)));
                    assertFalse(c.isNull(c.getColumnIndex(Combined.HISTORY_ID)));
                }
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
            int position = c.getPosition();

            
            if (position == MIN_COUNT - 1) {
                assertBlank(c);
            } else {
                int index;
                String prefix;

                if (c.isPinned()) {
                    index = pinnedIndex++;
                    prefix = PINNED_PREFIX;
                } else {
                    index = topIndex++;
                    prefix = TOP_PREFIX;
                }

                assertUrlAndTitle(c, prefix, index);
            }
        }

        c.close();
    }
}