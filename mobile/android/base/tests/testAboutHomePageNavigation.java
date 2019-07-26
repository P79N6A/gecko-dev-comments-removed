package org.mozilla.gecko.tests;

import org.mozilla.gecko.tests.components.AboutHomeComponent.PanelType;
import org.mozilla.gecko.tests.helpers.DeviceHelper;
import org.mozilla.gecko.tests.helpers.GeckoHelper;






public class testAboutHomePageNavigation extends UITest {
    
    
    
    public void testAboutHomePageNavigation() {
        GeckoHelper.blockForDelayedStartup();

        mAboutHome.assertVisible()
                  .assertCurrentPanel(PanelType.TOP_SITES);

        mAboutHome.swipeToPanelOnRight();
        mAboutHome.assertCurrentPanel(PanelType.BOOKMARKS);

        mAboutHome.swipeToPanelOnRight();
        mAboutHome.assertCurrentPanel(PanelType.READING_LIST);

        
        
        if (DeviceHelper.isTablet()) {
            helperTestTablet();
        } else {
            helperTestPhone();
        }
    }

    private void helperTestTablet() {
        mAboutHome.swipeToPanelOnRight();
        mAboutHome.assertCurrentPanel(PanelType.HISTORY);

        
        mAboutHome.swipeToPanelOnRight();
        mAboutHome.assertCurrentPanel(PanelType.HISTORY);

        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.READING_LIST);

        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.BOOKMARKS);

        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.TOP_SITES);

        
        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.TOP_SITES);
    }

    private void helperTestPhone() {
        
        mAboutHome.swipeToPanelOnRight();
        mAboutHome.assertCurrentPanel(PanelType.READING_LIST);

        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.BOOKMARKS);

        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.TOP_SITES);

        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.HISTORY);

        
        mAboutHome.swipeToPanelOnLeft();
        mAboutHome.assertCurrentPanel(PanelType.HISTORY);

        mAboutHome.swipeToPanelOnRight();
        mAboutHome.assertCurrentPanel(PanelType.TOP_SITES);
    }

    
    































}
