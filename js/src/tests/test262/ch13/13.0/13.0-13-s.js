














function testcase() {
       
        try {
            eval("var _13_0_13_fun = new Function(\" \", \"'use strict'; eval = 42;\"); _13_0_13_fun();");
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
