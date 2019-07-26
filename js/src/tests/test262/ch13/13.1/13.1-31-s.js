















function testcase() {

        try {
            eval("'use strict'; var _13_1_31_fun = function (param1, param2, param1) { };");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
