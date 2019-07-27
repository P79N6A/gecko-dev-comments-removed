












function testcase() {
        return Object.getOwnPropertyDescriptor(new Function("'use strict';"), 
                                               "arguments") === undefined;
}
runTestCase(testcase);
