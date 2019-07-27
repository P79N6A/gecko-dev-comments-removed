



package org.mozilla.gecko.tests;

import java.io.File;
import java.util.ArrayList;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.mozilla.gecko.Actions;
import org.mozilla.gecko.GeckoProfile;
import org.mozilla.gecko.home.HomePager;

import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;

import com.jayway.android.robotium.solo.Condition;







public class testAddSearchEngine extends AboutHomeTest {
    private final int MAX_WAIT_TEST_MS = 5000;
    private final String SEARCH_TEXT = "Firefox for Android";
    private final String ADD_SEARCHENGINE_OPTION_TEXT = "Add as Search Engine";

    public void testAddSearchEngine() {
        String blankPageURL = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        String searchEngineURL = getAbsoluteUrl(mStringHelper.ROBOCOP_SEARCH_URL);

        blockForGeckoReady();
        int height = mDriver.getGeckoTop() + 150;
        int width = mDriver.getGeckoLeft() + 150;

        inputAndLoadUrl(blankPageURL);
        waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        
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
        verifyUrlBarTitle(searchEngineURL);

        
        getInstrumentation().waitForIdleSync();
        mAsserter.dumpLog("Long Clicking at width = " + String.valueOf(width) + " and height = " + String.valueOf(height));
        mSolo.clickLongOnScreen(width,height);

        ImageView view = waitForViewWithDescription(ImageView.class, ADD_SEARCHENGINE_OPTION_TEXT);
        mAsserter.isnot(view, null, "The action mode was opened");

        
        mSolo.clickOnView(view);
        waitForText("Cancel");
        clickOnButton("OK");
        mAsserter.ok(!mSolo.searchText(ADD_SEARCHENGINE_OPTION_TEXT), "Adding the Search Engine", "The add Search Engine pop-up has been closed");
        waitForText(mStringHelper.ROBOCOP_SEARCH_TITLE); 

        
        
        loadUrl(blankPageURL);
        waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
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

        
        
        final File f = GeckoProfile.get(getActivity()).getFile("searchplugins/robocop-search-engine.xml");
        mAsserter.ok(f.exists(), "Checking that new search plugin file exists", "");
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
        boolean correctNumSearchEnginesDisplayed = waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                ListView list = findListViewWithTag(HomePager.LIST_TAG_BROWSER_SEARCH);
                if (list == null) {
                    return false;
                }
                ListAdapter adapter = list.getAdapter();
                if (adapter == null) {
                    return false;
                }
                return (adapter.getCount() == expectedCount);
            }
        }, MAX_WAIT_TEST_MS);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        waitForText(mStringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);
        mAsserter.ok(correctNumSearchEnginesDisplayed, expectedCount + " Search Engines should be displayed" , "The correct number of Search Engines has been displayed");
    }
}
