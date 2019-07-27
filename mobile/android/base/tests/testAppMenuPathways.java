



package org.mozilla.gecko.tests;

import org.json.JSONObject;
import org.mozilla.gecko.Tabs;
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

        
        final JSONObject message = new JSONObject();
        try {
            message.put("contentType", "video/webm");
            message.put("baseDomain", "webmfiles.org");
            message.put("type", "Content:LocationChange");
            message.put("sameDocument", false);
            message.put("userRequested", "");
            message.put("uri", getAbsoluteIpUrl("/big-buck-bunny_trailer.webm"));
            message.put("tabID", 0);
        } catch (Exception ex) {
            mAsserter.ok(false, "exception in testSaveAsPDFPathway", ex.toString());
        }

        
        Tabs.getInstance().handleMessage("Content:LocationChange", message);

        
        mAppMenu.assertMenuItemIsDisabledAndVisible(AppMenuComponent.PageMenuItem.SAVE_AS_PDF);

        
        
        NavigationHelper.enterAndLoadUrl(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);
        mToolbar.assertTitle(mStringHelper.ROBOCOP_BLANK_PAGE_01_URL);

        
        
        
        mAppMenu.pressMenuItem(AppMenuComponent.PageMenuItem.SAVE_AS_PDF);
    }
}
