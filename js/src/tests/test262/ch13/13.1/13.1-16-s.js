















function testcase() {

        try {
            eval("function _13_1_16_fun(eval) { 'use strict'; }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
