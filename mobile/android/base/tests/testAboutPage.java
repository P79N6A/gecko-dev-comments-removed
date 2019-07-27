



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Element;
import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;

import android.app.Activity;





public class testAboutPage extends PixelTest {

    public void testAboutPage() {
        blockForGeckoReady();

        
        String url = StringHelper.ABOUT_SCHEME;
        loadAndPaint(url);

        verifyUrlBarTitle(url);

        
        url = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        inputAndLoadUrl(url);

        
        verifyUrlBarTitle(url);

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        selectSettingsItem(StringHelper.MOZILLA_SECTION_LABEL, StringHelper.ABOUT_LABEL);

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyUrlBarTitle(StringHelper.ABOUT_SCHEME);
    }
}
