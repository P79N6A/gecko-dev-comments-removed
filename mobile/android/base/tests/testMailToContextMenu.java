



package org.mozilla.gecko.tests;

public class testMailToContextMenu extends ContentContextMenuTest {

    
    private static String MAILTO_PAGE_URL;
    private static final String mailtoMenuItems [] = {"Copy Email Address", "Share Email Address"};

    public void testMailToContextMenu() {
        final String MAILTO_PAGE_TITLE = mStringHelper.ROBOCOP_BIG_MAILTO_TITLE;

        blockForGeckoReady();

        MAILTO_PAGE_URL=getAbsoluteUrl(mStringHelper.ROBOCOP_BIG_MAILTO_URL);
        loadUrlAndWait(MAILTO_PAGE_URL);
        waitForText(MAILTO_PAGE_TITLE);

        verifyContextMenuItems(mailtoMenuItems);
        verifyCopyOption(mailtoMenuItems[0], "foo.bar@example.com"); 
        verifyShareOption(mailtoMenuItems[1], MAILTO_PAGE_TITLE); 
    }
}
