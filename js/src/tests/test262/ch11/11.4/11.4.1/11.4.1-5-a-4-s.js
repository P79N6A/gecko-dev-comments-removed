











function testcase() {
        "use strict";
        function funObj(x, y, z) {
            eval("delete y;");
        }

        try {
            funObj(1);
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
