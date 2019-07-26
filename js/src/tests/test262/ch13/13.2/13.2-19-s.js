












function testcase() {
        var foo = Function("'use strict';");
        
        for (var tempIndex in foo) {
            if (tempIndex === "arguments") {
                return false;
            }
        }
        return true;
}
runTestCase(testcase);