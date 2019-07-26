














function testcase() {

        try {
            eval("'use strict'; function _13_0_15_fun() {eval = 42;};");
            _13_0_15_fun();
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
