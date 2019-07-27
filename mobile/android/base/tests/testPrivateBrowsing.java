



package org.mozilla.gecko.tests;

import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.Actions;







public class testPrivateBrowsing extends ContentContextMenuTest {

    public void testPrivateBrowsing() {
        String bigLinkUrl = getAbsoluteUrl(mStringHelper.ROBOCOP_BIG_LINK_URL);
        String blank1Url = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String blank2Url = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_02_URL);

        blockForGeckoReady();

        inputAndLoadUrl(mStringHelper.ABOUT_BLANK_URL);

        addTab(bigLinkUrl, mStringHelper.ROBOCOP_BIG_LINK_TITLE, true);

        verifyTabCount(1);

        
        verifyContextMenuItems(mStringHelper.CONTEXT_MENU_ITEMS_IN_PRIVATE_TAB);

        
        mAsserter.ok(!mSolo.searchText(mStringHelper.CONTEXT_MENU_ITEMS_IN_NORMAL_TAB[0]), "Checking that 'Open Link in New Tab' is not displayed in the context menu", "'Open Link in New Tab' is not displayed in the context menu");

        
        Actions.EventExpecter privateTabEventExpector = mActions.expectGeckoEvent("Tab:Added");
        mSolo.clickOnText(mStringHelper.CONTEXT_MENU_ITEMS_IN_PRIVATE_TAB[0]);
        String eventData = privateTabEventExpector.blockForEventData();
        privateTabEventExpector.unregisterListener();

        mAsserter.ok(isTabPrivate(eventData), "Checking if the new tab opened from the context menu was a private tab", "The tab was a private tab");
        verifyTabCount(2);

        
        addTab(blank2Url, mStringHelper.ROBOCOP_BLANK_PAGE_02_TITLE, false);
        verifyTabCount(2);

        
        final ArrayList<String> firefoxHistory = mDatabaseHelper.getBrowserDBUrls(DatabaseHelper.BrowserDataType.HISTORY);

        mAsserter.ok(!firefoxHistory.contains(bigLinkUrl), "Check that the link opened in the first private tab was not saved", bigLinkUrl + " was not added to history");
        mAsserter.ok(!firefoxHistory.contains(blank1Url), "Check that the link opened in the private tab from the context menu was not saved", blank1Url + " was not added to history");
        mAsserter.ok(firefoxHistory.contains(blank2Url), "Check that the link opened in the normal tab was saved", blank2Url + " was added to history");
    }

    private boolean isTabPrivate(String eventData) {
        try {
            JSONObject data = new JSONObject(eventData);
            return data.getBoolean("isPrivate");
        } catch (JSONException e) {
            mAsserter.ok(false, "Error parsing the event data", e.toString());
            return false;
        }
    }
}
