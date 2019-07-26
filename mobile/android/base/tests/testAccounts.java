



package org.mozilla.gecko.tests;

import android.app.Activity;
import org.mozilla.gecko.*;
import org.mozilla.gecko.fxa.activities.FxAccountGetStartedActivity;

public class testAccounts extends JavascriptTest {
    public testAccounts() {
        super("testAccounts.js");
    }

    @Override
    public void testJavascript() throws Exception {
        super.testJavascript();
        Activity activity = mSolo.getCurrentActivity();
        System.out.println("Current activity: " + activity);
        mAsserter.ok(activity instanceof FxAccountGetStartedActivity, "checking activity", "setup activity launched");
    }
}
