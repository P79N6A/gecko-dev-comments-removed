














function testcase() {
        "use strict";

        try {
            eval("var _13_0_8_fun = function () {eval = 42;};");
            _13_0_8_fun();
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
