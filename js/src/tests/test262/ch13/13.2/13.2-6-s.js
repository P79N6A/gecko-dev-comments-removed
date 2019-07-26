












function testcase() {
        var foo = new Function("'use strict';");
        try {
            foo.caller = 41;
            return false;
        }
        catch (e) {
            return e instanceof TypeError;
        }
}
runTestCase(testcase);