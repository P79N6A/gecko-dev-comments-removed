package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.AppConstants;




public class testSystemPages extends PixelTest {
    final int mExpectedTabCount = 1;
    private static final int AFTER_BACK_SLEEP_MS = 500;

    public void testSystemPages() {
        blockForGeckoReady();

        final String urls [] = { mStringHelper.ABOUT_FIREFOX_URL, mStringHelper.ABOUT_RIGHTS_URL,
                mStringHelper.ABOUT_ADDONS_URL, mStringHelper.ABOUT_DOWNLOADS_URL, StringHelper.ABOUT_PASSWORDS_URL,
                mStringHelper.ABOUT_BUILDCONFIG_URL, mStringHelper.ABOUT_FEEDBACK_URL,
                mStringHelper.ABOUT_HEALTHREPORT_URL, mStringHelper.ABOUT_SCHEME
        };
        
        String menuItems [][][] = {{{ mStringHelper.APPS_LABEL }, { mStringHelper.ABOUT_APPS_URL }},
                                  {{ mStringHelper.DOWNLOADS_LABEL }, { mStringHelper.ABOUT_DOWNLOADS_URL}},
                                  {{ mStringHelper.LOGINS_LABEL}, { StringHelper.ABOUT_PASSWORDS_URL }},
                                  {{ mStringHelper.ADDONS_LABEL }, { mStringHelper.ABOUT_ADDONS_URL }},
                                  {{ mStringHelper.SETTINGS_LABEL, mStringHelper.MOZILLA_SECTION_LABEL, mStringHelper.ABOUT_LABEL }, { mStringHelper.ABOUT_SCHEME }},
                                  {{ mStringHelper.SETTINGS_LABEL, mStringHelper.MOZILLA_SECTION_LABEL, mStringHelper.FEEDBACK_LABEL }, { mStringHelper.ABOUT_FEEDBACK_URL }},
                                  {{ mStringHelper.SETTINGS_LABEL, mStringHelper.MOZILLA_SECTION_LABEL, mStringHelper.MY_HEALTH_REPORT_LABEL }, { mStringHelper.ABOUT_HEALTHREPORT_URL }}};

        
        checkUrl(urls);

        

        loadAndPaint(mStringHelper.ABOUT_ABOUT_URL);

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        
        mSolo.sleep(AFTER_BACK_SLEEP_MS);

        
        loadAndPaint(mStringHelper.ABOUT_SCHEME);
        verifyUrl(mStringHelper.ABOUT_SCHEME); 

        
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

            
            if (mStringHelper.ABOUT_SCHEME.equals(expectedUrl)) {
                waitForPaint(paintExpecter); 
                paintExpecter.unregisterListener();
            } else {
                tabEventExpecter.blockForEvent();
                contentEventExpecter.blockForEvent();
            }
            tabEventExpecter.unregisterListener();
            contentEventExpecter.unregisterListener();

            verifyUrl(expectedUrl);
            if (mStringHelper.ABOUT_SCHEME.equals(expectedUrl)) {
                
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
