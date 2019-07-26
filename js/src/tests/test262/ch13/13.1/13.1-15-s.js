















function testcase() {

        try {
            eval("'use strict';function _13_1_15_fun(eval) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
