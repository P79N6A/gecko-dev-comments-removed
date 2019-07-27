



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;





public class testAboutPage extends PixelTest {

    public void testAboutPage() {
        blockForGeckoReady();

        
        String url = mStringHelper.ABOUT_SCHEME;
        loadAndPaint(url);

        verifyUrlBarTitle(url);

        
        url = getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        loadUrlAndWait(url);

        
        verifyUrlBarTitle(url);

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        selectSettingsItem(mStringHelper.MOZILLA_SECTION_LABEL, mStringHelper.ABOUT_LABEL);

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyUrlBarTitle(mStringHelper.ABOUT_SCHEME);
    }
}
