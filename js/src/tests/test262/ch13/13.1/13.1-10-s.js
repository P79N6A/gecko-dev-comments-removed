















function testcase() {
        "use strict";

        try {
            eval("var _13_1_10_fun = function (param, param, param) { };")
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
