















function testcase() {
        "use strict";

        try {
            eval("var _13_1_8_fun = function (param, param) { };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
