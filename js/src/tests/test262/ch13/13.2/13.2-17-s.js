












function testcase() {
        var foo = Function("'use strict';");
        try {
            var temp = foo.arguments;
            return false;
        }
        catch (e) {
            return e instanceof TypeError;
        }
}
runTestCase(testcase);