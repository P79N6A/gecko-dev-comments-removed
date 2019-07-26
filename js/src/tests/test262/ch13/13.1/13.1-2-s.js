















function testcase() {
        "use strict";

        try {
            eval("var _13_1_2_fun = function (eval) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
