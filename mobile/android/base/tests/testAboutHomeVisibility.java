package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.components.AboutHomeComponent.PanelType;
import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;




public class testAboutHomeVisibility extends UITest {
    public void testAboutHomeVisibility() {
        GeckoHelper.blockForReady();

        
        mToolbar.assertTitle(StringHelper.ABOUT_HOME_TITLE);
        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);
        mAboutHome.assertNotVisible();

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_02_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_02_TITLE);
        mAboutHome.assertNotVisible();

        
        mToolbar.enterEditingMode();
        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        
        mToolbar.dismissEditingMode();
        mAboutHome.assertNotVisible();

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ABOUT_HOME_URL);
        mToolbar.assertTitle(StringHelper.ABOUT_HOME_TITLE);
        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        
    }
}
