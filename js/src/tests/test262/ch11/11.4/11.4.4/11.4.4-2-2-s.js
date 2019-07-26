











function testcase() {
        "use strict";
        var blah = arguments;
        try {
            eval("++arguments;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError && blah === arguments;
        }
    }
runTestCase(testcase);
