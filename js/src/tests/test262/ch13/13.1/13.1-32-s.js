















function testcase() {

        try {
            eval("var _13_1_32_fun = function (param1, param2, param1) { 'use strict'; };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
