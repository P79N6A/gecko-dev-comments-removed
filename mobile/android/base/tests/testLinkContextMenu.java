package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

public class testLinkContextMenu extends ContentContextMenuTest {

    
    private static String LINK_PAGE_URL;
    private static String BLANK_PAGE_URL;
    private static final String LINK_PAGE_TITLE = "Big Link";
    private static final String linkMenuItems [] = { "Open Link in New Tab", "Open Link in Private Tab", "Copy Link", "Share Link", "Bookmark Link"};

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testLinkContextMenu() {
        blockForGeckoReady();

        LINK_PAGE_URL=getAbsoluteUrl("/robocop/robocop_big_link.html");
        BLANK_PAGE_URL=getAbsoluteUrl("/robocop/robocop_blank_01.html");
        inputAndLoadUrl(LINK_PAGE_URL);
        waitForText(LINK_PAGE_TITLE);

        verifyContextMenuItems(linkMenuItems); 
        openTabFromContextMenu(linkMenuItems[0],2); 
        openTabFromContextMenu(linkMenuItems[1],2); 
        verifyCopyOption(linkMenuItems[2], BLANK_PAGE_URL); 
        verifyShareOption(linkMenuItems[3], LINK_PAGE_TITLE); 
        verifyBookmarkLinkOption(linkMenuItems[4], BLANK_PAGE_URL); 
    }

    @Override
    public void tearDown() throws Exception {
        mDatabaseHelper.deleteBookmark(BLANK_PAGE_URL);
        super.tearDown();
    }
}
