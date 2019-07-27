



package org.mozilla.gecko.tests;

import org.mozilla.gecko.home.HomeConfig;
import org.mozilla.gecko.home.HomeConfig.PanelType;
import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;




public class testAboutHomeVisibility extends UITest {
    public void testAboutHomeVisibility() {
        GeckoHelper.blockForReady();

        
        mToolbar.assertTitle(mStringHelper.ABOUT_HOME_URL);
        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        
        NavigationHelper.enterAndLoadUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mToolbar.assertTitle(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mAboutHome.assertNotVisible();

        
        NavigationHelper.enterAndLoadUrl(mStringHelper.ROBOCOP_BLANK_PAGE_02_URL);
        mToolbar.assertTitle(mStringHelper.ROBOCOP_BLANK_PAGE_02_URL);
        mAboutHome.assertNotVisible();

        
        mToolbar.enterEditingMode();
        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        
        mToolbar.dismissEditingMode();
        mAboutHome.assertNotVisible();

        
        NavigationHelper.enterAndLoadUrl(mStringHelper.ABOUT_HOME_URL);
        mToolbar.assertTitle(mStringHelper.ABOUT_HOME_URL);
        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        
        mAboutHome.navigateToBuiltinPanelType(PanelType.BOOKMARKS)
                  .assertVisible()
                  .assertCurrentPanel(PanelType.BOOKMARKS);
        mAboutHome.navigateToBuiltinPanelType(PanelType.HISTORY)
                  .assertVisible()
                  .assertCurrentPanel(PanelType.HISTORY);
    }
}
