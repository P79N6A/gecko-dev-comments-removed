












function testcase() {
        function foo() {"use strict";}
        return Object.getOwnPropertyDescriptor(foo, 
                                               "arguments") === undefined;
}
runTestCase(testcase);
