


package org.mozilla.gecko.harness;

import junit.framework.AssertionFailedError;
import junit.framework.Test;
import junit.framework.TestListener;
import android.util.Log;










public class BrowserTestListener implements TestListener {
    public static final String LOG_TAG = "BTestListener";

    @Override
    public void startTest(Test test) {
        Log.d(LOG_TAG, "startTest: " + test);
    }

    @Override
    public void endTest(Test test) {
        Log.d(LOG_TAG, "endTest: " + test);
    }

    @Override
    public void addFailure(Test test, AssertionFailedError t) {
        Log.d(LOG_TAG, "addFailure: " + test);
    }

    @Override
    public void addError(Test test, Throwable t) {
        Log.d(LOG_TAG, "addError: " + test);
    }
}
