package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

public class testBookmarkKeyword extends AboutHomeTest {

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testBookmarkKeyword() {
        blockForGeckoReady();

        final String url = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        final String keyword = "testkeyword";

        
        mDatabaseHelper.addOrUpdateMobileBookmark(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, url);
        mDatabaseHelper.updateBookmark(url, StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, keyword);

        
        inputAndLoadUrl(keyword);

        
        waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        verifyPageTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        mDatabaseHelper.deleteBookmark(url);
    }
}
