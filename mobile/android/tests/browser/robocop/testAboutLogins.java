



package org.mozilla.gecko.tests;

import org.mozilla.gecko.AppConstants;

public class testAboutLogins extends JavascriptTest {
    private static final String LOGTAG = testAboutLogins.class.getSimpleName();

    public testAboutLogins() {
        super("testAboutLogins.js");
    }

    @Override
    public void testJavascript() throws Exception {
        
        if (!AppConstants.NIGHTLY_BUILD) {
            mAsserter.dumpLog(LOGTAG + " is disabled on non-Nightly builds: returning");
            return;
        }
        super.testJavascript();
    }
}
