















function testcase() {

        try {
            eval("'use strict'; var _13_1_29_fun = function (param, param) { };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
