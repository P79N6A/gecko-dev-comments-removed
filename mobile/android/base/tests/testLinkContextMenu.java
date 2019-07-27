package org.mozilla.gecko.tests;


public class testLinkContextMenu extends ContentContextMenuTest {

    
    private static String LINK_PAGE_URL;
    private static String BLANK_PAGE_URL;
    private static final String LINK_PAGE_TITLE = "Big Link";
    private static final String linkMenuItems [] = StringHelper.CONTEXT_MENU_ITEMS_IN_NORMAL_TAB;

    public void testLinkContextMenu() {
        blockForGeckoReady();

        LINK_PAGE_URL=getAbsoluteUrl(StringHelper.ROBOCOP_BIG_LINK_URL);
        BLANK_PAGE_URL=getAbsoluteUrl(StringHelper.ROBOCOP_BLANK_PAGE_01_URL);
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
