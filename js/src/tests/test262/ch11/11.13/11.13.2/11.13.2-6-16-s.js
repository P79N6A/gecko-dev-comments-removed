











function testcase() {
        "use strict";
        var blah = arguments;
        try {
            eval("arguments -= 20;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError && blah === arguments;
        }
    }
runTestCase(testcase);
