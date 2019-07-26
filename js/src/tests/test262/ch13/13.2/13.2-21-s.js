












function testcase() {
        function foo () {"use strict";}
        try {
            var temp = foo.caller;
            return false;
        }
        catch (e) {
            return e instanceof TypeError;
        }
}
runTestCase(testcase);