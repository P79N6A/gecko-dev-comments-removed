












function testcase() {
        return Object.getOwnPropertyDescriptor(new Function("'use strict';"), 
                                               "caller") === undefined;
}
runTestCase(testcase);
