



package org.mozilla.gecko.tests;

import org.mozilla.gecko.AppConstants;

import android.widget.Spinner;
import android.view.View;

import com.jayway.android.robotium.solo.Condition;

import android.hardware.Camera;
import android.os.Build;

public class testGetUserMedia extends BaseTest {
    private static final String LOGTAG = testGetUserMedia.class.getSimpleName();

    private static final String GUM_MESSAGE = "Would you like to share your camera and microphone with";
    private static final String GUM_ALLOW = "^Share$";
    private static final String GUM_DENY = "^Don't Share$";

    private static final String GUM_BACK_CAMERA = "Back facing camera";
    private static final String GUM_SELECT_TAB = "Choose a tab to stream";

    private static final String GUM_PAGE_TITLE = "gUM Test Page";
    private static final String GUM_PAGE_FAILED = "failed gumtest";
    private static final String GUM_PAGE_AUDIO = "audio gumtest";
    private static final String GUM_PAGE_VIDEO = "video gumtest";
    private static final String GUM_PAGE_AUDIOVIDEO = "audiovideo gumtest";

    public void testGetUserMedia() {
        
        if (AppConstants.RELEASE_BUILD) {
            mAsserter.dumpLog(LOGTAG + " is disabled on release builds: returning");
            return;
        }

        
        if (Camera.getNumberOfCameras() <= 0) {
            return;
        }

        blockForGeckoReady();

        final String GUM_CAMERA_URL = getAbsoluteUrl("/robocop/robocop_getusermedia2.html");
        final String GUM_TAB_URL = getAbsoluteUrl("/robocop/robocop_getusermedia.html");
        
        final String GUM_TAB_HTTPS_URL = GUM_TAB_URL.replace("http://mochi.test:8888", "https://example.com");

        
        
        

        
        loadUrlAndWait(GUM_CAMERA_URL);
        waitForText(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "getUserMedia doorhanger has been displayed");
        waitForSpinner();
        
        mAsserter.is(mSolo.searchText(GUM_BACK_CAMERA), true, "getUserMedia found a camera");
        mSolo.clickOnButton(GUM_DENY);
        waitForTextDismissed(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), false, "getUserMedia doorhanger hidden after dismissal");
        verifyUrlBarTitle(GUM_CAMERA_URL);

        
        loadUrlAndWait(GUM_TAB_HTTPS_URL);
        waitForText(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "getUserMedia doorhanger has been displayed");
        waitForSpinner();
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), true, "Video source selection available");
        mAsserter.is(mSolo.searchText("MICROPHONE TO USE"), true, "Microphone selection available");
        mAsserter.is(mSolo.searchText("Microphone 1"), true, "Microphone 1 available");
        mSolo.clickOnText("Microphone 1");
        waitForText("No Audio");
        mAsserter.is(mSolo.searchText("No Audio"), true, "No 'No Audio' selection available");
        mSolo.clickOnText("No Audio");
        waitForTextDismissed("Microphone 1");
        mAsserter.is(mSolo.searchText("Microphone 1"), false, "Audio selection hidden after dismissal");
        mAsserter.is(mSolo.searchText(GUM_ALLOW), true, "Share button available after selection");
        mSolo.clickOnButton(GUM_ALLOW);
        waitForTextDismissed(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), false, "getUserMedia doorhanger hidden after dismissal");
        waitForText(GUM_SELECT_TAB);
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), true, "Tab selection dialog displayed");
        mSolo.clickOnText(GUM_PAGE_TITLE);
        waitForTextDismissed(GUM_SELECT_TAB);
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), false, "Tab selection dialog hidden");
        verifyUrlBarTitle(GUM_TAB_HTTPS_URL);

        
        
        
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
            return;
        }

        loadUrlAndWait(GUM_TAB_HTTPS_URL);
        waitForText(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "getUserMedia doorhanger has been displayed");

        waitForSpinner();
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), true, "Video source selection available");
        mSolo.clickOnButton(GUM_ALLOW);
        waitForTextDismissed(GUM_MESSAGE);
        waitForText(GUM_SELECT_TAB);
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), true, "Tab selection dialog displayed");
        mSolo.clickOnText(GUM_PAGE_TITLE);
        waitForTextDismissed(GUM_SELECT_TAB);
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), false, "Tab selection dialog hidden");
        verifyUrlBarTitle(GUM_TAB_HTTPS_URL);

        loadUrlAndWait(GUM_TAB_HTTPS_URL);
        waitForText(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "getUserMedia doorhanger has been displayed");

        waitForSpinner();
        mAsserter.is(mSolo.searchText(GUM_SELECT_TAB), true, "Video source selection available");
        mSolo.clickOnText(GUM_SELECT_TAB);
        waitForText("No Video");
        mAsserter.is(mSolo.searchText("No Video"), true, "'No video' source selection available");
        mSolo.clickOnText("No Video");
        waitForTextDismissed(GUM_SELECT_TAB);
        mSolo.clickOnButton(GUM_ALLOW);
        waitForTextDismissed(GUM_MESSAGE);
        mAsserter.is(mSolo.searchText(GUM_MESSAGE), false, "getUserMedia doorhanger hidden after dismissal");
        verifyUrlBarTitle(GUM_TAB_HTTPS_URL);
    }

    
    private void waitForSpinner() {
        waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                for (Spinner view : mSolo.getCurrentViews(Spinner.class)) {
                    if (view.isClickable() &&
                        view.getVisibility() == View.VISIBLE &&
                        view.getWidth() > 0 &&
                        view.getHeight() > 0) {
                        return true;
                    }
                }
                return false;
            }
        }, MAX_WAIT_MS);
    }

    
    private void waitForTextDismissed(final String text) {
        waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return !mSolo.searchText(text);
            }
        }, MAX_WAIT_MS);
    }
}
