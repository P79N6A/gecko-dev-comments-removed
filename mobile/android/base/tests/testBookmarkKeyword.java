



package org.mozilla.gecko.tests;


public class testBookmarkKeyword extends AboutHomeTest {
    public void testBookmarkKeyword() {
        blockForGeckoReady();

        final String url = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        final String keyword = "testkeyword";

        
        mDatabaseHelper.addOrUpdateMobileBookmark(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, url);
        mDatabaseHelper.updateBookmark(url, mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, keyword);

        
        inputAndLoadUrl(keyword);

        
        verifyUrlBarTitle(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);

        
        mDatabaseHelper.deleteBookmark(url);
    }
}
