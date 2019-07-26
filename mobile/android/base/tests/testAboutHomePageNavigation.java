package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.home.HomePager.Page;
import org.mozilla.gecko.tests.helpers.*;




public class testAboutHomePageNavigation extends UITest {
    
    
    
    public void testAboutHomePageNavigation() {
        GeckoHelper.blockForReady();

        mAboutHome.assertVisible()
                  .assertCurrentPage(Page.TOP_SITES);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(Page.BOOKMARKS);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(Page.READING_LIST);

        
        
        if (DeviceHelper.isTablet()) {
            helperTestTablet();
        } else {
            helperTestPhone();
        }
    }

    private void helperTestTablet() {
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(Page.HISTORY);

        
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(Page.HISTORY);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.READING_LIST);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.BOOKMARKS);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.TOP_SITES);

        
        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.TOP_SITES);
    }

    private void helperTestPhone() {
        
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(Page.READING_LIST);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.BOOKMARKS);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.TOP_SITES);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.HISTORY);

        
        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(Page.HISTORY);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(Page.TOP_SITES);
    }

    
    































}
