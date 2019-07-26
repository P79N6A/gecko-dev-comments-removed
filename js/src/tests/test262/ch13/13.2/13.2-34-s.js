












function testcase() {
        return ! Object.getOwnPropertyDescriptor(Function("'use strict';"), 
                                                  "arguments").configurable;
}
runTestCase(testcase);