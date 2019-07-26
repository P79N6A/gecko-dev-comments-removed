












function testcase() {
        return ! Object.getOwnPropertyDescriptor(new Function("'use strict';"), 
                                                  "caller").configurable;
}
runTestCase(testcase);