package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import java.util.ArrayList;

public class testClearPrivateData extends PixelTest {
    private final int TEST_WAIT_MS = 10000;

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testClearPrivateData() {
        blockForGeckoReady();
        clearHistory();
    }

    private void clearHistory() {
        
        String blank1 = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String blank2 = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_02_URL);

        loadAndPaint(blank1);
        waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        mDatabaseHelper.addOrUpdateMobileBookmark(StringHelper.ROBOCOP_BLANK_PAGE_02_TITLE, blank2);

        
        verifyHistoryCount(1);
        clearPrivateData();

        
        verifyHistoryCount(0);

        
        mAsserter.ok(mDatabaseHelper.isBookmark(blank2), "Checking that bookmarks have not been removed", "User made bookmarks were not removed with private data");
    }

    private void verifyHistoryCount(final int expectedCount) {
        boolean match = waitForTest( new BooleanTest() {
            public boolean test() {
                return (mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.HISTORY).size() == expectedCount);
            }
        }, TEST_WAIT_MS);
        mAsserter.ok(match, "Checking that the number of history items is correct", String.valueOf(expectedCount) + " history items present in the database");
    }
}
