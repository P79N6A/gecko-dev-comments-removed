package org.mozilla.gecko.tests;


public class testBookmarkKeyword extends AboutHomeTest {
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
