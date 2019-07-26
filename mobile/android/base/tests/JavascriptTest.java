package org.mozilla.gecko.tests;

import org.mozilla.gecko.*;

import android.util.Log;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import junit.framework.AssertionFailedError;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;


public class JavascriptTest extends BaseTest {
    public static final String LOGTAG = "JavascriptTest";

    public final String javascriptUrl;

    public JavascriptTest(String javascriptUrl) {
        super();
        this.javascriptUrl = javascriptUrl;
    }

    @Override
    protected int getTestType() {
        return TEST_MOCHITEST;
    }

    



    protected static class JavascriptTestMessageParser {
        
        
        private static final Pattern testMessagePattern =
            Pattern.compile("\n+TEST-(.*) \\| (.*) \\| (.*)\n*");

        private final Assert mAsserter;

        
        private String lastTestName = "";

        
        private boolean testFinishedMessageSeen = false;

        public JavascriptTestMessageParser(final Assert asserter) {
            this.mAsserter = asserter;
        }

        private boolean testIsFinished() {
            return testFinishedMessageSeen;
        }

        private void logMessage(String str) {
            Matcher m = testMessagePattern.matcher(str);

            if (m.matches()) {
                String type = m.group(1);
                String name = m.group(2);
                String message = m.group(3);

                if ("INFO".equals(type)) {
                    mAsserter.info(name, message);
                    testFinishedMessageSeen = testFinishedMessageSeen ||
                        "exiting test".equals(message);
                } else if ("PASS".equals(type)) {
                    mAsserter.ok(true, name, message);
                } else if ("UNEXPECTED-FAIL".equals(type)) {
                    try {
                        mAsserter.ok(false, name, message);
                    } catch (junit.framework.AssertionFailedError e) {
                        
                        
                    }
                } else if ("KNOWN-FAIL".equals(type)) {
                    mAsserter.todo(false, name, message);
                } else if ("UNEXPECTED-PASS".equals(type)) {
                    mAsserter.todo(true, name, message);
                }

                lastTestName = name;
            } else {
                
                
                mAsserter.info(lastTestName, str.trim());
            }
        }
    }

    public void testJavascript() throws Exception {
        blockForGeckoReady();

        
        
        
        final Actions.EventExpecter expecter = mActions.expectGeckoEvent("Robocop:Status");
        mAsserter.dumpLog("Registered listener for Robocop:Status");

        final String url = getAbsoluteUrl("/robocop/robocop_javascript.html?path=" + javascriptUrl);
        mAsserter.dumpLog("Loading JavaScript test from " + url);

        loadUrl(url);

        final JavascriptTestMessageParser testMessageParser =
            new JavascriptTestMessageParser(mAsserter);

        try {
            while (true) {
                if (Log.isLoggable(LOGTAG, Log.VERBOSE)) {
                    Log.v(LOGTAG, "Waiting for Robocop:Status");
                }
                String data = expecter.blockForEventData();
                if (Log.isLoggable(LOGTAG, Log.VERBOSE)) {
                    Log.v(LOGTAG, "Got Robocop:Status with data '" + data + "'");
                }

                JSONObject o = new JSONObject(data);
                String innerType = o.getString("innerType");

                if (!"progress".equals(innerType)) {
                    throw new Exception("Unexpected Robocop:Status innerType " + innerType);
                }

                String message = o.getString("message");
                if (message == null) {
                    throw new Exception("Robocop:Status progress message must not be null");
                }

                testMessageParser.logMessage(message);

                if (testMessageParser.testIsFinished()) {
                    if (Log.isLoggable(LOGTAG, Log.DEBUG)) {
                        Log.d(LOGTAG, "Got test finished message");
                    }
                    break;
                }
            }
        } finally {
            expecter.unregisterListener();
            mAsserter.dumpLog("Unregistered listener for Robocop:Status");
        }
    }
}
