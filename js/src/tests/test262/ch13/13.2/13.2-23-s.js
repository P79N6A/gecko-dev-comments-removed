












function testcase() {
        function foo () {"use strict";}
        for (var tempIndex in foo) {
            if (tempIndex === "caller") {
                return false;
            }
        }
        return true;
}
runTestCase(testcase);