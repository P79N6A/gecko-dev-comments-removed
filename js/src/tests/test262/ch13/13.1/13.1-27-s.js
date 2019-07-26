















function testcase() {

        try {
            eval("'use strict'; function _13_1_27_fun(param, param, param) { }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
