



package org.mozilla.gecko.tests.helpers;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Actions.EventExpecter;
import org.mozilla.gecko.GeckoThread;
import org.mozilla.gecko.GeckoThread.LaunchState;
import org.mozilla.gecko.tests.UITestContext;

import android.app.Activity;




public final class GeckoHelper {
    private static Activity sActivity;
    private static Actions sActions;

    private GeckoHelper() {  }

    public static void init(final UITestContext context) {
        sActivity = context.getActivity();
        sActions = context.getActions();
    }

    public static void blockForReady() {
        final EventExpecter geckoReady = sActions.expectGeckoEvent("Gecko:Ready");

        final boolean isReady = GeckoThread.checkLaunchState(LaunchState.GeckoRunning);
        if (!isReady) {
            geckoReady.blockForEvent();
        }

        geckoReady.unregisterListener();
    }
}
