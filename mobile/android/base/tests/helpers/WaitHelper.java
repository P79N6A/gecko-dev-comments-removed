



package org.mozilla.gecko.tests.helpers;

import static org.mozilla.gecko.tests.helpers.AssertionHelper.*;

import org.mozilla.gecko.Actions;
import org.mozilla.gecko.Actions.EventExpecter;
import org.mozilla.gecko.tests.components.ToolbarComponent;
import org.mozilla.gecko.tests.UITestContext;
import org.mozilla.gecko.tests.UITestContext.ComponentType;

import com.jayway.android.robotium.solo.Condition;
import com.jayway.android.robotium.solo.Solo;




public final class WaitHelper {
    
    
    private static final int DEFAULT_MAX_WAIT_MS = 5000;
    private static final int PAGE_LOAD_WAIT_MS = 10000;
    private static final int CHANGE_WAIT_MS = 5000;

    
    private static final ChangeVerifier[] PAGE_LOAD_VERIFIERS = new ChangeVerifier[] {
        new ToolbarTitleTextChangeVerifier()
    };

    private static UITestContext sContext;
    private static Solo sSolo;
    private static Actions sActions;

    private static ToolbarComponent sToolbar;

    private WaitHelper() {  }

    public static void init(final UITestContext context) {
        sContext = context;
        sSolo = context.getSolo();
        sActions = context.getActions();

        sToolbar = (ToolbarComponent) context.getComponent(ComponentType.TOOLBAR);
    }

    



    public static void waitFor(String message, final Condition condition) {
        message = "Waiting for " + message + ".";
        assertTrue(message, sSolo.waitForCondition(condition, DEFAULT_MAX_WAIT_MS));
    }

    



    public static void waitFor(String message, final Condition condition, final int waitMillis) {
        message = "Waiting for " + message + " with timeout " + waitMillis + ".";
        assertTrue(message, sSolo.waitForCondition(condition, waitMillis));
    }

    



    public static void waitForPageLoad(final Runnable initiatingAction) {
        assertNotNull("initiatingAction is not null", initiatingAction);

        
        
        
        
        final ChangeVerifier[] pageLoadVerifiers = PAGE_LOAD_VERIFIERS;
        for (final ChangeVerifier verifier : pageLoadVerifiers) {
            verifier.storeState();
        }

        
        final EventExpecter contentEventExpecter = sActions.expectGeckoEvent("DOMContentLoaded");
        final EventExpecter titleEventExpecter = sActions.expectGeckoEvent("DOMTitleChanged");

        initiatingAction.run();

        contentEventExpecter.blockForEventDataWithTimeout(PAGE_LOAD_WAIT_MS);
        contentEventExpecter.unregisterListener();
        titleEventExpecter.blockForEventDataWithTimeout(PAGE_LOAD_WAIT_MS);
        titleEventExpecter.unregisterListener();

        
        for (final ChangeVerifier verifier : pageLoadVerifiers) {
            
            
            
            final boolean hasTimedOut = !sSolo.waitForCondition(new Condition() {
                @Override
                public boolean isSatisfied() {
                    return verifier.hasStateChanged();
                }
            }, CHANGE_WAIT_MS);

            if (hasTimedOut) {
                sContext.dumpLog(verifier.getClass().getName() + " timed out.");
            }
        }
    }

    




    private static interface ChangeVerifier {
        





        public void storeState();
        public boolean hasStateChanged();
    }

    private static class ToolbarTitleTextChangeVerifier implements ChangeVerifier {
        
        private static final String LOADING_REGEX = "^[A-Za-z]{3,9}://";

        private CharSequence oldTitleText;

        @Override
        public void storeState() {
            oldTitleText = sToolbar.getPotentiallyInconsistentTitle();
        }

        @Override
        public boolean hasStateChanged() {
            
            
            final CharSequence title = sToolbar.getPotentiallyInconsistentTitle();

            
            
            
            
            
            final boolean isLoading = title.toString().matches(LOADING_REGEX);
            return !isLoading && !oldTitleText.equals(title);
        }
    }
}
