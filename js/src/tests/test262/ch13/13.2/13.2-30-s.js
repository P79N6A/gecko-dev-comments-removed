












function testcase() {
        return ! Object.getOwnPropertyDescriptor(Function("'use strict';"), 
                                                  "caller").configurable;
}
runTestCase(testcase);