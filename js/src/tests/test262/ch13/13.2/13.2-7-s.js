











function testcase() {
        var foo = new Function("'use strict';");
        
        for (var tempIndex in foo) {
            if (tempIndex === "caller") {
                return false;
            }
        }
        return true;
    }
runTestCase(testcase);
