











function testcase() {
        "use strict";
        var funObj = new Function("a", "eval('public = 1;');");
        funObj();
        return true;
    }
runTestCase(testcase);
