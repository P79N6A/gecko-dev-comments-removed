package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.tests.components.AboutHomeComponent.PanelType;
import org.mozilla.gecko.tests.helpers.*;




public class testAboutHomePageNavigation extends UITest {
    
    
    
    public void testAboutHomePageNavigation() {
        GeckoHelper.blockForReady();

        mAboutHome.assertVisible()
                  .assertCurrentPage(PanelType.TOP_SITES);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PanelType.BOOKMARKS);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PanelType.READING_LIST);

        
        
        if (DeviceHelper.isTablet()) {
            helperTestTablet();
        } else {
            helperTestPhone();
        }
    }

    private void helperTestTablet() {
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PanelType.HISTORY);

        
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PanelType.HISTORY);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.READING_LIST);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.BOOKMARKS);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.TOP_SITES);

        
        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.TOP_SITES);
    }

    private void helperTestPhone() {
        
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PanelType.READING_LIST);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.BOOKMARKS);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.TOP_SITES);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.HISTORY);

        
        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PanelType.HISTORY);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PanelType.TOP_SITES);
    }

    
    































}
