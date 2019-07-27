package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Element;
import org.mozilla.gecko.R;





public class testAboutPage extends PixelTest {
    private void ensureTitleMatches(final String regex) {
        Element urlBarTitle = mDriver.findElement(getActivity(), R.id.url_bar_title);
        mAsserter.isnot(urlBarTitle, null, "Got the URL bar title");
        assertMatches(urlBarTitle.getText(), regex, "page title match");
    }

    public void testAboutPage() {
        blockForGeckoReady();

        
        String url = "about:";
        loadAndPaint(url);

        ensureTitleMatches(StringHelper.ABOUT_LABEL);

        
        url = getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        inputAndLoadUrl(url);

        
        ensureTitleMatches(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        selectSettingsItem(StringHelper.MOZILLA_SECTION_LABEL, StringHelper.ABOUT_LABEL);

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        ensureTitleMatches(StringHelper.ABOUT_LABEL);
    }
}
