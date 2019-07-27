package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.helpers.DeviceHelper;
import org.mozilla.gecko.tests.helpers.GeckoClickHelper;
import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.NavigationHelper;
import org.mozilla.gecko.tests.helpers.WaitHelper;





public class testStateWhileLoading extends UITest {
    public void testStateWhileLoading() {
        if (!DeviceHelper.isTablet()) {
            
            return;
        }

        GeckoHelper.blockForReady();

        NavigationHelper.enterAndLoadUrl(mStringHelper.ROBOCOP_LINK_TO_SLOW_LOADING);

        GeckoClickHelper.openCentralizedLinkInNewTab();

        WaitHelper.waitForPageLoad(new Runnable() {
            @Override
            public void run() {
                mTabStrip.switchToTab(1);

                
                
                mToolbar.assertBackButtonIsNotEnabled();
            }
        });

        
        mToolbar.assertBackButtonIsNotEnabled();
    }
}
