












function testcase() {
        return Object.getOwnPropertyDescriptor(Function("'use strict';"), 
                                               "caller") === undefined;
}
runTestCase(testcase);
