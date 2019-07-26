















function testcase() {

        try {
            eval("function _13_1_20_fun(arguments) { 'use strict'; }");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
