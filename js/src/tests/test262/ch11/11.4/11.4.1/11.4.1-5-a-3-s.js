











function testcase() {
        "use strict";
        function funObj () { }

        try {
            eval("delete funObj");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
