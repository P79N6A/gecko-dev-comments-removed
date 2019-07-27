package org.mozilla.gecko.tests;

import android.hardware.Camera;
import android.os.Build;

public class testGetUserMedia extends BaseTest {
    public void testGetUserMedia() {
        String GUM_CAMERA_URL = getAbsoluteUrl("/robocop/robocop_getusermedia2.html");
        String GUM_TAB_URL = getAbsoluteUrl("/robocop/robocop_getusermedia.html");
        
        String GUM_TAB_HTTPS_URL = GUM_TAB_URL.replace("http://mochi.test:8888", "https://example.com");

        String GUM_MESSAGE = "Would you like to share your camera and microphone with";
        String GUM_ALLOW = "^Share$";
        String GUM_DENY = "^Don't Share$";

        String GUM_BACK_CAMERA = "Back facing camera";

        String GUM_PAGE_FAILED = "failed gumtest";
        String GUM_PAGE_AUDIO = "audio gumtest";
        String GUM_PAGE_VIDEO = "video gumtest";
        String GUM_PAGE_AUDIOVIDEO = "audiovideo gumtest";

        blockForGeckoReady();

        
        if (Camera.getNumberOfCameras() <= 0) {
            return;
        }

        
        
        

        
        inputAndLoadUrl(GUM_CAMERA_URL);
        waitForText(GUM_MESSAGE);
        
        mAsserter.is(mSolo.searchText(GUM_BACK_CAMERA), true, "getUserMedia found a camera");
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "getUserMedia doorhanger has been displayed");
        mSolo.clickOnButton(GUM_DENY);
        verifyPageTitle(GUM_PAGE_FAILED, GUM_CAMERA_URL);

        
        inputAndLoadUrl(GUM_TAB_HTTPS_URL);
        waitForText(GUM_MESSAGE);
        mSolo.clickOnText("Microphone 1");
        mSolo.clickOnText("No Audio");
        mSolo.clickOnButton(GUM_ALLOW);
        mSolo.clickOnText("gUM Test Page");
        verifyPageTitle(GUM_PAGE_VIDEO, GUM_TAB_HTTPS_URL);

        
        
        
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            return;
        }

        inputAndLoadUrl(GUM_TAB_HTTPS_URL);
        waitForText(GUM_MESSAGE);
        mSolo.clickOnButton(GUM_ALLOW);
        mSolo.clickOnText("gUM Test Page");
        verifyPageTitle(GUM_PAGE_AUDIOVIDEO, GUM_TAB_HTTPS_URL);

        inputAndLoadUrl(GUM_TAB_HTTPS_URL);
        waitForText(GUM_MESSAGE);
        mSolo.clickOnText("Choose a tab to stream");
        mSolo.clickOnText("No Video");
        mSolo.clickOnButton(GUM_ALLOW);
        verifyPageTitle(GUM_PAGE_AUDIO, GUM_TAB_HTTPS_URL);
    }
}
