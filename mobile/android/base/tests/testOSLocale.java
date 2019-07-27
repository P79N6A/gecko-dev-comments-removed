



package org.mozilla.gecko.tests;

import org.mozilla.gecko.Actions;

public class testOSLocale extends JavascriptTest {
    private static final String GECKO_DELAYED_STARTUP = "Gecko:DelayedStartup";

    public testOSLocale() {
        super("testOSLocale.js");
    }

    @Override
    public void testJavascript() throws Exception {
        final Actions.EventExpecter expecter = mActions.expectGeckoEvent(GECKO_DELAYED_STARTUP);
        expecter.blockForEvent(MAX_WAIT_MS, true);

        
        
        super.doTestJavascript();
    }
}
