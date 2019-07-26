package org.mozilla.gecko.tests;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.tests.components.AboutHomeComponent.PageType;
import org.mozilla.gecko.tests.helpers.*;




public class testAboutHomePageNavigation extends UITest {
    
    
    
    public void testAboutHomePageNavigation() {
        GeckoHelper.blockForReady();

        mAboutHome.assertVisible()
                  .assertCurrentPage(PageType.TOP_SITES);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PageType.BOOKMARKS);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PageType.READING_LIST);

        
        
        if (DeviceHelper.isTablet()) {
            helperTestTablet();
        } else {
            helperTestPhone();
        }
    }

    private void helperTestTablet() {
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PageType.HISTORY);

        
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PageType.HISTORY);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.READING_LIST);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.BOOKMARKS);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.TOP_SITES);

        
        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.TOP_SITES);
    }

    private void helperTestPhone() {
        
        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PageType.READING_LIST);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.BOOKMARKS);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.TOP_SITES);

        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.HISTORY);

        
        mAboutHome.swipeToPageOnLeft();
        mAboutHome.assertCurrentPage(PageType.HISTORY);

        mAboutHome.swipeToPageOnRight();
        mAboutHome.assertCurrentPage(PageType.TOP_SITES);
    }

    
    































}
