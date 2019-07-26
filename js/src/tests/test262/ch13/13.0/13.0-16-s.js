














function testcase() {

        try {
            eval("'use strict'; var _13_0_16_fun = function () {eval = 42;};");
            _13_0_16_fun();
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
