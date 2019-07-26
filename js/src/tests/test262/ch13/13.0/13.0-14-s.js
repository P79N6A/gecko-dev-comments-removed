














function testcase() {

        try {
            var _13_0_14_fun = new Function(" ", "'use strict'; eval = 42; ");
            _13_0_14_fun();
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
