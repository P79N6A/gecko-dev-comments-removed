package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import android.util.DisplayMetrics;

import org.json.JSONArray;
import org.json.JSONObject;

public class testAddonManager extends PixelTest  {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    





    public void testAddonManager() {
        Actions.EventExpecter tabEventExpecter;
        Actions.EventExpecter contentEventExpecter;
        String url = "about:addons";

        blockForGeckoReady();

        
        selectMenuItem("Add-ons");

        
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyPageTitle("Add-ons");

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        loadAndPaint(url);
        verifyPageTitle("Add-ons");

        
        JSONObject jsonPref = new JSONObject();
        try {
            jsonPref.put("name", "extensions.getAddons.browseAddons");
            jsonPref.put("type", "string");
            jsonPref.put("value", getAbsoluteUrl("/robocop/robocop_blank_01.html"));
            setPreferenceAndWaitForChange(jsonPref);

        } catch (Exception ex) { 
            mAsserter.ok(false, "exception in testAddonManager", ex.toString());
        }

        
        DisplayMetrics dm = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(dm);

        


        float top = mDriver.getGeckoTop() + 25 * dm.density;;
        float right = mDriver.getGeckoLeft() + mDriver.getGeckoWidth() - 25 * dm.density;;

        
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        
        mSolo.clickOnScreen(right, top);

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyTabCount(2);

        
        verifyPageTitle("Browser Blank Page 01");

        
        selectMenuItem("Add-ons");        

        
        verifyTabCount(2);
    }
}
