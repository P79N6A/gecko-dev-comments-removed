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
        NavigationHelper.enterAndLoadUrl("about:home");
        eventExpecter.blockForEvent();

        
        mAboutHome.assertBannerText(TEXT);

        
        eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageClicked");
        mAboutHome.clickOnBanner();
        eventExpecter.blockForEvent();

        
        NavigationHelper.enterAndLoadUrl("about:firefox");

        mAboutHome.assertBannerNotVisible();
    }


    




    private void removeBannerTest() {
        removeBannerMessage();

        
        NavigationHelper.enterAndLoadUrl("about:home");
        mAboutHome.assertVisible()
                  .assertBannerNotVisible();
    }

    





    private void dismissBannerTest() {
        
        addBannerMessage();

        NavigationHelper.enterAndLoadUrl("about:home");
        mAboutHome.assertVisible();

        
        final Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageDismissed");
        mAboutHome.dismissBanner();
        eventExpecter.blockForEvent();

        mAboutHome.assertBannerNotVisible();
    }

    private void hideOnToolbarFocusTest() {
        NavigationHelper.enterAndLoadUrl("about:home");
        mAboutHome.assertVisible()
                  .assertBannerVisible();

        mToolbar.enterEditingMode();
        mAboutHome.assertBannerNotVisible();

        mToolbar.dismissEditingMode();
        mAboutHome.assertBannerVisible();
    }

    


    private void addBannerMessage() {
        final Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageAdded");
        NavigationHelper.enterAndLoadUrl(TEST_URL + "#addMessage");
        eventExpecter.blockForEvent();
    }

    


    private void removeBannerMessage() {
        final Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageRemoved");
        NavigationHelper.enterAndLoadUrl(TEST_URL + "#removeMessage");
        eventExpecter.blockForEvent();
    }
}
