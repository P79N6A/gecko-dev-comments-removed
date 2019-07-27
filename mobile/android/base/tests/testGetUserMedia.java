package org.mozilla.gecko.tests;

import android.hardware.Camera;
import android.os.Build;

public class testGetUserMedia extends BaseTest {
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
