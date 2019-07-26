package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

public class testBookmarksPage extends AboutHomeTest {

    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testBookmarksPage() {
        final String BOOKMARK_URL = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);

        
        mDatabaseHelper.addOrUpdateMobileBookmark(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, BOOKMARK_URL);

        openAboutHomeTab(AboutHomeTabs.BOOKMARKS);

        
        for (String url : StringHelper.DEFAULT_BOOKMARKS_URLS) {
            isBookmarkDisplayed(url);
        }

        
        openBookmarkContextMenu(StringHelper.DEFAULT_BOOKMARKS_URLS[0]);

        
        for (String contextMenuOption : StringHelper.BOOKMARK_CONTEXT_MENU_ITEMS) {
            mAsserter.ok(mSolo.searchText(contextMenuOption), "Checking that the context menu option is present", contextMenuOption + " is present");
        }

        
        final Element tabCount = mDriver.findElement(getActivity(), "tabs_counter");
        final int tabCountInt = Integer.parseInt(tabCount.getText());
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        mSolo.clickOnText(StringHelper.BOOKMARK_CONTEXT_MENU_ITEMS[0]);
        tabEventExpecter.blockForEvent();
        tabEventExpecter.unregisterListener();
        mAsserter.ok(mSolo.searchText(StringHelper.TITLE_PLACE_HOLDER), "Checking that the tab is not changed", "The tab was not changed");

        
        openBookmarkContextMenu(StringHelper.DEFAULT_BOOKMARKS_URLS[1]);
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        mSolo.clickOnText(StringHelper.BOOKMARK_CONTEXT_MENU_ITEMS[1]);
        tabEventExpecter.blockForEvent();
        tabEventExpecter.unregisterListener();
        mAsserter.ok(mSolo.searchText(StringHelper.TITLE_PLACE_HOLDER), "Checking that the tab is not changed", "The tab was not changed");

        
        String[] editedBookmarkValues = new String[] { "New bookmark title", "www.NewBookmark.url", "newBookmarkKeyword" };
        editBookmark(BOOKMARK_URL, editedBookmarkValues);
        checkBookmarkEdit(editedBookmarkValues[1], editedBookmarkValues);

        
        openBookmarkContextMenu(editedBookmarkValues[1]);
        mSolo.clickOnText(StringHelper.BOOKMARK_CONTEXT_MENU_ITEMS[3]);
        waitForText("Bookmark removed");
        mAsserter.ok(!mDatabaseHelper.isBookmark(editedBookmarkValues[1]), "Checking that the bookmark was removed", "The bookmark was removed");
    }

   



    private void editBookmark(String bookmarkUrl, String[] values) {
        openBookmarkContextMenu(bookmarkUrl);
        mSolo.clickOnText("Edit");
        waitForText("Edit Bookmark");

        
        for (int i = 0; i < values.length; i++) {
            mSolo.clearEditText(i);
            mSolo.clickOnEditText(i);
            mActions.sendKeys(values[i]);
        }

        mSolo.clickOnButton("OK");
        waitForText("Bookmark updated");
    }

   



    private void checkBookmarkEdit(String bookmarkUrl, String[] values) {
        openBookmarkContextMenu(bookmarkUrl);
        mSolo.clickOnText("Edit");
        waitForText("Edit Bookmark");

        
        for (String value : values) {
            mAsserter.ok(mSolo.searchText(value), "Checking that the value is correct", "The value = " + value + " is correct");
        }

        mSolo.clickOnButton("Cancel");
        waitForText("BOOKMARKS");
    }
}
