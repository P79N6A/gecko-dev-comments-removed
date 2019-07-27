



package org.mozilla.gecko.tests;

import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.Actions;
import org.mozilla.gecko.home.HomePager;
import org.mozilla.gecko.R;

import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;







public class testReaderMode extends AboutHomeTest {
    static final int EVENT_CLEAR_DELAY_MS = 3000;
    static final int READER_ICON_MAX_WAIT_MS = 15000;

    public void testReaderMode() {
        blockForGeckoReady();

        Actions.EventExpecter contentEventExpecter;
        Actions.EventExpecter contentReaderAddedExpecter;
        Actions.EventExpecter faviconExpecter;
        Actions.EventExpecter contentPageShowExpecter;
        Actions.RepeatedEventExpecter paintExpecter;
        ListView list;
        View child;
        View readerIcon;
        String textUrl = getAbsoluteUrl(mStringHelper.ROBOCOP_TEXT_PAGE_URL);
        String devType = mDevice.type;
        int childNo;
        int height;
        int width;

        loadAndPaint(textUrl);

        
        readerIcon = getReaderIcon();
        contentReaderAddedExpecter = mActions.expectGeckoEvent("Reader:Added");
        mSolo.clickLongOnView(readerIcon);
        String eventData = contentReaderAddedExpecter.blockForEventData();
        isAdded(eventData);
        contentReaderAddedExpecter.unregisterListener();

        
        readerIcon = getReaderIcon();
        contentReaderAddedExpecter = mActions.expectGeckoEvent("Reader:Added");
        mSolo.clickLongOnView(readerIcon);
        eventData = contentReaderAddedExpecter.blockForEventData();
        isAdded(eventData);
        contentReaderAddedExpecter.unregisterListener();

        
        faviconExpecter = mActions.expectGeckoEvent("Reader:FaviconRequest");
        contentPageShowExpecter = mActions.expectGeckoEvent("Content:PageShow");
        readerIcon = getReaderIcon();
        paintExpecter = mActions.expectPaint();
        mSolo.clickOnView(readerIcon);

        
        mSolo.setActivityOrientation(Solo.PORTRAIT);
        faviconExpecter.blockForEvent();
        faviconExpecter.unregisterListener();
        contentPageShowExpecter.blockForEvent();
        contentPageShowExpecter.unregisterListener();
        paintExpecter.blockUntilClear(EVENT_CLEAR_DELAY_MS);
        paintExpecter.unregisterListener();
        verifyUrlBarTitle(mStringHelper.ROBOCOP_TEXT_PAGE_URL);

        
        height = mDriver.getGeckoTop() + mDriver.getGeckoHeight() - 10;
        width = mDriver.getGeckoLeft() + mDriver.getGeckoWidth() - 10;
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        mSolo.clickOnScreen(width,height);
        mAsserter.ok(mSolo.waitForText("Share via"), "Waiting for the share menu", "The share menu is present");
        mActions.sendSpecialKey(Actions.SpecialKey.BACK); 

        
        height = mDriver.getGeckoTop() + mDriver.getGeckoHeight() - 10;
        width = mDriver.getGeckoLeft() + 50;
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        mSolo.clickOnScreen(width,height);
        mAsserter.ok(mSolo.waitForText("Page removed from your Reading List"), "Waiting for the page to removed from your Reading List", "The page is removed from your Reading List");

        
        mSolo.clickOnScreen(width,height);
        mAsserter.ok(mSolo.waitForText("Page added to your Reading List"), "Waiting for the page to be added to your Reading List", "The page was added to your Reading List");

        
        height = mDriver.getGeckoTop() + mDriver.getGeckoHeight() - 10;
        width = mDriver.getGeckoLeft() + mDriver.getGeckoWidth()/2 - 10;
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
        mSolo.clickOnScreen(width,height);
        contentEventExpecter.blockForEvent();
        contentEventExpecter.unregisterListener();

        
        mAsserter.ok(mSolo.waitForText(mStringHelper.ROBOCOP_TEXT_PAGE_TITLE),
                "Verify if the page is added to your Reading List",
                mStringHelper.ROBOCOP_TEXT_PAGE_TITLE);

        
        openAboutHomeTab(AboutHomeTabs.HISTORY);
        list = findListViewWithTag(HomePager.LIST_TAG_HISTORY);
        child = list.getChildAt(1);
        mAsserter.ok(child != null, "item can be retrieved", child != null ? child.toString() : "null!");
        mSolo.clickLongOnView(child);
        mAsserter.ok(mSolo.waitForText("Open in Reader"), "Verify if the page is present in history as a Reading List item", "The page is present in history as a Reading List item");
        mActions.sendSpecialKey(Actions.SpecialKey.BACK); 
        mSolo.waitForText(mStringHelper.ROBOCOP_TEXT_PAGE_TITLE);

        
        if (devType.equals("phone")) {
            childNo = 1;
        }
        else {
            childNo = 2;
        }
        
        openAboutHomeTab(AboutHomeTabs.READING_LIST);
        list = findListViewWithTag(HomePager.LIST_TAG_READING_LIST);
        child = list.getChildAt(childNo-1);
        mAsserter.ok(child != null, "Verify if the page is present to your Reading List", "The page is present in your Reading List");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
        mSolo.clickOnView(child);
        contentEventExpecter.blockForEvent();
        contentEventExpecter.unregisterListener();
        verifyUrlBarTitle(mStringHelper.ROBOCOP_TEXT_PAGE_URL);

        
        height = mDriver.getGeckoTop() + mDriver.getGeckoHeight() - 10;
        width = mDriver.getGeckoLeft() + 50;
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        mSolo.clickOnScreen(width,height);
        mAsserter.ok(mSolo.waitForText("Page removed from your Reading List"), "Waiting for the page to removed from your Reading List", "The page is removed from your Reading List");
        verifyUrlBarTitle(mStringHelper.ROBOCOP_TEXT_PAGE_URL);

        
        openAboutHomeTab(AboutHomeTabs.READING_LIST);
        list = findListViewWithTag(HomePager.LIST_TAG_READING_LIST);
        child = list.getChildAt(childNo-1);
        mAsserter.ok(child == null, "Verify if the Reading List is empty", "The Reading List is empty");
    }

    
    protected View getReaderIcon() {
        View pageActionLayout = mSolo.getView(R.id.page_action_layout);
        final ViewGroup actionLayoutEntry = (ViewGroup)pageActionLayout;
        View icon = actionLayoutEntry.getChildAt(1);
        if (icon == null || icon.getVisibility() != View.VISIBLE) {
            
            
            mAsserter.dumpLog("reader icon not visible -- waiting for visibility");
            Condition visibilityCondition = new Condition() {
                @Override
                public boolean isSatisfied() {
                    View conditionIcon = actionLayoutEntry.getChildAt(1);
                    if (conditionIcon == null ||
                        conditionIcon.getVisibility() != View.VISIBLE)
                        return false;
                    return true;
                }
            };
            waitForCondition(visibilityCondition, READER_ICON_MAX_WAIT_MS);
            icon = actionLayoutEntry.getChildAt(1);
            mAsserter.ok(icon != null, "checking reader icon view", "reader icon view not null");
            mAsserter.ok(icon.getVisibility() == View.VISIBLE, "checking reader icon visible", "reader icon visible");
        }
        return icon;
    }

    
    private boolean isAdded(String eventData) {
        try {
            JSONObject data = new JSONObject(eventData);
                if (data.getInt("result") == 0) {
                    mAsserter.ok(true, "Waiting for the page to be added to your Reading List", "The page was added to your Reading List");
                }
                else {
                    if (data.getInt("result") == 2) {
                        mAsserter.ok(true, "Trying to add a second time the page in your Reading List", "The page is already in your Reading List");
                    }
                }
        } catch (JSONException e) {
            mAsserter.ok(false, "Error parsing the event data", e.toString());
            return false;
        }
        return true;
    }
}
