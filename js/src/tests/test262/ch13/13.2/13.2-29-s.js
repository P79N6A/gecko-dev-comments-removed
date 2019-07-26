












function testcase() {
        function foo() {"use strict";}
        return ! Object.getOwnPropertyDescriptor(foo, 
                                                  "caller").configurable;
}
runTestCase(testcase);