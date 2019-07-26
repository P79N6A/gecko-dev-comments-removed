package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.R;
import org.mozilla.gecko.tests.helpers.*;

import android.view.View;

public class testHomeBanner extends UITest {

    private static final String TEST_URL = "chrome://roboextender/content/robocop_home_banner.html";
    private static final String TEXT = "The quick brown fox jumps over the lazy dog.";

    public void testHomeBanner() {
        GeckoHelper.blockForReady();

        
        mAboutHome.assertVisible()
                  .assertBannerNotVisible();

        addBannerMessage();

        
        Actions.EventExpecter eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageShown");
        NavigationHelper.enterAndLoadUrl("about:home");
        eventExpecter.blockForEvent();

        
        mAboutHome.assertBannerText(TEXT);

        
        eventExpecter = getActions().expectGeckoEvent("TestHomeBanner:MessageClicked");
        mAboutHome.clickOnBanner();
        eventExpecter.blockForEvent();

        
        NavigationHelper.enterAndLoadUrl("about:firefox");

        
        
        final View banner = getActivity().findViewById(R.id.home_banner);
        assertTrue("The HomeBanner is not visible", banner == null || banner.getVisibility() != View.VISIBLE);

        removeBannerMessage();

        
        NavigationHelper.enterAndLoadUrl("about:home");
        mAboutHome.assertVisible()
                  .assertBannerNotVisible();
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
