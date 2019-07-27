



package org.mozilla.gecko.tests;

import java.util.concurrent.atomic.AtomicBoolean;

import com.jayway.android.robotium.solo.Condition;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.tests.helpers.AssertionHelper;
import org.mozilla.gecko.tests.helpers.GeckoHelper;
import org.mozilla.gecko.tests.helpers.JavascriptBridge;
import org.mozilla.gecko.tests.helpers.NavigationHelper;
import org.mozilla.gecko.tests.helpers.WaitHelper;
import org.mozilla.gecko.util.GeckoRequest;
import org.mozilla.gecko.util.NativeJSObject;




public class testGeckoRequest extends UITest {
    private static final String TEST_JS = "testGeckoRequest.js";
    private static final String REQUEST_EVENT = "Robocop:GeckoRequest";
    private static final String REQUEST_EXCEPTION_EVENT = "Robocop:GeckoRequestException";
    private static final int MAX_WAIT_MS = 5000;

    private JavascriptBridge js;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        js = new JavascriptBridge(this);
    }

    @Override
    public void tearDown() throws Exception {
        js.disconnect();
        super.tearDown();
    }

    public void testGeckoRequest() {
        GeckoHelper.blockForReady();
        NavigationHelper.enterAndLoadUrl(mStringHelper.getHarnessUrlForJavascript(TEST_JS));

        
        js.syncCall("add_request_listener", REQUEST_EVENT);

        
        checkFooRequest();

        
        js.syncCall("add_second_request_listener", REQUEST_EVENT);

        
        js.syncCall("remove_request_listener", REQUEST_EVENT);

        
        checkUnregisteredRequest();

        
        js.syncCall("add_exception_listener", REQUEST_EXCEPTION_EVENT);
        checkExceptionRequest();
        js.syncCall("remove_request_listener", REQUEST_EXCEPTION_EVENT);

        js.syncCall("finish_test");
    }

    private void checkFooRequest() {
        final AtomicBoolean responseReceived = new AtomicBoolean(false);
        final String data = "foo";

        GeckoAppShell.sendRequestToGecko(new GeckoRequest(REQUEST_EVENT, data) {
            @Override
            public void onResponse(NativeJSObject nativeJSObject) {
                
                final String result = nativeJSObject.getString("result");
                AssertionHelper.fAssertEquals("Sent and received request data", data + "bar", result);
                responseReceived.set(true);
            }
        });

        WaitHelper.waitFor("Received response for registered listener", new Condition() {
            @Override
            public boolean isSatisfied() {
                return responseReceived.get();
            }
        }, MAX_WAIT_MS);
    }

    private void checkExceptionRequest() {
        final AtomicBoolean responseReceived = new AtomicBoolean(false);
        final AtomicBoolean errorReceived = new AtomicBoolean(false);

        GeckoAppShell.sendRequestToGecko(new GeckoRequest(REQUEST_EXCEPTION_EVENT, null) {
            @Override
            public void onResponse(NativeJSObject nativeJSObject) {
                responseReceived.set(true);
            }

            @Override
            public void onError(NativeJSObject error) {
                errorReceived.set(true);
            }
        });

        WaitHelper.waitFor("Received error for listener with exception", new Condition() {
            @Override
            public boolean isSatisfied() {
                return errorReceived.get();
            }
        }, MAX_WAIT_MS);

        AssertionHelper.fAssertTrue("onResponse not called for listener with exception", !responseReceived.get());
    }

    private void checkUnregisteredRequest() {
        final AtomicBoolean responseReceived = new AtomicBoolean(false);

        GeckoAppShell.sendRequestToGecko(new GeckoRequest(REQUEST_EVENT, null) {
            @Override
            public void onResponse(NativeJSObject nativeJSObject) {
                responseReceived.set(true);
            }
        });

        
        
        getSolo().waitForCondition(new Condition() {
            @Override
            public boolean isSatisfied() {
                return responseReceived.get();
            }
        }, MAX_WAIT_MS);

        AssertionHelper.fAssertTrue("Did not receive response for unregistered listener", !responseReceived.get());
    }
}
