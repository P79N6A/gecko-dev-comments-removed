



package org.mozilla.gecko.tests;

import com.jayway.android.robotium.solo.Condition;

public class testBookmark extends AboutHomeTest {
    private static String BOOKMARK_URL;
    private static final int WAIT_FOR_BOOKMARKED_TIMEOUT = 10000;

    public void testBookmark() {
        BOOKMARK_URL = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        runAboutHomeTest();
        runMenuTest();
    }

    public void runMenuTest() {
        mAsserter.is(mDatabaseHelper.isBookmark(BOOKMARK_URL), false, "Page is not bookmarked initially");
        setUpBookmark(); 
        waitForBookmarked(true);

        cleanUpBookmark(); 
        waitForBookmarked(false);
    }

    public void runAboutHomeTest() {
        blockForGeckoReady();
        for (String url : mStringHelper.DEFAULT_BOOKMARKS_URLS) {
            mAsserter.ok(mDatabaseHelper.isBookmark(url), "Checking that " + url + " is bookmarked by default", url + " is bookmarked");
        }

        mDatabaseHelper.addMobileBookmark(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, BOOKMARK_URL);
        waitForBookmarked(true);

        isBookmarkDisplayed(BOOKMARK_URL);
        loadBookmark(BOOKMARK_URL);
        verifyUrlBarTitle(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);

        mDatabaseHelper.deleteBookmark(BOOKMARK_URL);
        waitForBookmarked(false);
    }

    private void waitForBookmarked(final boolean isBookmarked) {
        boolean bookmarked = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return (isBookmarked) ?
                    mDatabaseHelper.isBookmark(BOOKMARK_URL) :
                    !mDatabaseHelper.isBookmark(BOOKMARK_URL);
            }
        }, WAIT_FOR_BOOKMARKED_TIMEOUT);
        mAsserter.is(bookmarked, true, BOOKMARK_URL + " was " + (isBookmarked ? "added as a bookmark" : "removed from bookmarks"));
    }

    private void setUpBookmark() {
        
        loadUrl(BOOKMARK_URL);
        waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);
        toggleBookmark();
        mAsserter.is(waitForText(mStringHelper.BOOKMARK_ADDED_LABEL), true, "bookmark added successfully");
    }

    private void cleanUpBookmark() {
        
        loadUrl(BOOKMARK_URL);
        waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);
        toggleBookmark();
        mAsserter.is(waitForText(mStringHelper.BOOKMARK_REMOVED_LABEL), true, "bookmark removed successfully");
    }
}
