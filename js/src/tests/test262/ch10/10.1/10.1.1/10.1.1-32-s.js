











function testcase() {
        var funObj = new Function("a", "eval('public = 1;'); anotherVariable = 2; 'use strict';");
        funObj();
        return public === 1 && anotherVariable === 2;
    }
runTestCase(testcase);
