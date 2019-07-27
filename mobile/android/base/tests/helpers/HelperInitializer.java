



package org.mozilla.gecko.tests.helpers;

import org.mozilla.gecko.tests.UITestContext;






public final class HelperInitializer {

    private HelperInitializer() {  }

    public static void init(final UITestContext context) {
        
        AssertionHelper.init(context);

        DeviceHelper.init(context);
        GeckoClickHelper.init(context);
        GeckoHelper.init(context);
        JavascriptBridge.init(context);
        NavigationHelper.init(context);
        WaitHelper.init(context);
    }
}
