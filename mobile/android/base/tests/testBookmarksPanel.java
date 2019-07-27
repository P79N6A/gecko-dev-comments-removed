



package org.mozilla.gecko.tests;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Element;
import org.mozilla.gecko.R;
import org.mozilla.gecko.util.StringUtils;

public class testBookmarksPanel extends AboutHomeTest {
    public void testBookmarksPanel() {
        final String BOOKMARK_URL = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        JSONObject data = null;

        
        
        initializeProfile();

        
        mDatabaseHelper.addOrUpdateMobileBookmark(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, BOOKMARK_URL);

        openAboutHomeTab(AboutHomeTabs.BOOKMARKS);

        
        
        
        for (String url : mStringHelper.DEFAULT_BOOKMARKS_URLS) {
            isBookmarkDisplayed(url);
        }

        assertAllContextMenuOptionsArePresent(mStringHelper.DEFAULT_BOOKMARKS_URLS[1],
                mStringHelper.DEFAULT_BOOKMARKS_URLS[0]);

        openBookmarkContextMenu(mStringHelper.DEFAULT_BOOKMARKS_URLS[0]);

        
        final Element tabCount = mDriver.findElement(getActivity(), R.id.tabs_counter);
        final int tabCountInt = Integer.parseInt(tabCount.getText());
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        mSolo.clickOnText(mStringHelper.BOOKMARK_CONTEXT_MENU_ITEMS[0]);
        try {
            data = new JSONObject(tabEventExpecter.blockForEventData());
        } catch (JSONException e) {
            mAsserter.ok(false, "exception getting event data", e.toString());
        }
        tabEventExpecter.unregisterListener();
        mAsserter.ok(mSolo.searchText(mStringHelper.TITLE_PLACE_HOLDER), "Checking that the tab is not changed", "The tab was not changed");
        
        int tabID = 0;
        try {
            mAsserter.is(mStringHelper.ABOUT_FIREFOX_URL, data.getString("uri"), "Checking tab uri");
            tabID = data.getInt("tabID");
        } catch (JSONException e) {
            mAsserter.ok(false, "exception accessing event data", e.toString());
        }
        
        closeTab(tabID);

        
        openBookmarkContextMenu(mStringHelper.DEFAULT_BOOKMARKS_URLS[0]);
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        mSolo.clickOnText(mStringHelper.BOOKMARK_CONTEXT_MENU_ITEMS[1]);
        try {
            data = new JSONObject(tabEventExpecter.blockForEventData());
        } catch (JSONException e) {
            mAsserter.ok(false, "exception getting event data", e.toString());
        }
        tabEventExpecter.unregisterListener();
        mAsserter.ok(mSolo.searchText(mStringHelper.TITLE_PLACE_HOLDER), "Checking that the tab is not changed", "The tab was not changed");
        
        try {
            mAsserter.is(mStringHelper.ABOUT_FIREFOX_URL, data.getString("uri"), "Checking tab uri");
        } catch (JSONException e) {
            mAsserter.ok(false, "exception accessing event data", e.toString());
        }

        
        String[] editedBookmarkValues = new String[] { "New bookmark title", "www.NewBookmark.url", "newBookmarkKeyword" };
        editBookmark(BOOKMARK_URL, editedBookmarkValues);
        checkBookmarkEdit(editedBookmarkValues[1], editedBookmarkValues);

        
        openBookmarkContextMenu(editedBookmarkValues[1]);
        mSolo.clickOnText(mStringHelper.BOOKMARK_CONTEXT_MENU_ITEMS[5]);
        waitForText(mStringHelper.BOOKMARK_REMOVED_LABEL);
        mAsserter.ok(!mDatabaseHelper.isBookmark(editedBookmarkValues[1]), "Checking that the bookmark was removed", "The bookmark was removed");
    }

    






    private void assertAllContextMenuOptionsArePresent(final String shareableURL,
            final String nonShareableURL) {
        mAsserter.ok(StringUtils.isShareableUrl(shareableURL), "Ensuring url is shareable", "");
        mAsserter.ok(!StringUtils.isShareableUrl(nonShareableURL), "Ensuring url is not shareable", "");

        openBookmarkContextMenu(shareableURL);
        for (String contextMenuOption : mStringHelper.BOOKMARK_CONTEXT_MENU_ITEMS) {
            mAsserter.ok(mSolo.searchText(contextMenuOption),
                    "Checking that the context menu option is present",
                    contextMenuOption + " is present");
        }

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        openBookmarkContextMenu(nonShareableURL);
        for (String contextMenuOption : mStringHelper.BOOKMARK_CONTEXT_MENU_ITEMS) {
            
            if ("Share".equals(contextMenuOption)) {
                continue;
            }

            mAsserter.ok(mSolo.searchText(contextMenuOption),
                    "Checking that the context menu option is present",
                    contextMenuOption + " is present");
        }

        
        
        mAsserter.ok(!mSolo.searchText("Share"),
                "Checking that the Share option is not present",
                "Share option is not present");

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
    }

   



    private void editBookmark(String bookmarkUrl, String[] values) {
        openBookmarkContextMenu(bookmarkUrl);
        mSolo.clickOnText(mStringHelper.CONTEXT_MENU_EDIT);
        waitForText(mStringHelper.EDIT_BOOKMARK);

        
        for (int i = 0; i < values.length; i++) {
            mSolo.clearEditText(i);
            mSolo.clickOnEditText(i);
            mActions.sendKeys(values[i]);
        }

        mSolo.clickOnButton(mStringHelper.OK);
        waitForText(mStringHelper.BOOKMARK_UPDATED_LABEL);
    }

   



    private void checkBookmarkEdit(String bookmarkUrl, String[] values) {
        openBookmarkContextMenu(bookmarkUrl);
        mSolo.clickOnText(mStringHelper.CONTEXT_MENU_EDIT);
        waitForText(mStringHelper.EDIT_BOOKMARK);

        
        for (String value : values) {
            mAsserter.ok(mSolo.searchText(value), "Checking that the value is correct", "The value = " + value + " is correct");
        }

        mSolo.clickOnButton("Cancel");
        waitForText(mStringHelper.BOOKMARKS_LABEL);
    }
}
