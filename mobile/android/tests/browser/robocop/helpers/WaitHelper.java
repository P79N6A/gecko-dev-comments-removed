



package org.mozilla.gecko.tests.helpers;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertNotNull;
import static org.mozilla.gecko.tests.helpers.AssertionHelper.fAssertTrue;

import android.os.SystemClock;
import java.util.regex.Pattern;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Actions.EventExpecter;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.UITestContext.ComponentType;
import org.mozilla.gecko.tests.components.ToolbarComponent;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;




public final class WaitHelper {
    
    
    
    
    private static final int DEFAULT_MAX_WAIT_MS = 15000;
    private static final int PAGE_LOAD_WAIT_MS = 10000;
    private static final int CHANGE_WAIT_MS = 15000;

    
    private static final ChangeVerifier[] PAGE_LOAD_VERIFIERS = new ChangeVerifier[] {
        new ToolbarTitleTextChangeVerifier()
    };

    private static UITestContext sContext;
    private static Solo sSolo;
    private static Actions sActions;

    private static ToolbarComponent sToolbar;

    private WaitHelper() {  }

    protected static void init(final UITestContext context) {
        sContext = context;
        sSolo = context.getSolo();
        sActions = context.getActions();

        sToolbar = (ToolbarComponent) context.getComponent(ComponentType.TOOLBAR);
    }

    



    public static void waitFor(String message, final Condition condition) {
        message = "Waiting for " + message + ".";
        fAssertTrue(message, sSolo.waitForCondition(condition, DEFAULT_MAX_WAIT_MS));
    }

    



    public static void waitFor(String message, final Condition condition, final int waitMillis) {
        message = "Waiting for " + message + " with timeout " + waitMillis + ".";
        fAssertTrue(message, sSolo.waitForCondition(condition, waitMillis));
    }

    



    public static void waitForPageLoad(final Runnable initiatingAction) {
        fAssertNotNull("initiatingAction is not null", initiatingAction);

        
        
        
        
        final ChangeVerifier[] pageLoadVerifiers = PAGE_LOAD_VERIFIERS;
        for (final ChangeVerifier verifier : pageLoadVerifiers) {
            verifier.storeState();
        }

        
        final EventExpecter[] eventExpecters = new EventExpecter[] {
            sActions.expectGeckoEvent("DOMContentLoaded"),
            sActions.expectGeckoEvent("DOMTitleChanged")
        };

        initiatingAction.run();

        
        final long expecterStartMillis = SystemClock.uptimeMillis();
        for (final EventExpecter expecter : eventExpecters) {
            final int eventWaitTimeMillis = PAGE_LOAD_WAIT_MS - (int)(SystemClock.uptimeMillis() - expecterStartMillis);
            expecter.blockForEventDataWithTimeout(eventWaitTimeMillis);
            expecter.unregisterListener();
        }

        
        final long verifierStartMillis = SystemClock.uptimeMillis();

        
        for (final ChangeVerifier verifier : pageLoadVerifiers) {
            
            
            
            final int verifierWaitMillis = CHANGE_WAIT_MS - (int)(SystemClock.uptimeMillis() - verifierStartMillis);
            final boolean hasTimedOut = !sSolo.waitForCondition(new Condition() {
                @Override
                public boolean isSatisfied() {
                    return verifier.hasStateChanged();
                }
            }, verifierWaitMillis);

            sContext.dumpLog(verifier.getLogTag(),
                    (hasTimedOut ? "timed out." : "was satisfied."));
        }
    }

    




    private interface ChangeVerifier {
        public String getLogTag();

        





        public void storeState();
        public boolean hasStateChanged();
    }

    private static class ToolbarTitleTextChangeVerifier implements ChangeVerifier {
        private static final String LOGTAG = ToolbarTitleTextChangeVerifier.class.getSimpleName();

        
        private static final Pattern LOADING_PREFIX = Pattern.compile("[A-Za-z]{3,9}://");

        private CharSequence mOldTitleText;

        @Override
        public String getLogTag() {
            return LOGTAG;
        }

        @Override
        public void storeState() {
            mOldTitleText = sToolbar.getPotentiallyInconsistentTitle();
            sContext.dumpLog(LOGTAG, "stored title, \"" + mOldTitleText + "\".");
        }

        @Override
        public boolean hasStateChanged() {
            
            
            final CharSequence title = sToolbar.getPotentiallyInconsistentTitle();

            
            
            
            
            
            final boolean isLoading = LOADING_PREFIX.matcher(title).lookingAt();
            final boolean hasStateChanged = !isLoading && !mOldTitleText.equals(title);

            if (hasStateChanged) {
                sContext.dumpLog(LOGTAG, "state changed to title, \"" + title + "\".");
            }
            return hasStateChanged;
        }
    }
}
