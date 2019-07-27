



package org.mozilla.gecko.tests;

public class testPictureLinkContextMenu extends ContentContextMenuTest {

    
    private static String PICTURE_PAGE_URL;
    private static String BLANK_PAGE_URL;
    private static final String tabs [] = { "Image", "Link" };
    private static final String photoMenuItems [] = { "Copy Image Location", "Share Image", "Set Image As", "Save Image" };
    private static final String imageTitle = "^Image$";

    public void testPictureLinkContextMenu() {
        final String PICTURE_PAGE_TITLE = mStringHelper.ROBOCOP_PICTURE_LINK_TITLE;
        final String linkMenuItems [] = mStringHelper.CONTEXT_MENU_ITEMS_IN_NORMAL_TAB;

        blockForGeckoReady();

        PICTURE_PAGE_URL=getAbsoluteUrl(mStringHelper.ROBOCOP_PICTURE_LINK_URL);
        BLANK_PAGE_URL=getAbsoluteUrl(mStringHelper.ROBOCOP_BLANK_PAGE_02_URL);
        loadAndPaint(PICTURE_PAGE_URL);
        verifyUrlBarTitle(PICTURE_PAGE_URL);

        switchTabs(imageTitle);
        verifyContextMenuItems(photoMenuItems);
        verifyTabs(tabs);
        switchTabs(imageTitle);
        verifyCopyOption(photoMenuItems[0], "Firefox.jpg"); 
        switchTabs(imageTitle);
        verifyShareOption(photoMenuItems[1], PICTURE_PAGE_TITLE); 

        verifyContextMenuItems(linkMenuItems);
        openTabFromContextMenu(linkMenuItems[0],2); 
        openTabFromContextMenu(linkMenuItems[1],2); 
        verifyCopyOption(linkMenuItems[2], BLANK_PAGE_URL); 
        verifyShareOption(linkMenuItems[3], PICTURE_PAGE_TITLE); 
        verifyBookmarkLinkOption(linkMenuItems[4],BLANK_PAGE_URL); 
    }

    @Override
    public void tearDown() throws Exception {
        mDatabaseHelper.deleteBookmark(BLANK_PAGE_URL);
        super.tearDown();
    }
}
