











function testcase() {
        var foo = new Function("'use strict';");
        try {
            var temp = foo.caller;
            return false;
        }
        catch (e) {
            return e instanceof TypeError;
        }
    }
runTestCase(testcase);
