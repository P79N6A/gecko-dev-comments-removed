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

        ensureTitleMatches("About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)");

        
        url = getAbsoluteUrl("/robocop/robocop_blank_01.html");
        inputAndLoadUrl(url);

        
        ensureTitleMatches("Browser Blank Page 01");

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        selectSettingsItem("Mozilla", "About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)");

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        ensureTitleMatches("About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)");
    }
}
