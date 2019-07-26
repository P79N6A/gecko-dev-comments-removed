package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;





public class testAboutPage extends PixelTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testAboutPage() {
        blockForGeckoReady();

        
        String url = "about:";
        loadAndPaint(url);

        Element urlBarTitle = mDriver.findElement(getActivity(), URL_BAR_TITLE_ID);
        mAsserter.isnot(urlBarTitle, null, "Got the URL bar title");
        assertMatches(urlBarTitle.getText(), "About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)", "page title match");

        
        url = getAbsoluteUrl("/robocop/robocop_blank_01.html");
        inputAndLoadUrl(url);

        
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        Actions.EventExpecter contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");

        selectSettingsItem("Mozilla", "About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)");

        
        tabEventExpecter.blockForEvent();
        contentEventExpecter.blockForEvent();

        tabEventExpecter.unregisterListener();
        contentEventExpecter.unregisterListener();

        
        urlBarTitle = mDriver.findElement(getActivity(), URL_BAR_TITLE_ID);
        mAsserter.isnot(urlBarTitle, null, "Got the URL bar title");
        assertMatches(urlBarTitle.getText(), "About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)", "page title match");
    }
}
