package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.AppConstants;




public class testSystemPages extends PixelTest {
    final int mExpectedTabCount = 1;
    private static final int AFTER_BACK_SLEEP_MS = 500;

    public void testSystemPages() {
        blockForGeckoReady();

        final String urls [] = { StringHelper.ABOUT_FIREFOX_URL, StringHelper.ABOUT_RIGHTS_URL,
                StringHelper.ABOUT_ADDONS_URL, StringHelper.ABOUT_DOWNLOADS_URL, StringHelper.ABOUT_PASSWORDS_URL,
                StringHelper.ABOUT_BUILDCONFIG_URL, StringHelper.ABOUT_FEEDBACK_URL,
                StringHelper.ABOUT_HEALTHREPORT_URL, StringHelper.ABOUT_SCHEME
        };
        
        String menuItems [][][] = {{{ StringHelper.APPS_LABEL }, { StringHelper.ABOUT_APPS_URL }},
                                  {{ StringHelper.DOWNLOADS_LABEL }, { StringHelper.ABOUT_DOWNLOADS_URL}},
                                  {{ StringHelper.LOGINS_LABEL}, { StringHelper.ABOUT_PASSWORDS_URL }},
                                  {{ StringHelper.ADDONS_LABEL }, { StringHelper.ABOUT_ADDONS_URL }},
                                  {{ StringHelper.SETTINGS_LABEL, StringHelper.MOZILLA_SECTION_LABEL, StringHelper.ABOUT_LABEL }, { StringHelper.ABOUT_SCHEME }},
                                  {{ StringHelper.SETTINGS_LABEL, StringHelper.MOZILLA_SECTION_LABEL, StringHelper.FEEDBACK_LABEL }, { StringHelper.ABOUT_FEEDBACK_URL }},
                                  {{ StringHelper.SETTINGS_LABEL, StringHelper.MOZILLA_SECTION_LABEL, StringHelper.MY_HEALTH_REPORT_LABEL }, { StringHelper.ABOUT_HEALTHREPORT_URL }}};

        
        checkUrl(urls);

        

        loadAndPaint(StringHelper.ABOUT_ABOUT_URL);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        
        mSolo.sleep(AFTER_BACK_SLEEP_MS);

        
        loadAndPaint(StringHelper.ABOUT_SCHEME);
        verifyUrl(StringHelper.ABOUT_SCHEME); 

        
        loadItemsByLevel(menuItems);
    }

    
    public void checkUrl(String urls []) {
        for (String url:urls) {
            if (skipItemURL(url)) {
                continue;
            }
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

            if (skipItemURL(expectedUrl)) {
                continue;
            }

            expectedTabCount++;

            
            tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
            contentEventExpecter = mActions.expectGeckoEvent("DOMContentLoaded");
            selectMenuItemByPath(pathToItem);

            
            if (StringHelper.ABOUT_SCHEME.equals(expectedUrl)) {
                waitForPaint(paintExpecter); 
                paintExpecter.unregisterListener();
            } else {
                tabEventExpecter.blockForEvent();
                contentEventExpecter.blockForEvent();
            }
            tabEventExpecter.unregisterListener();
            contentEventExpecter.unregisterListener();

            verifyUrl(expectedUrl);
            if (StringHelper.ABOUT_SCHEME.equals(expectedUrl)) {
                
                expectedTabCount--;
            }
            verifyTabCount(expectedTabCount);
        }
    }

    private boolean skipItemURL(String item) {
        if (StringHelper.ABOUT_PASSWORDS_URL.equals(item) && !AppConstants.NIGHTLY_BUILD) {
            return true;
        }
        return false;
    }
}
