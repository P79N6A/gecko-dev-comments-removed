



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;

public class testHomeBanner extends UITest {

    private static final String TEST_URL = "chrome://roboextender/content/robocop_home_banner.html";
    private static final String TEXT = "The quick brown fox jumps over the lazy dog.";

    public void testHomeBanner() {
        GeckoHelper.blockForReady();

        
        mAboutHome.assertVisible()
                  .assertBannerNotVisible();

        
        addBannerTest();

        
        hideOnToolbarFocusTest();

        
        
        dismissBannerTest();
    }

    





    private void addBannerTest() {
        
        Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageShown");
        addBannerMessage();
        NavigationHelper.enterAndLoadUrl(mStringHelper.ABOUT_HOME_URL);
        eventExpecter.blockForEvent();

        
        mAboutHome.assertBannerText(TEXT);

        
        NavigationHelper.enterAndLoadUrl(mStringHelper.ABOUT_FIREFOX_URL);
        mAboutHome.assertBannerNotVisible();
    }


    private void hideOnToolbarFocusTest() {
        NavigationHelper.enterAndLoadUrl(mStringHelper.ABOUT_HOME_URL);
        mAboutHome.assertVisible()
                  .assertBannerVisible();

        mToolbar.enterEditingMode();
        mAboutHome.assertBannerNotVisible();

        mToolbar.dismissEditingMode();
        mAboutHome.assertBannerVisible();
    }

    





    private void dismissBannerTest() {
        NavigationHelper.enterAndLoadUrl(mStringHelper.ABOUT_HOME_URL);
        mAboutHome.assertVisible();

        
        final Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageDismissed");
        mAboutHome.dismissBanner();
        eventExpecter.blockForEvent();

        mAboutHome.assertBannerNotVisible();
    }

    


    private void addBannerMessage() {
        final Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageAdded");
        NavigationHelper.enterAndLoadUrl(TEST_URL + "#addMessage");
        eventExpecter.blockForEvent();
    }
}
