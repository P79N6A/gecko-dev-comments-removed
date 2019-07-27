package org.mozilla.gecko.tests;

import org.json.JSONObject;
import org.mozilla.gecko.Actions;

import android.util.DisplayMetrics;

public class testAddonManager extends PixelTest  {
    





    public void testAddonManager() {
        Actions.EventExpecter tabEventExpecter;
        Actions.EventExpecter contentEventExpecter;
        String url = StringHelper.ABOUT_ADDONS_URL;

        blockForGeckoReady();

        
        selectMenuItem(StringHelper.ADDONS_LABEL);

        
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyPageTitle(StringHelper.ADDONS_LABEL);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        loadAndPaint(url);
        verifyPageTitle(StringHelper.ADDONS_LABEL);

        
        JSONObject jsonPref = new JSONObject();
        try {
            jsonPref.put("name", "extensions.getAddons.browseAddons");
            jsonPref.put("type", "string");
            jsonPref.put("value", getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL));
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

        
        verifyPageTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        selectMenuItem(StringHelper.ADDONS_LABEL);

        
        verifyTabCount(2);
    }
}
