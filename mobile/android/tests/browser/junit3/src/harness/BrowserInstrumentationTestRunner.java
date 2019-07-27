


package org.mozilla.gecko.harness;

import android.os.Bundle;
import android.test.AndroidTestRunner;
import android.test.InstrumentationTestRunner;
import android.util.Log;







public class BrowserInstrumentationTestRunner extends InstrumentationTestRunner {
    private static final String LOG_TAG = "BInstTestRunner";

    @Override
    public void onCreate(Bundle arguments) {
        Log.d(LOG_TAG, "onCreate");
        super.onCreate(arguments);
    }

    @Override
    protected AndroidTestRunner getAndroidTestRunner() {
        Log.d(LOG_TAG, "getAndroidTestRunner");
        AndroidTestRunner testRunner = super.getAndroidTestRunner();
        testRunner.addTestListener(new BrowserTestListener());
        return testRunner;
    }
}
