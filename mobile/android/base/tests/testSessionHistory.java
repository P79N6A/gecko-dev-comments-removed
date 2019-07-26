package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.tests.helpers.*;




public class testSessionHistory extends UITest {
    public void testSessionHistory() {
        GeckoHelper.blockForReady();

        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_02_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_02_TITLE);

        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_03_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_03_TITLE);

        NavigationHelper.goBack();
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_02_TITLE);

        NavigationHelper.goBack();
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        






    }
}
