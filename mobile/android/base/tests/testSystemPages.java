package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;




public class testSystemPages extends PixelTest {
    final int mExpectedTabCount = 1;
    private static final int AFTER_BACK_SLEEP_MS = 500;

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testSystemPages() {
        blockForGeckoReady();

        String urls [] = { "about:firefox", "about:rights", "about:addons", "about:downloads", "about:buildconfig", "about:feedback", "about:healthreport", "about:" };
        
        String menuItems [][][] = {{{ "Apps" }, { "about:apps" }},
                                  {{ "Downloads" }, { "about:downloads" }},
                                  {{ "Add-ons" }, { "about:addons" }},
                                  {{ "Settings", "Mozilla", "About (Fennec|Nightly|Aurora|Firefox|Firefox Beta)" }, { "about:" }},
                                  {{ "Settings", "Mozilla", "Give feedback" }, { "about:feedback" }},
                                  {{ "Settings", "Mozilla", "View my Health Report" }, { "about:healthreport" }}};

        
        checkUrl(urls);

        

        loadAndPaint("about:config");

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        
        mSolo.sleep(AFTER_BACK_SLEEP_MS);

        
        loadAndPaint("about:");
        verifyUrl("about:"); 

        
        loadItemsByLevel(menuItems);
    }

    
    public void checkUrl(String urls []) {
        for (String url:urls) {
            loadAndPaint(url);
            verifyTabCount(mExpectedTabCount);
            verifyUrl(url);
        }
    }

    public void loadItemsByLevel(String[][][] menuItems) {
        Actions.EventExpecter tabEventExpecter;
        Actions.EventExpecter contentEventExpecter;
        Actions.RepeatedEventExpecter paintExpecter = mActions.expectPaint();
        int expectedTabCount = mExpectedTabCount;
        
        for (String[][] item : menuItems) {
            String [] pathToItem = item[0];
            String expectedUrl = item[1][0];

            expectedTabCount++;

            
            tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
            contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
            selectMenuItemByPath(pathToItem);

            
            if ("about:".equals(expectedUrl)) {
                waitForPaint(paintExpecter); 
                paintExpecter.unregisterListener();
            } else {
                tabEventExpecter.blockForEvent();
                contentEventExpecter.blockForEvent();
            }
            tabEventExpecter.unregisterListener();
            contentEventExpecter.unregisterListener();

            verifyUrl(expectedUrl);
            if ("about:".equals(expectedUrl)) {
                
                expectedTabCount--;
            }
            verifyTabCount(expectedTabCount);
        }
    }
}
