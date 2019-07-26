package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import com.jayway.android.robotium.solo.Solo;
import android.widget.ListView;
import android.view.View;
import java.util.ArrayList;
import android.view.ViewGroup;
import org.json.JSONException;
import org.json.JSONObject;







public class testReaderMode extends AboutHomeTest {
    int height,width;

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }
    public void testReaderMode() {
        blockForGeckoReady();

        Actions.EventExpecter contentEventExpecter;
        Actions.EventExpecter contentReaderAddedExpecter;
        Actions.EventExpecter faviconExpecter;
        ListView list;
        View child;
        String textUrl = getAbsoluteUrl(StringHelper.ROBOCOP_TEXT_PAGE_URL);
        String devType = mDevice.type;
        int childNo;

        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
        loadAndPaint(textUrl);
        contentEventExpecter.blockForEvent();
        contentEventExpecter.unregisterListener();
        View readerIcon = getReaderIcon();

        
        contentReaderAddedExpecter = mActions.expectGeckoEvent("Reader:Added");
        mSolo.clickLongOnView(readerIcon);
        String eventData = contentReaderAddedExpecter.blockForEventData();
        isAdded(eventData);
        contentReaderAddedExpecter.unregisterListener();

        
        contentReaderAddedExpecter = mActions.expectGeckoEvent("Reader:Added");
        mSolo.clickLongOnView(readerIcon);
        eventData = contentReaderAddedExpecter.blockForEventData();
        isAdded(eventData);
        contentReaderAddedExpecter.unregisterListener();

        
        faviconExpecter = mActions.expectGeckoEvent("Reader:FaviconRequest");
        mSolo.clickOnView(getReaderIcon());

        
        mSolo.setActivityOrientation(Solo.PORTRAIT);
        faviconExpecter.blockForEvent();
        faviconExpecter.unregisterListener();
        verifyPageTitle("Robocop Text Page");

        
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

        
        mAsserter.ok(mSolo.waitForText("Robocop Text Page"), "Verify if the page is added to your Reading List", "The page is present in your Reading List");

        
        openAboutHomeTab(AboutHomeTabs.MOST_RECENT);
        list = findListViewWithTag("most_recent");
        child = list.getChildAt(1);
        mAsserter.ok(child != null, "item can be retrieved", child != null ? child.toString() : "null!");
        mSolo.clickLongOnView(child);
        mAsserter.ok(mSolo.waitForText("Open in Reader"), "Verify if the page is present in history as a Reading List item", "The page is present in history as a Reading List item");
        mActions.sendSpecialKey(Actions.SpecialKey.BACK); 
        mSolo.waitForText("Robocop Text Page");

        
        if (devType.equals("phone")) {
            childNo = 1;
        }
        else {
            childNo = 2;
        }
        
        openAboutHomeTab(AboutHomeTabs.READING_LIST);
        list = findListViewWithTag("reading_list");
        child = list.getChildAt(childNo-1);
        mAsserter.ok(child != null, "Verify if the page is present to your Reading List", "The page is present in your Reading List");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
        mSolo.clickOnView(child);
        contentEventExpecter.blockForEvent();
        contentEventExpecter.unregisterListener();
        verifyPageTitle("Robocop Text Page");

        
        height = mDriver.getGeckoTop() + mDriver.getGeckoHeight() - 10;
        width = mDriver.getGeckoLeft() + 50;
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        mSolo.clickOnScreen(width,height);
        mAsserter.ok(mSolo.waitForText("Page removed from your Reading List"), "Waiting for the page to removed from your Reading List", "The page is removed from your Reading List");
        verifyPageTitle("Robocop Text Page");

        
        openAboutHomeTab(AboutHomeTabs.READING_LIST);
        list = findListViewWithTag("reading_list");
        child = list.getChildAt(childNo-1);
        mAsserter.ok(child == null, "Verify if the Reading List is empty", "The Reading List is empty");
    }

    
    protected View getReaderIcon() {
        View pageActionLayout = mSolo.getView(0x7f070025);
        ArrayList<String> pageActionLayoutChilds = new ArrayList();
        View actionLayoutItem = pageActionLayout;
        ViewGroup actionLayoutEntry = (ViewGroup)actionLayoutItem;
        View icon = actionLayoutEntry.getChildAt(1);
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
