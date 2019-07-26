











function testcase() {
        "use strict";
        function funObj(x) {
            eval("delete x;");
        }

        try {
            funObj(1);
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
