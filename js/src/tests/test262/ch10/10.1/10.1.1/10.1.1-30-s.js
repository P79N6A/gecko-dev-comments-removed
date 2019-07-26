











function testcase() {
        try {
            var funObj = new Function("a", "'use strict'; eval('public = 1;');");
            funObj();
            return false;
        } catch (e) {
            return e instanceof SyntaxError;
        }
    }
runTestCase(testcase);
