















function testcase() {

        try {
            eval("var _13_1_30_fun = function (param, param) { 'use strict'; };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
