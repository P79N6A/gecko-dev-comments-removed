package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import android.view.View;
import android.widget.ListAdapter;
import android.widget.ListView;
import java.util.ArrayList;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;







public class testAddSearchEngine extends AboutHomeTest {
    private final int MAX_WAIT_TEST_MS = 5000;
    private final String SEARCH_TEXT = "Firefox for Android";
    private final String ADD_SEARCHENGINE_OPTION_TEXT = "Add Search Engine";

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testAddSearchEngine() {
        String blankPageURL = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String searchEngineURL = getAbsoluteUrl(StringHelper.ROBOCOP_SEARCH_URL);

        blockForGeckoReady();
        int height = mDriver.getGeckoTop() + 150;
        int width = mDriver.getGeckoLeft() + 150;

        inputAndLoadUrl(blankPageURL);
        waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        
        Actions.EventExpecter searchEngineDataEventExpector = mActions.expectGeckoEvent("SearchEngines:Data");
        focusUrlBar();
        mActions.sendKeys(SEARCH_TEXT);
        String eventData = searchEngineDataEventExpector.blockForEventData();
        searchEngineDataEventExpector.unregisterListener();

        ArrayList<String> searchEngines;
        try {
            
            searchEngines = getSearchEnginesNames(eventData);
        } catch (JSONException e) {
            mAsserter.ok(false, "Fatal exception in testAddSearchEngine while decoding JSON search engine string from Gecko prior to addition of new engine.", e.toString());
            return;
        }
        final int initialNumSearchEngines = searchEngines.size();
        mAsserter.dumpLog("Search Engines list = " + searchEngines.toString());

        
        verifyDisplayedSearchEnginesCount(initialNumSearchEngines);

        
        inputAndLoadUrl(searchEngineURL);
        waitForText(StringHelper.ROBOCOP_SEARCH_TITLE);
        verifyPageTitle(StringHelper.ROBOCOP_SEARCH_TITLE);

        
        getInstrumentation().waitForIdleSync();
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        mSolo.clickLongOnScreen(width,height);

        mAsserter.ok(waitForText(ADD_SEARCHENGINE_OPTION_TEXT), "Waiting for the context menu to be opened", "The context menu was opened");

        
        mSolo.clickOnText(ADD_SEARCHENGINE_OPTION_TEXT);
        waitForText("Cancel");
        clickOnButton("OK");
        mAsserter.ok(!mSolo.searchText(ADD_SEARCHENGINE_OPTION_TEXT), "Adding the Search Engine", "The add Search Engine pop-up has been closed");
        waitForText(StringHelper.ROBOCOP_SEARCH_TITLE); 

        
        
        loadUrl(blankPageURL);
        waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        searchEngineDataEventExpector = mActions.expectGeckoEvent("SearchEngines:Data");
        focusUrlBar();
        mActions.sendKeys(SEARCH_TEXT);
        eventData = searchEngineDataEventExpector.blockForEventData();

        try {
            
            searchEngines = getSearchEnginesNames(eventData);
        } catch (JSONException e) {
            mAsserter.ok(false, "Fatal exception in testAddSearchEngine while decoding JSON search engine string from Gecko after adding of new engine.", e.toString());
            return;
        }

        mAsserter.dumpLog("Search Engines list = " + searchEngines.toString());
        mAsserter.is(searchEngines.size(), initialNumSearchEngines + 1, "Checking the number of Search Engines has increased");
        
        
        verifyDisplayedSearchEnginesCount(initialNumSearchEngines + 1);
        searchEngineDataEventExpector.unregisterListener();
    }

    







    public ArrayList<String> getSearchEnginesNames(String searchEngineData) throws JSONException {
        JSONObject data = new JSONObject(searchEngineData);
        JSONArray engines = data.getJSONArray("searchEngines");

        ArrayList<String> searchEngineNames = new ArrayList<String>();
        for (int i = 0; i < engines.length(); i++) {
            JSONObject engineJSON = engines.getJSONObject(i);
            searchEngineNames.add(engineJSON.getString("name"));
        }
        return searchEngineNames;
    }

    



    public void verifyDisplayedSearchEnginesCount(final int expectedCount) {
        mSolo.clearEditText(0);
        mActions.sendKeys(SEARCH_TEXT);
        boolean correctNumSearchEnginesDisplayed = waitForTest(new BooleanTest() {
            @Override
            public boolean test() {
                return (findListViewWithTag("browser_search").getAdapter().getCount() == expectedCount);
            }
        }, MAX_WAIT_TEST_MS);
        
        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        waitForText(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);
        mAsserter.ok(correctNumSearchEnginesDisplayed, expectedCount + " Search Engines should be displayed" , "The correct number of Search Engines has been displayed");
   }
}
