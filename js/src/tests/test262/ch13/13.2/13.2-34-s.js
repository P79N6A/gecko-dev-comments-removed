












function testcase() {
        return Object.getOwnPropertyDescriptor(Function("'use strict';"), 
                                               "arguments") === undefined;
}
runTestCase(testcase);
