package org.mozilla.gecko.tests;

import android.hardware.Camera;
import android.os.Build;

public class testGetUserMedia extends BaseTest {
    public void testGetUserMedia() {
        String GUM_URL = getAbsoluteUrl("/robocop/robocop_getusermedia.html");

        String GUM_MESSAGE = "Would you like to share your camera and microphone with";
        String GUM_ALLOW = "^Share$";
        String GUM_DENY = "^Don't Share$";

        String GUM_PAGE_FAILED = "failed gumtest";
        String GUM_PAGE_AUDIO = "audio gumtest";
        String GUM_PAGE_VIDEO = "video gumtest";
        String GUM_PAGE_AUDIOVIDEO = "audiovideo gumtest";

        blockForGeckoReady();

        
        
        
        
        
        if (Build.VERSION.SDK_INT < 9) {
            return;
        }

        if (Camera.getNumberOfCameras() <= 0) {
            return;
        }
        
        inputAndLoadUrl(GUM_URL);
        waitForText(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "GetUserMedia doorhanger has been displayed");
        mSolo.clickOnButton(GUM_DENY);
        verifyPageTitle(GUM_PAGE_FAILED);

        inputAndLoadUrl(GUM_URL);
        waitForText(GUM_MESSAGE);
        
        mSolo.clickOnText("Back facing camera");
        mSolo.clickOnText("Choose a tab to stream");
        mSolo.clickOnButton(GUM_ALLOW);
        mSolo.clickOnText("gUM Test Page");
        verifyPageTitle(GUM_PAGE_AUDIOVIDEO);

        inputAndLoadUrl(GUM_URL);
        waitForText(GUM_MESSAGE);
        mSolo.clickOnText("Back facing camera");
        mSolo.clickOnText("No Video");
        mSolo.clickOnButton(GUM_ALLOW);
        verifyPageTitle(GUM_PAGE_AUDIO);

        inputAndLoadUrl(GUM_URL);
        waitForText(GUM_MESSAGE);
        
        mSolo.clickOnText("Back facing camera");
        mSolo.clickOnText("Choose a tab to stream");
        mSolo.clickOnText("Microphone 1");
        mSolo.clickOnText("No Audio");
        mSolo.clickOnButton(GUM_ALLOW);
        mSolo.clickOnText("gUM Test Page");
        verifyPageTitle(GUM_PAGE_VIDEO);
    }
}
