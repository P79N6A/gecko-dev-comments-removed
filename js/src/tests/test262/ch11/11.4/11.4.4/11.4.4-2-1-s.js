











function testcase() {
        "use strict";
        var blah = eval;
        try {
            eval("++eval;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError && blah === eval;
        }
    }
runTestCase(testcase);
