















function testcase() {
        "use strict";

        try {
            eval("function _13_1_3_fun(arguments) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
