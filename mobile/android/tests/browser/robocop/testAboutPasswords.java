



package org.mozilla.gecko.tests;

import org.mozilla.gecko.AppConstants;

public class testAboutPasswords extends JavascriptTest {
    private static final String LOGTAG = testAboutPasswords.class.getSimpleName();

    public testAboutPasswords() {
        super("testAboutPasswords.js");
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
