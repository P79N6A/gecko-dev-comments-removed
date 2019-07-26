












function testcase() {
        return ! Object.getOwnPropertyDescriptor(new Function("'use strict';"), 
                                                  "arguments").configurable;
}
runTestCase(testcase);