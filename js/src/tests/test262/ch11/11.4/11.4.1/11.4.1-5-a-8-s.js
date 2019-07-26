











function testcase() {
        "use strict";
        var funObj = function () { };

        try {
            eval("delete funObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
