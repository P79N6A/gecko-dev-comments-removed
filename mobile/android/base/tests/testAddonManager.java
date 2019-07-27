



package org.mozilla.gecko.tests;

import org.json.JSONObject;
import org.mozilla.gecko.Actions;

import android.util.DisplayMetrics;









public class testAddonManager extends PixelTest  {
    public void testAddonManager() {
        Actions.EventExpecter tabEventExpecter;
        Actions.EventExpecter contentEventExpecter;
        final String aboutAddonsURL = mStringHelper.ABOUT_ADDONS_URL;

        blockForGeckoReady();

        
        selectMenuItem(mStringHelper.ADDONS_LABEL);

        
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyUrlBarTitle(aboutAddonsURL);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);

        
        loadAndPaint(aboutAddonsURL);
        verifyUrlBarTitle(aboutAddonsURL);

        
        tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        
        final String blankURL = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        addTab(blankURL);

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyTabCount(2);

        
        verifyUrlBarTitle(blankURL);

        
        selectMenuItem(mStringHelper.ADDONS_LABEL);

        
        verifyTabCount(2);
    }
}
