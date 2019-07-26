



package org.mozilla.gecko.tests.helpers;

import org.mozilla.gecko.Assert;

import junit.framework.AssertionFailedError;

import java.util.regex.Matcher;
import java.util.regex.Pattern;





public final class JavascriptMessageParser {

    



    public static final String EVENT_TYPE = "Robocop:JS";

    
    
    private static final Pattern testMessagePattern =
        Pattern.compile("\n+TEST-(.*) \\| (.*) \\| (.*)\n*");

    private final Assert asserter;
    
    private String lastTestName = "";
    
    private boolean testFinishedMessageSeen = false;

    public JavascriptMessageParser(final Assert asserter) {
        this.asserter = asserter;
    }

    public boolean isTestFinished() {
        return testFinishedMessageSeen;
    }

    public void logMessage(final String str) {
        final Matcher m = testMessagePattern.matcher(str);

        if (m.matches()) {
            final String type = m.group(1);
            final String name = m.group(2);
            final String message = m.group(3);

            if ("INFO".equals(type)) {
                asserter.info(name, message);
                testFinishedMessageSeen = testFinishedMessageSeen ||
                                          "exiting test".equals(message);
            } else if ("PASS".equals(type)) {
                asserter.ok(true, name, message);
            } else if ("UNEXPECTED-FAIL".equals(type)) {
                try {
                    asserter.ok(false, name, message);
                } catch (AssertionFailedError e) {
                    
                    
                }
            } else if ("KNOWN-FAIL".equals(type)) {
                asserter.todo(false, name, message);
            } else if ("UNEXPECTED-PASS".equals(type)) {
                asserter.todo(true, name, message);
            }

            lastTestName = name;
        } else {
            
            
            asserter.info(lastTestName, str.trim());
        }
    }
}
