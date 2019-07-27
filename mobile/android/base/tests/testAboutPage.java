package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Element;
import org.mozilla.gecko.NewTabletUI;
import org.mozilla.gecko.R;

import android.app.Activity;





public class testAboutPage extends PixelTest {
    private void ensureTitleMatches(final String titleRegex, final String urlRegex) {
        final Activity activity = getActivity();
        final Element urlBarTitle = mDriver.findElement(activity, R.id.url_bar_title);

        
        final String expectedTitle = NewTabletUI.isEnabled(activity) ? urlRegex : titleRegex;
        mAsserter.isnot(urlBarTitle, null, "Got the URL bar title");
        assertMatches(urlBarTitle.getText(), expectedTitle, "page title match");
    }

    public void testAboutPage() {
        blockForGeckoReady();

        
        String url = StringHelper.ABOUT_SCHEME;
        loadAndPaint(url);

        verifyPageTitle(StringHelper.ABOUT_LABEL, url);

        
        url = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        inputAndLoadUrl(url);

        
        verifyPageTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE, url);

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        selectSettingsItem(StringHelper.MOZILLA_SECTION_LABEL, StringHelper.ABOUT_LABEL);

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        verifyPageTitle(StringHelper.ABOUT_LABEL, StringHelper.ABOUT_SCHEME);
    }
}
