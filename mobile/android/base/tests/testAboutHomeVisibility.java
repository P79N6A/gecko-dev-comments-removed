package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.home.HomePager.Page;
import org.mozilla.gecko.tests.helpers.*;




public class testAboutHomeVisibility extends UITest {
    public void testAboutHomeVisibility() {
        GeckoHelper.blockForReady();

        
        mToolbar.assertTitle(StringHelper.ABOUT_HOME_TITLE);
        mAboutHome.assertVisible()
                  .assertCurrentPage(Page.TOP_SITES);

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);
        mAboutHome.assertNotVisible();

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_02_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_02_TITLE);
        mAboutHome.assertNotVisible();

        
        mToolbar.enterEditingMode();
        mAboutHome.assertVisible()
                  .assertCurrentPage(Page.TOP_SITES);

        
        mToolbar.dismissEditingMode();
        mAboutHome.assertNotVisible();

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ABOUT_HOME_URL);
        mToolbar.assertTitle(StringHelper.ABOUT_HOME_TITLE);
        mAboutHome.assertVisible()
                  .assertCurrentPage(Page.TOP_SITES);

        
    }
}
