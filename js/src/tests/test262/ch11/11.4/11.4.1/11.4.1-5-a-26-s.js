











function testcase() {
        "use strict";
        var errorBackup = Error;
        try {
            eval("delete Error;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        } finally {
            Error = errorBackup;
        }
        
    }
runTestCase(testcase);
