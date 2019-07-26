












function testcase() {
        function foo() {"use strict";}
        return ! Object.getOwnPropertyDescriptor(foo, 
                                                  "arguments").configurable;
}
runTestCase(testcase);