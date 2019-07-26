











function testcase() {
        "use strict";
        var numObj = new Number(0);

        try {
            eval("delete numObj;");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
