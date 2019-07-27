



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

    protected static void init(final UITestContext context) {
        sActivity = context.getActivity();
        sActions = context.getActions();
    }

    public static void blockForReady() {
        blockForEvent("Gecko:Ready");
    }

    



    public static void blockForDelayedStartup() {
        blockForEvent("Gecko:DelayedStartup");
    }

    private static void blockForEvent(final String eventName) {
        final EventExpecter eventExpecter = sActions.expectGeckoEvent(eventName);

        final boolean isRunning = GeckoThread.checkLaunchState(LaunchState.GeckoRunning);
        if (!isRunning) {
            eventExpecter.blockForEvent();
        }

        eventExpecter.unregisterListener();
    }
}
