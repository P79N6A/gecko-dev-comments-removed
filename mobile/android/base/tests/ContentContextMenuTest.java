package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;
import org.mozilla.gecko.util.Clipboard;

import android.content.ContentResolver;
import android.util.DisplayMetrics;





abstract class ContentContextMenuTest extends PixelTest {
    private static final int MAX_TEST_TIMEOUT = 10000;

    
    protected void openWebContentContextMenu(String waitText) {
        DisplayMetrics dm = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(dm);

        
        float top = mDriver.getGeckoTop() + 30 * dm.density;
        float left = mDriver.getGeckoLeft() + mDriver.getGeckoWidth() / 2;

        mAsserter.dumpLog("long-clicking at "+left+", "+top);
        mSolo.clickLongOnScreen(left, top);
        waitForText(waitText);
    }

    protected void verifyContextMenuItems(String[] items) {
        
        openWebContentContextMenu(items[0]);
        for (String option:items) {
            mAsserter.ok(mSolo.searchText(option), "Checking that the option: " + option + " is available", "The option is available");
        }
    }

    protected void openTabFromContextMenu(String contextMenuOption, int expectedTabCount) {
        if (!mSolo.searchText(contextMenuOption)) {
            openWebContentContextMenu(contextMenuOption); 
        }
        Actions.EventExpecter tabEventExpecter = mActions.expectGeckoEvent("Tab:Added");
        mSolo.clickOnText(contextMenuOption);
        tabEventExpecter.blockForEvent();
        tabEventExpecter.unregisterListener();
        verifyTabCount(expectedTabCount);
    }

    protected void verifyCopyOption(String copyOption, final String copiedText) {
        if (!mSolo.searchText(copyOption)) {
            openWebContentContextMenu(copyOption); 
        }
        mSolo.clickOnText(copyOption);
        boolean correctText = waitForTest(new BooleanTest() {
            @Override
            public boolean test() {
                final String clipboardText = Clipboard.getText();
                mAsserter.dumpLog("Clipboard text = " + clipboardText + " , expected text = " + copiedText);
                return clipboardText.contains(copiedText);
            }
        }, MAX_TEST_TIMEOUT);
        mAsserter.ok(correctText, "Checking if the text is correctly copied", "The text was correctly copied");
    }



    protected void verifyShareOption(String shareOption, String pageTitle) {
        waitForText(pageTitle);
        openWebContentContextMenu(shareOption);
        mSolo.clickOnText(shareOption);
        mAsserter.ok(waitForText("Share via"), "Checking that the share pop-up is displayed", "The pop-up has been displayed");

        
        mActions.sendSpecialKey(Actions.SpecialKey.BACK);
        waitForText(pageTitle);
    }

    protected void verifyBookmarkLinkOption(String bookmarkOption, String link) {
        openWebContentContextMenu(bookmarkOption);
        mSolo.clickOnText(bookmarkOption);
        mAsserter.ok(waitForText("Bookmark added"), "Waiting for the Bookmark added toaster notification", "The notification has been displayed");
        mAsserter.ok(mDatabaseHelper.isBookmark(link), "Checking if the link has been added as a bookmark", "The link has been bookmarked");
    }
}
