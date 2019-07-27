package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.components.AppMenuComponent;
import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;




public class testAppMenuPathways extends UITest {

    



    public void testAppMenuPathways() {
        GeckoHelper.blockForReady();

        _testSaveAsPDFPathway();
    }

    public void _testSaveAsPDFPathway() {
        
        mAppMenu.assertMenuItemIsDisabledAndVisible(AppMenuComponent.PageMenuItem.SAVE_AS_PDF);

        
        NavigationHelper.enterAndLoadUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mToolbar.assertTitle(StringHelper.ROBOCOP_BLANK_PAGE_01_TITLE);

        
        
        mAppMenu.pressMenuItem(AppMenuComponent.PageMenuItem.SAVE_AS_PDF);
    }
}