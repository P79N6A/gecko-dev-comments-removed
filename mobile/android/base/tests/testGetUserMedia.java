package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

import android.app.Activity;
import android.hardware.Camera;
import android.os.Build;
import java.lang.reflect.Method;

public class testGetUserMedia extends BaseTest {
    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    public void testGetUserMedia() {
        String GUM_URL = getAbsoluteUrl("/robocop/robocop_getusermedia.html");

        String GUM_MESSAGE = "Would you like to share your camera and microphone with";
        String GUM_ALLOW = "Share";
        String GUM_DENY = "Don't share";

        blockForGeckoReady();

        
        
        
        
        
        if (Build.VERSION.SDK_INT >= 9) {
            if (Camera.getNumberOfCameras() > 0) {
                
                inputAndLoadUrl(GUM_URL);
                waitForText(GUM_MESSAGE);
                mAsserter.is(mSolo.searchText(GUM_MESSAGE), true, "GetUserMedia doorhanger has been displayed");
            }
        }
    }
}
